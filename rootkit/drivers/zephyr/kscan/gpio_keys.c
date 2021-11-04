#include <errno.h>

#include <kernel.h>
#include <version.h>
#include <drivers/gpio.h>
#include <drivers/kscan.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(kscan, CONFIG_KSCAN_LOG_LEVEL);

#include <soc.h>

#define DT_DRV_COMPAT gpio_buttons
#define GPIO_DEBOUNCE 10 /* Unit: ms */


struct gpio_key_device;
struct gpio_button {
#if KERNEL_VERSION_NUMBER < ZEPHYR_VERSION(2,5,99)
    struct k_delayed_work work;
#else
    struct k_work_delayable work;
#endif
    struct gpio_key_device *dev;
    struct gpio_callback cb;
    const struct device *gpio;
    uint16_t pin;
    uint16_t polarity;
    uint16_t code;
    uint16_t status;
};

struct gpio_key_device {
    const struct device *device;
    struct gpio_button *btn;
    uint16_t size;
	kscan_callback_t callback;
	bool enabled;
};

static void key_default_handler(const struct device *dev, uint32_t row, 
    uint32_t column, bool pressed)
{
    LOG_WRN("Please set kscan callback\n");
}

static inline void report_key(struct gpio_button *btn, bool pressed)
{
    struct gpio_key_device *kd = btn->dev;
    if (likely(kd->enabled))
        kd->callback(kd->device, btn->code, 0, pressed);
}

static void gpio_key_work(struct k_work *work)
{
    struct gpio_button *btn = CONTAINER_OF(work, struct gpio_button, work.work);
    uint32_t status = gpio_pin_get(btn->gpio, btn->pin);

    if (status != btn->status) {
        report_key(btn, status == btn->polarity);
        btn->status = status;
    }
}

static inline void gpio_key_isr_process(struct gpio_button *btn)
{
#if KERNEL_VERSION_NUMBER < ZEPHYR_VERSION(2,5,99)
    k_delayed_work_submit(&btn->work, K_MSEC(GPIO_DEBOUNCE));
#else					
    k_work_schedule(&btn->work, K_MSEC(GPIO_DEBOUNCE));
#endif
}

static void gpio_key_isr(const struct device *dev, 
	struct gpio_callback *cb, gpio_port_pins_t pins)
{
    struct gpio_button *key = CONTAINER_OF(cb, struct gpio_button, cb);
    ARG_UNUSED(dev);
    ARG_UNUSED(pins);
    
    gpio_key_isr_process(key);
}

static int gpio_keys_configure(const struct device *dev, kscan_callback_t callback)
{
	struct gpio_key_device *kd = dev->data;
	if (!callback) {
		LOG_ERR("%s(): Callback is null\n", __func__);
		return -EINVAL;
	}
	LOG_DBG("%s: set callback", dev->name);
    if (kd->callback != key_default_handler)
        LOG_WRN("The kcan callback already exists\n");
	kd->callback = callback;
	return 0;
}

static int gpio_keys_enable_callback(const struct device *dev)
{
	struct gpio_key_device *kd = dev->data;
	LOG_DBG("%s: enable cb", dev->name);
	kd->enabled = true;
	return 0;
}

static int gpio_keys_disable_callback(const struct device *dev)
{
	struct gpio_key_device *kd = dev->data;
	LOG_DBG("%s: disable cb", dev->name);
	kd->enabled = false;
	return 0;
}

static int gpio_keys_init(const struct device *dev)
{
    struct gpio_key_device *kd = dev->data;

    for (int i = 0; i < kd->size; i++) {
        struct gpio_button *btn = kd->btn + i;
        btn->dev = kd;

    #if KERNEL_VERSION_NUMBER < ZEPHYR_VERSION(2,5,99)
        k_delayed_work_init(&btn->work, gpio_key_work);
    #else
        k_work_init_delayable(&btn->work, gpio_key_work);
    #endif
        gpio_init_callback(&btn->cb, gpio_key_isr, BIT(btn->pin));
        gpio_add_callback(btn->gpio, &btn->cb);
        int ret = gpio_pin_interrupt_configure(btn->gpio, btn->pin,
            GPIO_INT_EDGE_BOTH|GPIO_INPUT|GPIO_PULL_UP);
        if (ret) {
            LOG_ERR("%s install gpio interrupt failed\n", __func__);
            return -EINVAL;
        }
    }
	return 0;
}

static const struct kscan_driver_api gpio_buttons_driver = {
    .config = gpio_keys_configure,
    .enable_callback = gpio_keys_enable_callback,
    .disable_callback = gpio_keys_disable_callback
};

#define GPIO_KEY_ARGS(nid)	{\
        .gpio      = DEVICE_DT_GET(DT_GPIO_CTLR(nid, gpios)), \
        .pin       = DT_PHA(nid, gpios, pin),         \
        .polarity  = !DT_PHA(nid, gpios, flags),    \
        .code      = DT_PROP(nid, code), \
    },

#define GPIO_KEYS(nid) \
    static struct gpio_button gpio_buttons##nid[] = { \
        DT_INST_FOREACH_CHILD(nid, GPIO_KEY_ARGS) \
    }; \
    static struct gpio_key_device gpio_key_device_##nid = {\
        .btn = gpio_buttons##nid, \
        .size = ARRAY_SIZE(gpio_buttons##nid), \
        .callback = key_default_handler, \
    }; \
    DEVICE_DT_DEFINE(DT_DRV_INST(nid),   \
        gpio_keys_init,              \
        device_pm_control_nop,          \
        &gpio_key_device_##nid,                   \
        NULL,           \
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
        &gpio_buttons_driver);

DT_INST_FOREACH_STATUS_OKAY(GPIO_KEYS)
