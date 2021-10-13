#include <stdint.h>
#ifndef __ALARM_LIST_CTL_H__
#define __ALARM_LIST_CTL_H__

#include "../guix_simple_resources.h"
#include "../guix_simple_specifications.h"
#include "../utils/custom_checkbox.h"
#include "../utils/custom_util.h"
#include "gx_api.h"
#include <settings/settings.h>
#include <storage/flash_map.h> // the zephyr flash_map

#define MAX_TIME_TEXT_LENGTH 2

#define ALARM_ON 0x00
#define ALARM_OFF 0xff

#define ALARM_RPT_ONCE_MASK (1 << 7)
#define ALARM_RPT_WEEK0_MASK (1 << 0)
#define ALARM_RPT_WEEK1_MASK (1 << 1)
#define ALARM_RPT_WEEK2_MASK (1 << 2)
#define ALARM_RPT_WEEK3_MASK (1 << 3)
#define ALARM_RPT_WEEK4_MASK (1 << 4)
#define ALARM_RPT_WEEK5_MASK (1 << 5)
#define ALARM_RPT_WEEK6_MASK (1 << 6)

typedef struct ALARM_INFO_STRUCT {
	UCHAR avalid; // indicate it is avalid or not. 0xaa:valid othres:invalid
	GX_CHAR alarm_hour_text[MAX_TIME_TEXT_LENGTH + 1];
	GX_CHAR alarm_minute_text[MAX_TIME_TEXT_LENGTH + 1];
	UINT alarm_am_pm;
	UCHAR alarm_repeat;
	UCHAR alarm_status;
} ALARM_INFO;

typedef struct ALARM_MANAGER_STRUCT {
	uint32_t magic_num;
	ALARM_INFO alarm_table[10];
} ALARM_MANAGER;

typedef void (*alarm_callback_fn)(void *);

void alarm_list_ctl_init(void);

void alarm_ctl_exit(void);

void alarm_list_ctl_reg_callback(alarm_callback_fn fn);

uint8_t alarm_list_ctl_get_total(uint8_t *total_cnt, uint8_t *max_cnt);

void alarm_list_ctl_get_info(uint8_t index, ALARM_INFO **info);

// status_flag: ALARM_ON or ALARM_OFF
void alarm_list_ctl_status_change(uint8_t index, UCHAR status_flag);

void alarm_list_ctl_add(ALARM_INFO new_alarm_info);

uint8_t alarm_list_ctl_del(uint8_t index);

#endif