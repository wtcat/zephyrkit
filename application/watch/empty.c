#include <string.h>
#include <stdio.h>

#include <zephyr.h>
#include <drivers/kscan.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <soc.h>
#include <devicetree.h>

#include "drivers_ext/sensor_priv.h"

#include "stp/stp.h"
#include "stp/stp_core.h"
#include "stp/opc_proto.h"

#include "base/vibration.h"


#include <logging/log.h>
LOG_MODULE_REGISTER(test);

#include "services/magnet/magnet_svr.h"

//#define CONFIG_GPIO_GENERATE_PULSE
//#define CONFIG_BUTTON_TEST
//#define CONFIG_EVM_TEST
//#define CONFIG_MAGNET_TEST

#if CONFIG_STP
static int __unused msg_send_test(int major, int minor, 
    const char *info, size_t len)
{
    struct stp_chan *chan = opc_chan_get(major);
    struct opc_subhdr *hdr = opc_alloc(len);
    struct stp_bufv v[1];
    
    hdr->minor = minor;
    hdr->len = ltons(len);
    memcpy(hdr->data, info, len);

    v[0].buf = hdr;
    v[0].len = sizeof(struct opc_subhdr) + len;
    return stp_sendmsgs(chan, v, 1);
}

static int __unused msgs_send_test(void)
{
    struct stp_chan *chan = opc_chan_get(OPC_CLASS_OTA);
    struct opc_subhdr *hdr_1, *hdr_2, *hdr_3;
    struct stp_bufv v[3];

    hdr_1 = opc_alloc(7 + sizeof(*hdr_1));
    hdr_1->minor = 0x01;
    hdr_1->len = ltons(7);
    memcpy(hdr_1->data, "Thanks", 7);
    v[0].buf = hdr_1;
    v[0].len = sizeof(struct opc_subhdr) + 7;

    hdr_2 = opc_alloc(11 + sizeof(*hdr_2));
    hdr_2->minor = 0x02;
    hdr_2->len = ltons(11);
    memcpy(hdr_2->data, "Don't move", 11);
    v[1].buf = hdr_2;
    v[1].len = sizeof(struct opc_subhdr) + 11;


    hdr_3 = opc_alloc(4 + sizeof(*hdr_3));
    hdr_3->minor = 0x03;
    hdr_3->len = ltons(4);
    memcpy(hdr_3->data, "GO!", 4);
    v[2].buf = hdr_3;
    v[2].len = sizeof(struct opc_subhdr) + 4;

    return stp_sendmsgs(chan, v, ARRAY_SIZE(v));
}

#endif 

#define ADC_NAME DT_LABEL(DT_INST(0, ambiq_apollo3p_adc))

#ifdef CONFIG_GPIO_GENERATE_PULSE
static void generate_pulses(int array[], size_t size, int pin, 
    int interval)
{
    static const struct device *gpio;
    static bool actived;

    if (!actived) {
        actived = true;
        gpio = device_get_binding(pin2name(pin));
        __ASSERT(gpio != NULL, "");
        gpio_pin_configure(gpio, pin2gpio(pin), GPIO_OUTPUT);
    }
    //Ugly code.....
    unsigned int key = irq_lock();
    gpio_pin_set(gpio, pin2gpio(pin), 0);
    k_busy_wait(1500);
    gpio_pin_set(gpio, pin2gpio(pin), 1);

    for (int i = 0; i < size; i++) {
        int n = array[i];
        while (n > 0) {
            gpio_pin_set(gpio, pin2gpio(pin), 0);
            k_busy_wait(interval);
            gpio_pin_set(gpio, pin2gpio(pin), 1);
            k_busy_wait(interval);
            n--;
        }
        k_busy_wait(600);
    }
    irq_unlock(key);
}

static void keyboard_notify(const struct device *dev, uint32_t row, 
    uint32_t column, bool pressed)
{
    int pulses_array[] = {33, 75};
    static bool old;

    if (!pressed && old)
        generate_pulses(pulses_array, ARRAY_SIZE(pulses_array), 46, 20);
    old = pressed;
}

