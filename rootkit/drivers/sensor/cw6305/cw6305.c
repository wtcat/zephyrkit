#include <errno.h>
#include <kernel.h>

#include <init.h>
#include <kernel.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <sys/__assert.h>
#include <soc.h>

#include "i2c/i2c-message.h"
#include "drivers_ext/sensor_priv.h"
#include "cw6305.h"

LOG_MODULE_REGISTER(CW6305);

struct cw6305_private {
    struct k_thread daemon;
    struct k_sem sync;
    struct k_sem mutex;
    struct gpio_callback cb;
    sensor_trigger_handler_t notify;
    const char *i2c_name;
    const struct device *i2c_master;
    const struct device *gpio;
    uint16_t pin;
    uint16_t polarity;
};

struct cw6305_cc {
    uint16_t current; /* Unit: mA */
    uint8_t iset;
};

#define DT_DRV_COMPAT cellwise_cw6305
#define CW6305_CHIP_ID 0x14
#define CW6305_INT_MASK (CHARGE_INT_CHG_DET | \
                          CHARGE_INT_CHG_REMOVE | \
                          CHARGE_INT_CHG_FINISH | \
                          CHARGE_INT_CHG_OV | \
                          CHARGE_INT_BAT_OT | \
                          CHARGE_INT_BAT_UT)

static K_THREAD_STACK_DEFINE(cw6305_stack, 2048);
static const uint16_t cw6305_i2c_slave_addr = DT_INST_REG_ADDR(0);
static const struct cw6305_cc cw6305_cc_table[] = {
    {180, 0x22},
    {200, 0x24},
    {220, 0x26},
    {250, 0x29},
    {280, 0x2c},
    {300, 0x2e},
    {320, 0x30},
};

static uint8_t cw6305_calc_cc(uint8_t temp_offset, uint8_t curr_offset)
{
    uint8_t regval;
    __ASSERT(temp_offset < ARRAY_SIZE(cw6305_cc_temperature), "");
    __ASSERT(curr_offset < ARRAY_SIZE(cw6305_cc_table), "");
    regval = temp_offset;
    regval = (regval << 6) | cw6305_cc_table[curr_offset].iset;
    return regval;
}

static  int cw6305_i2c_read(struct cw6305_private *priv, uint8_t addr, 
    uint8_t *value, uint8_t len)
{
    struct i2c_message msg = {
        .msg = {
            .buf = value,
            .len = len,
            .flags = I2C_MSG_READ
        },
        .reg_addr = addr,
        .reg_len = 1
    };
    return i2c_transfer(priv->i2c_master, &msg.msg, 1,
        cw6305_i2c_slave_addr);
}

static  int cw6305_i2c_write(struct cw6305_private *priv, uint8_t addr, 
    const uint8_t *value, uint8_t len)
{
    struct i2c_message msg = {
        .msg = {
            .buf = (void *)value,
            .len = len,
            .flags = I2C_MSG_WRITE
        },
        .reg_addr = addr,
        .reg_len = 1
    };
    return i2c_transfer(priv->i2c_master, &msg.msg, 1,
        cw6305_i2c_slave_addr);
}

static inline int cw6305_read_reg(struct cw6305_private *priv, uint8_t addr, 
    uint8_t *value)
{
    int ret;
    
    ret = cw6305_i2c_read(priv, addr, value, 1);
    if (ret)
        LOG_ERR("CW6305 read register(%x) failed: %d\n", addr, ret);
    return ret;
}

static inline int cw6305_write_reg(struct cw6305_private *priv, uint8_t addr, 
    uint8_t value)
{
    int ret;

    ret = cw6305_i2c_write(priv, addr, &value, 1);
    if (ret)
        LOG_ERR("CW6305 write register(%x) failed: %d\n", addr, ret);
    return ret;
}

static inline void cw6305_lock(struct cw6305_private *priv)
{
    k_sem_take(&priv->mutex, K_FOREVER);
}

static inline void cw6305_unlock(struct cw6305_private *priv)
{
    k_sem_give(&priv->mutex);
}

