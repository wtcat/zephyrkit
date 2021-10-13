#include "gh3011_i2c.h"

#define BUS_DEV "I2C_2"
#define SLAVE_ADDR 0x14

const struct device *i2c_dev = NULL;
bool i2c_init(void)
{
	i2c_dev = device_get_binding(BUS_DEV);
	if (i2c_dev == NULL) {
		return false;
	}
	return true;
}

uint8_t gh3011_i2c_write(uint8_t device_id, const uint8_t *write_buffer, uint16_t length)
{
	if (i2c_dev != NULL) {
		
			struct i2c_message msg = {
			.msg =
				{
					.flags = I2C_MSG_WRITE,
					.len = length-2,
					.buf = (uint8_t *)(write_buffer + 2),
				},
			.reg_addr = (write_buffer[0] << 8) + write_buffer[1],
			.reg_len = 2,
			};
			return i2c_transfer(i2c_dev, &msg.msg, 1, SLAVE_ADDR);
		
	} else {
		return -1;
	}
}

uint8_t gh3011_i2c_read(uint8_t device_id, const uint8_t *write_buffer, uint16_t write_length, uint8_t *read_buffer, uint16_t read_length)
{
	if (i2c_dev != NULL) {
		
			struct i2c_message msg = {
			.msg =
				{
					.flags = I2C_MSG_READ,
					.len = read_length ,
					.buf = (uint8_t *)(read_buffer),
				},
			.reg_addr = (write_buffer[0] << 8) + write_buffer[1],
			.reg_len = 2,
			};
			return i2c_transfer(i2c_dev, &msg.msg, 1, SLAVE_ADDR);
		
	} else {
		return -1;
	}
}