#else /* !CONFIG_GPIO_GENERATE_PULSE */

#ifdef CONFIG_BUTTON_TEST
static void keyboard_notify(const struct device *dev, uint32_t row, 
    uint32_t column, bool pressed)
{
    printf("Key(%u): %s\n", row, pressed? "Pressed": "Released");

#ifdef CONFIG_EVM_TEST
    if (pressed) {
        event_report(EV_BAT_CHARGE,  30, 0, 0);
        event_report(EV_OTA_UPGRADE, 29, 0, 0);
        event_report(EV_INCOMING_REMIND, 10, 0, 0);
    } else {
        event_sync();
    }
#endif
}
#endif
#endif /* CONFIG_GPIO_GENERATE_PULSE */

static void charge_trigger(const struct device *dev,
    struct sensor_trigger *trigger)
{
    int type = trigger->type;
    switch (type) {
    case SENSOR_TRIG_CHARGE_IN:
        printf("=> Charging in\n");
        break;
    case SENSOR_TRIG_CHARGE_OUT:
        printf("=> Charging removed\n");
        break;
    case SENSOR_TRIG_CHARGE_FINISH:
        printf("=> Charging finished\n");
        break;
    case SENSOR_TRIG_CHARGE_OV:
        printf("=> Charging over voltage\n");
        break;
    case SENSOR_TRIG_CHARGE_OT:
        printf("=> Charging over temperature\n");
        break;
    case SENSOR_TRIG_CHARGE_UT:
        printf("=> Charging under temperature\n");
        break;
    default:
        printf("Invalid charging status\n");
        break;
    }
}

#ifdef CONFIG_EVM_TEST
static void event_process(struct event_struct *e)
{
    switch (e->type) {
    case EV_OTA_UPGRADE:
        printf("Event process[%d](%d): OTA upgrade\n", e->time, e->prio);
        break;
    case EV_BAT_CHARGE:
        printf("Event process[%d](%d): Battery charge\n", e->time, e->prio);
        break;
    case EV_INCOMING_REMIND:
        printf("Event process[%d](%d): Incoming remind\n", e->time, e->prio);
        break;
    default:
        break;
    }
    event_ack(e);
}

static struct event_notifer kevent = {
    .notify = event_process
};
#endif /* CONFIG_EVM_TEST */


int main(void)
{
#ifdef CONFIG_EVM_TEST
    event_manager_init();
    event_notifier_register(&kevent);
#endif
#ifdef CONFIG_BUTTON_TEST
    const struct device *key = device_get_binding("buttons");
    if (key) {
        kscan_config(key, keyboard_notify);
        kscan_enable_callback(key);
    }
#endif
    const struct device *chg = device_get_binding("CW6305");
    if (chg) 
        sensor_trigger_set(chg, NULL, charge_trigger);

#ifdef CONFIG_CRASH_TEST
    *(volatile int *)0 = 0xFF;
#endif
#ifdef CONFIG_MAGNET_TEST
    float data[3], angle[3];
    for (;;) {
        printf("Starting magnetic service...\n");
        magnet_service_start();
        for (int i = 0; i < 10; i++) {
            magnet_get_xyz(&data[0], &data[1], &data[2]);
            magnet_get_angle(&angle[0], &angle[1], &angle[2]);
            printf("X:Azimuth(%.4f: %.2f) Y:Pitch(%.4f: %.2f) Z:Roll(%.4f: %.2f)\n", 
                data[0], angle[0], 
                data[1], angle[1],
                data[2], angle[2]);
            k_msleep(1000);
        }
        magnet_service_stop();
        printf("Stoped magnetic service\n");
        k_msleep(2000);
    }
#else

#ifdef CONFIG_EVM_TEST
    int old_prio;
    int level = 31;
    for (;;) {
        k_sleep(K_MSEC(3000));
        old_prio = event_disable_level_set(level);
        printf("Event disable priority level: %d(old: %d)\n", level, old_prio);
        level--;
        if (level < 0)
            level = 32;
    }
#endif
#endif
    return 0;
}
