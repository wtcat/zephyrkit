#ifndef DRIVER_APOLLO3P_I2C_MESSAGE_H_
#define DRIVER_APOLLO3P_I2C_MESSAGE_H_

#include <drivers/i2c.h>

#ifdef __cplusplus
extern "C"{
#endif

struct i2c_message {
    struct i2c_msg msg;
    uint16_t reg_addr;
    uint16_t reg_len;
};

#ifdef __cplusplus
}
#endif
#endif /* DRIVER_APOLLO3P_I2C_MESSAGE_H_*/
