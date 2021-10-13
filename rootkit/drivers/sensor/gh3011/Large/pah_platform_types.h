/*==============================================================================
* Edit History
*
* This section contains comments describing changes made to the module. Notice
* that changes are listed in reverse chronological order. Please use ISO format
* for dates.
*
* when       who       what, where, why
* ---------- ---       -----------------------------------------------------------
* 2016-04-29 bell      Initial revision.
==============================================================================*/

#ifndef __pah_platform_types_h__
#define __pah_platform_types_h__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "kernel.h"

#define CONFIG_GH3011_THREAD_STACK_SIZE 8192

struct gh3011_data {
	const struct device *dev;
	int32_t hr_data;
	uint8_t hr_data_rdy_flag;
	int32_t spo2_data;
	uint8_t spo2_data_rdy_flag;
	struct k_thread thread;
	struct k_timer timer_motion;
	// uint8_t hrd_timer_status;
	// struct k_timer timer_spo2;
	// uint8_t spo2_timer_status;
};
#include "accelerometer.h"

#endif // header guard
