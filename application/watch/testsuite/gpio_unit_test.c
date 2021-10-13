#include <zephyr.h>
#include <device.h>
#include <devicetree.h>

#include <drivers/gpio.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(gpio_test);

#define GPIO_PORT DT_PROP(DT_NODELABEL(gpio1), label)

static struct gpio_callback  gpio_cb;


static void gpio_pin6_isr(const struct device *port,
    struct gpio_callback *cb, gpio_port_pins_t pins)
{
    printk("GPIO interrupt signal\n");
}
					
void main(void)
{
    const struct device *dev;
    int ret;

    dev = device_get_binding("GPIO_1");
    if (!dev) {
        LOG_ERR(GPIO_PORT " is not exist\n");
        return;
    }

    gpio_init_callback(&gpio_cb, gpio_pin6_isr, BIT(6));
    gpio_add_callback(dev, &gpio_cb);
    ret = gpio_pin_interrupt_configure(dev, 6, GPIO_INT_EDGE_FALLING);
    if (ret) {
        LOG_ERR("Configure gpio interrupt failed with error %d\n", ret);
    }

    for ( ; ; ) {
        k_sleep(K_MSEC(2000));
        ret = -1;
        ret = gpio_pin_get(dev, 6);
        if (ret >= 0)
            printk("Pin6 logic status is: %d\n", ret);
    }

}
