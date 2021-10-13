#ifndef __ZEPHYR_SYSTEM_CLOCK_H__
#define __ZEPHYR_SYSTEM_CLOCK_H__

#include "zephyr.h"

uint32_t get_tick_count(void);
#define delay_ms(x) k_msleep(x)

#define get_tick_count() (10 * z_tick_get())

#endif