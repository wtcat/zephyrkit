#ifndef __I2C_PORTING_H__
#define __I2C_PORTING_H__
#include "stdio.h"
#include "stdint.h"
#include "errno.h"
#include "device.h"
#include "i2c/i2c-message.h"
bool i2c_init(void);
uint8_t pah8112_i2c_write(size_t addr, uint8_t const *pdata, size_t size);
uint8_t pah8112_i2c_read(uint8_t addr, uint8_t *pdata, uint8_t size);
#endif