#include <errno.h>
#include <kernel.h>
#include <init.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <sys/atomic.h>
#include <sys/__assert.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(tp);

#include <soc.h>
#include "i2c/i2c-message.h"
#include "base/input_event.h"

//#define DEBUG_ITE7259

#define BUS_DEV "I2C_2"
#define GPIO_DEV "GPIO_1"

/*
 * Input event type
 */
#define INPUT_TP_EVENT   0x01
#define INPUT_KEY_EVENT 0x02

/* 
 * ITE7259 commands
 */
#define COMMAND_RESPONSE_BUFFER_INDEX 0xA0
#define ITE7259_CMD_ADDR      0x20
#define ITE7259_STATUS_ADDR   0x80
#define ITE7259_STATUS_BUSY   0x01
#define ITE7259_POINT_ADDR    0xE0
#define ITE7259_POINT_SIZE    14


struct ite7259_private {
    struct gpio_callback cb;
    const struct device *i2c;
    const struct device *gpio;
    uint16_t addr;
    uint16_t pin_rst;
    uint16_t pin_int;
    uint16_t reg_len;
    uint16_t chipid;
};

#define GPIO_PIN(n) ((n) & 31)

static struct ite7259_private k_touch = {
    .addr = 0x46,
    .pin_rst = GPIO_PIN(61),
    .pin_int = GPIO_PIN(62)
};

static inline void ite7259_set_regsize(struct ite7259_private *priv, 
    uint16_t size)
{
    priv->reg_len = size;
}

static int ite7259_read(struct ite7259_private *priv,  uint16_t reg, 
        uint8_t *data, uint16_t len)
{
    struct i2c_message msg = {
        .msg = {
            .flags = I2C_MSG_READ,
            .len = len,
            .buf = data
        },
        .reg_addr = reg,
        .reg_len  = priv->reg_len
    };
	
    return i2c_transfer(priv->i2c, &msg.msg, 1, priv->addr);
}

static int __unused ite7259_write(struct ite7259_private *priv,  uint16_t reg, 
        uint8_t *data, uint16_t len)
{
    struct i2c_message msg = {
        .msg = {
            .flags = I2C_MSG_WRITE,
            .len = len,
            .buf = data
        },
        .reg_addr = reg,
        .reg_len  = priv->reg_len
    };
    
    return i2c_transfer(priv->i2c, &msg.msg, 1, priv->addr);
}

static void ite7259_reset(struct ite7259_private *priv)
{
    gpio_pin_set(priv->gpio, priv->pin_rst, 0);
    k_busy_wait(20 * 1000);
    gpio_pin_set(priv->gpio, priv->pin_rst, 1);
    k_busy_wait(200 * 1000);
}

static void ite7259_gpio_isr(const struct device *dev, struct gpio_callback *cb,
    gpio_port_pins_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(pins);
    input_event_post(INPUT_EVENT_TP);
}

static bool ite7259_data_ready(struct ite7259_private *priv)
{
    int retry = 5;
    do {
        uint8_t state = 0;
        ite7259_read(priv, ITE7259_STATUS_ADDR, &state, 1);
        if (state & 0x80)
            return true;

        if (state & ITE7259_STATUS_BUSY) {
            k_yield();
            if (--retry <= 0)
                break;
        } else {
            break;
        }
    } while (true);

    return false;
}

