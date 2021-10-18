#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <drivers/led.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(base);

struct vib_struct {
    struct k_timer timer;
    struct k_sem sem;
    const struct device *dev;
    int remain_times;
    uint32_t duty_on;
    uint32_t duty_off;
    uint32_t time_on;
    uint32_t time_off;
    uint8_t active;
};

static struct vib_struct vib_instance;

static inline int __vibration_on(struct vib_struct *vib)
{
    return led_on(vib->dev, 0);
}

static inline int __vibration_off(struct vib_struct *vib)
{
    return led_off(vib->dev, 0);
}

static inline int __vibration_launch(struct vib_struct *vib, uint32_t duty_on, 
    uint32_t duty_off)
{
    return led_blink(vib->dev, 0, duty_on, duty_off);
}

static void timer_process(struct k_timer *timer)
{
    struct vib_struct *vib = CONTAINER_OF(timer, struct vib_struct, timer);
    vib->remain_times -= vib->active;
    if (vib->remain_times > 0) {
        k_timeout_t period;
        if (vib->active) {
            period = K_MSEC(vib->time_on);
            vib->active = false;
            __vibration_off(vib);
        } else {
            period = K_MSEC(vib->time_off);
            vib->active = true;
            __vibration_launch(vib, vib->duty_on, vib->duty_off);
        }

        /* Update timer period */
        if (Z_TICK_ABS(period.ticks) < 0)
            period.ticks = MAX(period.ticks - 1, 1);
        timer->period = period;
    } else {
        __vibration_off(vib);
    }
}

int vib_on(void)
{
    struct vib_struct *vib = &vib_instance;
    int ret;
    ret = k_sem_take(&vib->sem, K_FOREVER);
    __ASSERT(ret == 0, "");
    k_timer_stop(&vib->timer);
    ret = __vibration_on(vib);
    k_sem_give(&vib->sem);
    return ret;
}

int vib_off(void)
{
    struct vib_struct *vib = &vib_instance;
    int ret;
    ret = k_sem_take(&vib->sem, K_FOREVER);
    __ASSERT(ret == 0, "");
    k_timer_stop(&vib->timer);
    ret = __vibration_off(vib);
    k_sem_give(&vib->sem);
    return ret;
}

int vib_launch(uint32_t ton, uint32_t toff, uint32_t times)
{
    struct vib_struct *vib = &vib_instance;
    int ret;
    ret = k_sem_take(&vib->sem, K_FOREVER);
    __ASSERT(ret == 0, "");
    k_timer_stop(&vib->timer);
    ret = led_blink(vib->dev, times, ton, toff);
    k_sem_give(&vib->sem);
    return ret;
}

int vib_start(uint32_t ton, uint32_t toff, uint32_t duty_on, 
    uint32_t duty_off, uint32_t times)
{
    struct vib_struct *vib = &vib_instance;
    int ret;
    ret = k_sem_take(&vib->sem, K_FOREVER);
    __ASSERT(ret == 0, "");
    k_timer_stop(&vib->timer);
    vib->duty_on = duty_on;
    vib->duty_off = duty_off;
    vib->active = true;
    vib->time_on = ton;
    vib->time_off = toff;
    vib->remain_times = times;
    ret = __vibration_launch(vib, duty_on, duty_off);
    if (!ret)
        k_timer_start(&vib->timer, K_MSEC(ton), K_MSEC(toff));
    k_sem_give(&vib->sem);
    return 0;
}

static int vib_init(const struct device *dev)
{
    struct vib_struct *vib = &vib_instance;
    ARG_UNUSED(dev);
    vib->dev = device_get_binding("MOTOR");
    if (vib->dev == NULL) {
        LOG_WRN("%s(): Not found vibration device\n", __func__);
        return -EINVAL;
    }
    k_sem_init(&vib->sem, 1, 1);
    k_timer_init(&vib->timer, timer_process, NULL);
    return 0;
}

SYS_INIT(vib_init, APPLICATION, 50);
    