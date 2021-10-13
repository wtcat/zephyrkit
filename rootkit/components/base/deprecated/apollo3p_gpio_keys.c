#include <init.h>
#include <kernel.h>
#include <errno.h>
#include <drivers/gpio.h>
#include <sys/__assert.h>

#include <soc.h>

#include "apollo3p_gpio_keys.h"
#include "base/input_event.h"

//#define CONFIG_KEY_LOG

#define GPIO_KEY_IDLE_POLARITY   1
#define GPIO_KEY_ACTIVE_POLARITY 0

#define QKEY_SIZE 8

/* Debounce time (unit: ms)*/
#define KEY_DEBOUNCE 20


#ifdef CONFIG_KEY_LOG
#define KLOG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define KLOG(fmt, ...)
#endif

#define GPIO_PIN(n) ((n) & 31)


struct gpio_key {
    struct k_timer timer;
    struct gpio_callback cb;
    const struct device *gpio;
    struct key_device *dev;
    const char *dname;
    uint16_t pin;
    uint16_t code;
    uint16_t status;
    uint16_t input;
    bool debounce_on;
};

struct key_device {
    struct key_event buf[QKEY_SIZE];
    struct gpio_key *const keys;
	struct k_spinlock spin;
    int  nums;
    int  head;
    int  tail;
    void (*notify)(void *context);
    void *context;
};

static struct gpio_key key_table[] = {
    {
    	.dname = "GPIO_1",
        .pin = GPIO_PIN(38),
        .status = GPIO_KEY_IDLE_POLARITY, 
        .code = 0
    }, {
    	.dname = "GPIO_1",
        .pin = GPIO_PIN(56),
        .status = GPIO_KEY_IDLE_POLARITY, 
        .code = 1
    }, {
    	.dname = "GPIO_1",
        .pin = GPIO_PIN(57),
        .status = GPIO_KEY_IDLE_POLARITY, 
        .code = 2
    }, {
    	.dname = "GPIO_1",
        .pin = GPIO_PIN(58),
        .status = GPIO_KEY_IDLE_POLARITY, 
        .code = 3
    },
};

static struct key_device key_dev = {
    .keys = key_table,
    .nums = ARRAY_SIZE(key_table),
};

static inline void key_queue_reset(struct key_device *dev)
{
    dev->head = dev->tail = 0;
}

static inline bool key_queue_empty(struct key_device *dev)
{
    return dev->head == dev->tail;
}

static inline void key_notify(struct key_device *dev)
{
    if (dev->notify)
        dev->notify(dev->context);
}

static void enqueue_key(struct key_device *dev, int value, int code)
{
    k_spinlock_key_t key;
    int new_tail;

    key = k_spin_lock(&dev->spin);
    new_tail = (dev->tail + 1) & (QKEY_SIZE-1);
    if (new_tail == dev->head)
        key_queue_reset(dev);

    dev->buf[dev->tail].code = code;
    dev->buf[dev->tail].value = value;
    dev->tail = new_tail;
    k_spin_unlock(&dev->spin, key);
}

static void key_timer_cb(struct k_timer *timer)
{
    struct gpio_key *key = CONTAINER_OF(timer, struct gpio_key, timer);
    uint32_t status = gpio_pin_get(key->gpio, key->pin);

    if (status == GPIO_KEY_ACTIVE_POLARITY) {
        if (key->status == GPIO_KEY_ACTIVE_POLARITY) {
            if (!key->debounce_on) {
                enqueue_key(key->dev, KEY_ACTION_LONG_PRESS, key->code);
                key->input = 1;
                input_event_post(INPUT_EVENT_KEY);
                KLOG("key%d long-press\n", key->code);
            } else {
                key->debounce_on = false;
                k_timer_start(timer, K_MSEC(2000), K_NO_WAIT);
            }
        }
    }
}

static int gpio_key_isr_process(struct gpio_key *key)
{
    uint32_t status = gpio_pin_get(key->gpio, key->pin);
    uint32_t previous = key->status;

    key->status = status;
    if (previous == GPIO_KEY_IDLE_POLARITY) {
        if (status == GPIO_KEY_ACTIVE_POLARITY) {
            KLOG("key%d debounce on\n", key->code);
            key->debounce_on = true;
            key->input = 0;
            k_timer_start(&key->timer, K_MSEC(KEY_DEBOUNCE), K_NO_WAIT);
            return 1;
        }
    } else {
        if (status == GPIO_KEY_IDLE_POLARITY) {
            if (!key->input && !key->debounce_on) {
                KLOG("key%d released\n", key->code);
                k_timer_stop(&key->timer);
                enqueue_key(key->dev, KEY_ACTION_CLICK, key->code);
                input_event_post(INPUT_EVENT_KEY);
            }
            key->debounce_on = false;
        }
    }
    return 0;
}

static void gpio_key_isr(const struct device *dev, 
	struct gpio_callback *cb, gpio_port_pins_t pins)
{
    struct gpio_key *key;
    ARG_UNUSED(dev);
    ARG_UNUSED(pins);
    key = CONTAINER_OF(cb, struct gpio_key, cb);
    gpio_key_isr_process(key);
}

static int read_keys(void *hdl, struct input_struct *input, input_report_fn report)
{
    struct key_device *dev = hdl;
    struct input_event val;
    k_spinlock_key_t key;

    key = k_spin_lock(&key_dev.spin);
    while (!key_queue_empty(dev)) {
        val.key = dev->buf[dev->head];
        dev->head = (dev->head + 1) & (QKEY_SIZE-1);
        k_spin_unlock(&key_dev.spin, key);
        if (val.key.value == KEY_ACTION_CLICK)
            report(input, &val, INPUT_EVENT_KEY);
        
        key = k_spin_lock(&key_dev.spin);
    }
    k_spin_unlock(&key_dev.spin, key);
    return 0;
}

static const struct input_device key_device = {
    .read = read_keys,
    .private_data = &key_dev
};

static int gpio_key_setup(const struct device *dev __unused)
{
    struct key_device *kd = &key_dev;
    int ret;

    key_queue_reset(kd);
    for (int i = 0; i < kd->nums; i++) {
        struct gpio_key *key = kd->keys + i;
        key->gpio = device_get_binding(key->dname);
        if (!key->gpio) {
            continue;
        }
		
        key->dev = kd;
        k_timer_init(&key->timer, key_timer_cb, NULL);
        gpio_init_callback(&key->cb, gpio_key_isr, BIT(key->pin));
        gpio_add_callback(key->gpio, &key->cb);
        ret = gpio_pin_configure(key->gpio, key->pin, 
            GPIO_INT_EDGE_BOTH|GPIO_INPUT|GPIO_PULL_UP);
        if (ret) {
            printk("%s install gpio interrupt failed\n", __func__);
            return -EINVAL;
        }
    }
    return input_device_register(&key_device, INPUT_EVENT_KEY);
}

SYS_INIT(gpio_key_setup, POST_KERNEL,
	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT+6);
