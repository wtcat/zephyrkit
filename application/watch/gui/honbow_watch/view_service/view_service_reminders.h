#ifndef __VIEW_SERVICE_REMINDERS_H__
#define __VIEW_SERVICE_REMINDERS_H__

#include "gx_api.h"
#include <settings/settings.h>
#include <storage/flash_map.h> // the zephyr flash_map

#define REMINDERS_INFO_VALID_FLAG (0xAA)
#define REMINDERS_INFO_SIZE 9

#define ACTIVE_FLAG_BIT 0
#define REMIND_LATER_FALG_BIT 1
#define SMART_AWAKE_FLAG_BIT 2
typedef struct {
	uint8_t info_avalid;  // 0xaa: avalid others:no
	uint8_t hour;		  // hour: 0~23
	uint8_t min;		  // min: 0~59
	uint8_t repeat_flags; // bit0:sunday bit1:monday.......bit6:saturday
	uint8_t alarm_flag;	  // bit0: active flasg bit1:remind later flag bit2:smart awake flag
	uint8_t vib_duration;
	GX_CHAR desc[20]; // desc abbout alarm
} REMINDER_INFO_T;

void view_service_reminders_info_get(REMINDER_INFO_T **alarm_info, uint8_t *info_num);

#endif