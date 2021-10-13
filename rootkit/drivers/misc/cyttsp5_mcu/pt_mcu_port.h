#ifndef __PT_MCU_PORT_H
#define __PT_MCU_PORT_H

/*
 * xx.h / xx.c
 *
 * Copyright (C) 2020 Parade Technologies
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contact Parade Technologies at www.paradetech.com <ttdrivers@paradetech.com>
 */
#if 0
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/uaccess.h>

#endif
#include "stdint.h"
#include "sys/util.h"
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef ABS
#define ABS(x) ((x) >= 0 ? (x) : (-(x)))
#endif

#ifndef ABS2
#define ABS2(x, y) ((x) >= (y) ? ((x) - (y)) : ((y) - (x)))
#endif

typedef int (*TOUCH_FUNC)(void);

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

int pt_get_irq_state(void);
int pt_set_int_gpio_mode(void);
int pt_set_rst_gpio(u8 value);
int pt_set_vdda_gpio(u8 value);
int pt_set_vddd_gpio(u8 value);
int pt_i2c_write(u8 addr, u8 *write_buf, u16 write_size);
int pt_i2c_read(u8 addr, u8 *read_buf, u16 read_size, u16 buf_size);
void pt_delay_us(u32 Us);
void pt_delay_ms(u32 Ms);
void pt_disable_irq(void);
void pt_enable_irq(void);
unsigned int pt_get_time_stamp(void);
void pt_example(void);
int pt_parse_interrupt(void);
void add_touch_handler(TOUCH_FUNC handler);
void add_touch_poller_60hz(TOUCH_FUNC handler);
void pt_pip1_cmd_send_noise_metric(void);
/* 0 - Succes; !0 - Fail*/
int pt_open_file(char *file_path, int flags);
int pt_close_file(void);
int pt_write_file(const u8 *wbuf, u32 len);
int pt_read_file(u8 *rbuf, u32 len);
int pt_file_seek(u32 offset);
u32 pt_file_size(void);
int pt_rename_file(char *old_file, char *new_file);

struct i2c_prv {
	uint16_t addr;
};

#endif
