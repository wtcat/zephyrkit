#ifndef __VIEW_SERVICE_STOP_WATCH_H__
#define __VIEW_SERVICE_STOP_WATCH_H__
#include "zephyr.h"

#define MAX_STOP_WATCH_RECORD 20

typedef enum {
	STOP_WATCH_OP_STATUS_CHG = 0,
	STOP_WATCH_OP_MARK_OR_RESUME,
} STOP_WATCH_OP_T;

typedef enum {
	STOP_WATCH_STATUS_INIT = 0,
	STOP_WATCH_STATUS_RUNNING,
	STOP_WATCH_STATUS_STOPPED,
} STOP_WATCH_STATUS_T;

typedef struct {
	STOP_WATCH_STATUS_T status;
	int64_t start_ticks;
	uint64_t ticks_before;
	uint64_t total_ticks;
	uint64_t counts_value[MAX_STOP_WATCH_RECORD];
	uint8_t counts;
} STOP_WATCH_INFO_T;

typedef struct {
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t tenth_sec;
} TIME_INFO_T;
void util_ticks_to_time(uint64_t ticks_total, TIME_INFO_T *time_info);
void view_service_stop_watch_info_sync(void);
#endif