static int read_xy_position(void *hdl, struct input_struct *input, 
    input_report_fn report)
{
    struct ite7259_private *priv = hdl;
    uint8_t buf[ITE7259_POINT_SIZE];
    static struct input_event val;
    struct tp_event *tp;
    int ret;

    tp = &val.tp;
    if (!ite7259_data_ready(priv)) {
        if (tp->state == INPUT_STATE_PRESSED) {
            tp->state = INPUT_STATE_RELEASED;
            report(input, &val, INPUT_EVENT_TP);
        }
        return 0;
    }
    /* Read data from device */
    ret = ite7259_read(priv, ITE7259_POINT_ADDR, buf, ITE7259_POINT_SIZE);
    if (ret) {
        printk("%s: read ite7259_private failed witch error %d\n", 
            __func__, ret);
        return ret;
    }
    
    if (buf[0] == 0x09) {
        tp->x = ((uint16_t)(buf[3] & 0x0F) << 8) | buf[2];
        tp->y = ((uint16_t)(buf[3] & 0xF0) << 4) | buf[4];
        tp->state = INPUT_STATE_PRESSED;
    } else if (buf[0] == 0x80 && buf[1] > 0x1f && buf[1] < 0x43) {
        switch (buf[1]) {
        case 0x20:
        case 0x21:
        case 0x23:
            tp->x = ((uint16_t)buf[3] << 8) | buf[2];
            tp->y = ((uint16_t)buf[5] << 8) | buf[4];
            tp->state = INPUT_STATE_RELEASED;
            break;
        case 0x22:
            //last->gx_point_x = ((uint16_t)(buf[3] & 0x0F) << 8) | buf[2];
            //last->gx_point_y = ((uint16_t)(buf[3] & 0xF0) << 4) | buf[4];
            tp->x = (uint16_t)buf[6] + ((uint16_t)buf[7] << 8);
            tp->y = (uint16_t)buf[8] + ((uint16_t)buf[9] << 8);                 
            tp->state = INPUT_STATE_PRESSED;
            break;
        default:
            break;
        }
    } else {
        if (tp->state == INPUT_STATE_PRESSED)
            tp->state = INPUT_STATE_RELEASED;
        else
            return 0;
    }
    report(input, &val, INPUT_EVENT_TP);
    return 0;
}

static int ite7259_check_id(struct ite7259_private *priv)
{
    uint8_t  id[2];
    int ret;
    
    ite7259_set_regsize(priv, 2);
    ret = ite7259_read(priv, 0x7032, &id[0], 1);
    if (ret)
        goto out;

    ret = ite7259_read(priv, 0x7033, &id[1], 1);
    if (ret)
        goto out;

    priv->chipid = ((uint16_t)id[1] << 8) | id[0];
    if (priv->chipid != 0x7259)
        ret = -EINVAL;
    
out:
    ite7259_set_regsize(priv, 1);
    return ret;
}

static int ite7259_setup(struct ite7259_private *priv)
{
    int ret;

    priv->i2c = device_get_binding(BUS_DEV);
    if (!priv->i2c)
        return -ENODEV;
	
    priv->gpio = device_get_binding(GPIO_DEV);
    if (!priv->i2c)
        return -ENODEV;
	
    ret = gpio_pin_configure(priv->gpio, priv->pin_rst,
        GPIO_OUTPUT_LOW);
    if (ret)
        goto out;
	
    ite7259_reset(priv);
    ret = ite7259_check_id(priv);
    if (ret) {
        printk("Touch device id is invalid\n");
        goto out;
    }
    
    gpio_init_callback(&priv->cb, ite7259_gpio_isr, BIT(priv->pin_int));
    gpio_add_callback(priv->gpio, &priv->cb);
    ret = gpio_pin_interrupt_configure(priv->gpio, priv->pin_int, 
        GPIO_INT_EDGE_FALLING);
    if (ret)
        printk("GPIO interrupt install failed with error %d\n", ret);
out:
    return ret;
}

static const struct input_device tp_device = {
    .read = read_xy_position,
    .private_data = &k_touch
};

static int ite7259_driver_init(const struct device *dev __unused)
{
    int ret;
    ret = ite7259_setup(&k_touch);
    if (ret) {
        printk("%s touch pannel intialize failed: %d\n", __func__, ret);
        return ret;
    }			  
    return input_device_register(&tp_device, INPUT_EVENT_TP);
}

SYS_INIT(ite7259_driver_init, POST_KERNEL, 
    CONFIG_KERNEL_INIT_PRIORITY_DEVICE+5);
 