#ifndef __ZEPHYR_PORTING_ACCELEROMETER_H__
#define __ZEPHYR_PORTING_ACCELEROMETER_H__
#include "zephyr.h"
bool accelerometer_init(void);
void accelerometer_get_fifo(int16_t **fifo, uint32_t *fifo_size);
void accelerometer_start(void);
void accelerometer_stop(void);

#endif