#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>
#include "i2c/i2c-message.h"
#include "mmc5603.h"

LOG_MODULE_DECLARE(MMC5603, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT memsic_mmc5603

static const uint8_t mmc5603_i2c_slave_addr = DT_INST_REG_ADDR(0);

static int mmc5603_read_reg(struct mmc5603_data *priv, uint8_t reg_addr, 
    uint8_t *value, uint8_t len)
{
    struct i2c_message msg = {
        .msg = {
            .buf = value,
            .len = len,
            .flags = I2C_MSG_READ
        },
        .reg_addr = reg_addr,
        .reg_len = 1
    };
            
    return i2c_transfer(priv->i2c_master, &msg.msg, 1,
        mmc5603_i2c_slave_addr);
}

static int mmc5603_write_reg(struct mmc5603_data *priv, uint8_t reg_addr, 
    const uint8_t *value, uint8_t len)
{
    struct i2c_message msg = {
        .msg = {
            .buf = (uint8_t *)value,
            .len = len,
            .flags = I2C_MSG_WRITE
        },
        .reg_addr = reg_addr,
        .reg_len = 1
    };
            
    return i2c_transfer(priv->i2c_master, &msg.msg, 1,
        mmc5603_i2c_slave_addr);
}

int mmc5603_interface_init(const struct device *dev)
{
    const struct mmc5603_config *cfg = dev->config;
    struct mmc5603_data *priv = dev->data;

    priv->i2c_master = device_get_binding(cfg->i2c_name);
    if (!priv->i2c_master) {
        LOG_ERR("%s(): Not found i2c device(%s)\n", __func__, cfg->i2c_name);
        return -ENODEV;
    }

    priv->read = mmc5603_read_reg;
    priv->write = mmc5603_write_reg;
    return 0;
}