static int cw6305_read_chip_id(struct cw6305_private *priv)
{
    uint8_t reg_val;

    if (cw6305_read_reg(priv, CHG_REG_VERSION, &reg_val))
        return -EIO;
    return reg_val;
}

static int cw6305_reset_charge(struct cw6305_private *priv)
{
    uint8_t regval = 0;
    int ret = 0;

    cw6305_read_reg(priv, CHG_REG_IC_STATUS2, &regval);
    if (regval & 0x3) { 
        uint8_t safe, vol;
        ret = cw6305_read_reg(priv, CHG_REG_CHG_SAFETY, &safe);
        ret |= cw6305_read_reg(priv, CHG_REG_VBUS_VOLT, &vol);
        if (ret == 0) {
            /* Clear data */
            ret |= cw6305_write_reg(priv, CHG_REG_CHG_SAFETY, 0x00);  
            /* Disable charge */            
            ret |= cw6305_write_reg(priv, CHG_REG_VBUS_VOLT, vol & ~0x40);
            k_sleep(K_MSEC(10));
           
            /* Restore data */
            ret |= cw6305_write_reg(priv, CHG_REG_CHG_SAFETY, safe);   
            /* Enable charge */
            ret |= cw6305_write_reg(priv, CHG_REG_VBUS_VOLT, vol | 0x40);
        }
    }
    return ret;
}

static int cw6305_set_work_mode(struct cw6305_private *priv)
{
    uint8_t regval;
    int ret;
    
    /* Enable interrupt */
    ret = cw6305_write_reg(priv, CHG_REG_INT_SET, 
            CHARGE_INT_CHG_DET | CHARGE_INT_CHG_REMOVE | CHARGE_INT_CHG_FINISH);
    if (ret)
        goto _out;

    /* Clean interrupt source register */
    ret = cw6305_write_reg(priv, CHG_REG_INT_SRC, 0x00);
    if (ret)
        goto _out;

    /* 4.4V VBUS, 0x46, 4.5V ;0x4F 4.95 */
    ret = cw6305_write_reg(priv, CHG_REG_VBUS_VOLT, 0x44);
    if (ret)
        goto _out;

    /* 
     * Set VBUS input current and VOL of VSYS
     * set VBUS max current 440 
     * 4.2V
     */
    ret = cw6305_write_reg(priv, CHG_REG_VBUS_CUR, 0x0E);
    if (ret)
        goto _out;

    /* 
     * Set Battery Charge Finish Voltage,
     * Set battery stop VOl 4.2V, 
     */
    ret = cw6305_write_reg(priv, CHG_REG_CHG_VOLT, 0xB1);
    if (ret)
        goto _out;

    /*
     * Battery Fast Charging Current
     *
     * 0xE4, 120^C, 200mA
     * 0xEE, 120^C, 300mA
     * 0xF0, 120^C, 320mA
     * 0xB3, 110011 350mA
     */
    regval = cw6305_calc_cc(CONFIG_CW6305_TEMPERATURE, CONFIG_CW6305_CURRENT);
    ret = cw6305_write_reg(priv, CHG_REG_CHG_CUR1, regval);
    if (ret)
        goto _out;

    /*
     * Set pre charge current
     *
     * 0x30, terminate at 2.5% C
     * 0x33, set pre charge current 10% and terminate at 10%
     */
    ret = cw6305_write_reg(priv, CHG_REG_CHG_CUR2, 0x30);
    if (ret)
        goto _out;

    /*
     * Charge safety and NTC configuration
     * disable NTC,enable terminathion change 
     *
     * 4 safy hours 5C 9hrs, 10K NTC; 0x5A,8hrs
     *
     */
    ret = cw6305_write_reg(priv, CHG_REG_CHG_SAFETY, 0x3A);
    if (ret)
        goto _out;
     
    /*
     * 0x51, button set to entry shipping mode
     * 0x59, enable button rest, and  VSYS rest 12s latter 
     */
    ret = cw6305_write_reg(priv, CHG_REG_CONFIG, 0x11);
    if (ret)
        goto _out;

    /* Disable i2c watchdong, battery lockout at 3.0V */
    ret = cw6305_write_reg(priv, CHG_REG_SAFETY_CFG, 0x05);
    if (ret)
        goto _out;

    /* 
     * Safety timer and VSYS over current protection
     * SYS over current protection 700mA 
     */
    ret = cw6305_write_reg(priv, CHG_REG_SAFETY_CFG2, 0x86);
    if (ret)
        goto _out;
    
    ret = cw6305_reset_charge(priv);
_out:
    return ret;
}

