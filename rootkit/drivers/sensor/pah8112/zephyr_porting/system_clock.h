#ifndef __ZEPHYR_SYSTEM_CLOCK_H__
#define __ZEPHYR_SYSTEM_CLOCK_H__

#include <version.h>
#include <zephyr.h>

uint32_t get_tick_count(void);
#define delay_ms(x) k_msleep(x)

#if (KERNEL_VERSION_NUMBER >= ZEPHYR_VERSION(2,6,0))
#define get_tick_count() (10 * sys_clock_tick_get_32())
#else
#define get_tick_count() (10 * z_tick_get())
#endif

#endif /* __ZEPHYR_SYSTEM_CLOCK_H__ */