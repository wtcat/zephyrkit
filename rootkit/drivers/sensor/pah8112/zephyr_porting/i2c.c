#include "i2c.h"

#define BUS_DEV "I2C_2"
#define SLAVE_ADDR 0x15

const struct device *i2c_dev = NULL;
bool i2c_init(void)
{
	i2c_dev = device_get_binding(BUS_DEV);
	if (i2c_dev == NULL) {
		return false;
	}
	return true;
}

uint8_t pah8112_i2c_write(size_t addr, uint8_t const *pdata, size_t size)
{
	if (i2c_dev != NULL) {
		struct i2c_message msg = {
			.msg =
				{
					.flags = I2C_MSG_WRITE,
					.len = size,
					.buf = (uint8_t *)pdata,
				},
			.reg_addr = addr,
			.reg_len = 1,
		};

		return i2c_transfer(i2c_dev, &msg.msg, 1, SLAVE_ADDR);
	} else {
		return -1;
	}
}

uint8_t pah8112_i2c_read(uint8_t addr, uint8_t *pdata, uint8_t size)
{
	if (i2c_dev != NULL) {
		struct i2c_message msg = {
			.msg =
				{
					.flags = I2C_MSG_READ,
					.len = size,
					.buf = pdata,
				},
			.reg_addr = addr,
			.reg_len = 1,
		};

		return i2c_transfer(i2c_dev, &msg.msg, 1, SLAVE_ADDR);
	} else {
		return -1;
	}
}