/*
 * Use private thread but not a workqueue, 
 */
static void charge_daemon(void *arg)
{
    const struct device *dev = arg;
    struct cw6305_private *priv = dev->data;
    struct sensor_trigger trig = {0};
    uint8_t regval;
    int ret;

    for (;;) {
        ret = k_sem_take(&priv->sync, K_FOREVER);
        __ASSERT(ret == 0, "");
        cw6305_lock(priv);
        ret = cw6305_read_reg(priv, CHG_REG_INT_SRC, &regval);
        if (ret || regval == 0) {
            cw6305_unlock(priv);
            continue;
        }
        regval &= CW6305_INT_MASK;
        if (regval & CHARGE_INT_CHG_DET) {
            cw6305_reset_charge(priv);
            trig.type = SENSOR_TRIG_CHARGE_IN;
            priv->notify(dev, &trig);
        }
        if (regval & CHARGE_INT_CHG_REMOVE) {
            trig.type = SENSOR_TRIG_CHARGE_OUT;
            priv->notify(dev, &trig);
        }
        if (regval & CHARGE_INT_CHG_FINISH) {
            trig.type = SENSOR_TRIG_CHARGE_FINISH;
            priv->notify(dev, &trig);
        }
        if (regval & CHARGE_INT_CHG_OV) {
            trig.type = SENSOR_TRIG_CHARGE_OV;
            priv->notify(dev, &trig);
        }
        if (regval & CHARGE_INT_BAT_OT) {
            trig.type = SENSOR_TRIG_CHARGE_OT;
            priv->notify(dev, &trig);
        }
        if (regval & CHARGE_INT_BAT_UT) {
            trig.type = SENSOR_TRIG_CHARGE_UT;
            priv->notify(dev, &trig);
        }
        /* Clear interrupt */
        cw6305_write_reg(priv, CHG_REG_INT_SRC, 0);
        cw6305_unlock(priv);
    }
}

static void cw6305_interrupt_process(const struct device *dev, 
	struct gpio_callback *cb, gpio_port_pins_t pins)
{
    struct cw6305_private *priv = CONTAINER_OF(cb, struct cw6305_private, cb);
    int value;

    value = gpio_pin_get(priv->gpio, pin2gpio(priv->pin));
    if (value == priv->polarity)
        k_sem_give(&priv->sync);
    ARG_UNUSED(dev);
    ARG_UNUSED(pins);
}   

static void cw6305_null_trigger(const struct device *dev,
    struct sensor_trigger *trigger)
{
}

static int cw6305_sample_fetch(const struct device *dev,
    enum sensor_channel chan)
{
    return -ENOTSUP;
}

static int cw6305_channel_get(const struct device *dev,
    enum sensor_channel chan, struct sensor_value *val)
{
    return -ENOTSUP;
}

static int cw6305_trigger_set(const struct device *dev,
                               const struct sensor_trigger *trig,
                               sensor_trigger_handler_t handler)
{
    struct cw6305_private *priv = dev->data;
    priv->notify = handler;
    return 0;
}

static int cw6305_attr_set(const struct device *dev,
            				 enum sensor_channel chan,
            				 enum sensor_attribute attr,
            				 const struct sensor_value *val)
{
    struct cw6305_private *priv = dev->data;
    int ret = 0;
    uint8_t regval;

    cw6305_lock(priv);
    switch ((int)attr) {
    case SENSOR_ATTR_SET_SHIPPING_MODE:
        ret |= cw6305_read_reg(priv, CHG_REG_CONFIG, &regval);
        regval |= BIT(5) | BIT(7);
        ret |= cw6305_write_reg(priv, CHG_REG_CONFIG, regval);
        break;
    case SENSOR_ATTR_SET_CHARGE_EN:
        ret |= cw6305_read_reg(priv, CHG_REG_VBUS_VOLT, &regval);
        if (!!val->val1)
            regval |= BIT(6);
        else
            regval &= ~BIT(6);
        ret |= cw6305_write_reg(priv, CHG_REG_VBUS_VOLT, regval);
        break;
    case SENSOR_ATTR_SET_VSYS_VOLTAGE:
        ret |= cw6305_read_reg(priv, CHG_REG_VBUS_CUR, &regval);
        regval &= 0x0F;
        if (val->val1 == 42)    //4.2V
            regval |= 0x00;
        else if (val->val1 == 44)  //4.4V
            regval |= 0x40;
        else if (val->val1 == 46)  //4.75
            regval |= 0x80;
        else if (val->val1 >= 47)  //4.75V
            regval |= 0xB0;
        ret |= cw6305_write_reg(priv, CHG_REG_VBUS_CUR, regval);
        break;
    default:
        return -ENOTSUP;
    }
    cw6305_unlock(priv);
    return ret;
}

static const struct sensor_driver_api cw6305_driver = {
    .attr_set = cw6305_attr_set,
    .trigger_set = cw6305_trigger_set,
    .sample_fetch = cw6305_sample_fetch,
    .channel_get = cw6305_channel_get,
};

static int cw6305_init(const struct device *dev)
{
    struct cw6305_private *priv = dev->data;
    k_tid_t tid;
    int ret;

    cw6305_lock(priv);
    priv->i2c_master = device_get_binding(priv->i2c_name);
    if (!priv->i2c_master) {
        LOG_ERR("Device(%s) is not found\n", priv->i2c_name);
        ret = -EINVAL;
        goto _exit;
    }
    ret = cw6305_read_chip_id(priv);
    if (ret < 0 || ret != CW6305_CHIP_ID) {
        LOG_ERR("CW6305 chip id is invalid\n");
        goto _exit;
    }

	tid = k_thread_create(&priv->daemon, 
            cw6305_stack, K_KERNEL_STACK_SIZEOF(cw6305_stack),
    		(k_thread_entry_t)charge_daemon, (void *)dev, NULL, NULL,
    		CONFIG_SYSTEM_WORKQUEUE_PRIORITY+1, 0, K_NO_WAIT);
    k_thread_name_set(tid, "/sensor@charge");
    gpio_init_callback(&priv->cb, 
                          cw6305_interrupt_process,
                          BIT(pin2gpio(priv->pin)));
    gpio_add_callback(priv->gpio, &priv->cb);
    ret = gpio_pin_interrupt_configure(priv->gpio, priv->pin, 
        GPIO_INT_EDGE_BOTH|GPIO_INPUT|GPIO_PULL_UP);
    if (ret) {
        LOG_ERR("CW6305 configure gpio interrupt failed\n");
        goto _exit;
    }
    ret = cw6305_set_work_mode(priv);
_exit:
    cw6305_unlock(priv);
    return ret;
}

 #define CW6305_INIT(nid) \
    static struct cw6305_private cw6305_data##nid = { \
        .sync = Z_SEM_INITIALIZER(cw6305_data##nid.sync, 0, 1), \
        .mutex = Z_SEM_INITIALIZER(cw6305_data##nid.mutex, 1, 1), \
        .notify = cw6305_null_trigger, \
        .i2c_name = DT_PROP(DT_PARENT(DT_DRV_INST(nid)), label), \
        .gpio = DEVICE_DT_GET(DT_GPIO_CTLR(DT_DRV_INST(nid), int_gpios)), \
        .pin = DT_PHA(DT_DRV_INST(nid), int_gpios, pin), \
        .polarity = !DT_PHA(DT_DRV_INST(nid), int_gpios, flags) \
    }; \
    DEVICE_DT_DEFINE(DT_DRV_INST(nid),   \
        cw6305_init,              \
        device_pm_control_nop,          \
        &cw6305_data##nid,                   \
        NULL,           \
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
        &cw6305_driver);

DT_INST_FOREACH_STATUS_OKAY(CW6305_INIT)
