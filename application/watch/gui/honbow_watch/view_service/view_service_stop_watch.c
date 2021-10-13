#include "view_service_custom.h"
#include <logging/log.h>
#include "data_service_custom.h"
#include "setting_service_custom.h"
#include "power/reboot.h"
#include "view_service_stop_watch.h"
#include "zephyr.h"
LOG_MODULE_DECLARE(guix_view_service);

static STOP_WATCH_INFO_T stop_watch_info;
static struct k_timer sync_timer;

void util_ticks_to_time(uint64_t ticks_total, TIME_INFO_T *time_info)
{
	uint64_t sec_total = ticks_total / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	if (time_info != NULL) {
		time_info->hour = sec_total / 3600;
		time_info->min = sec_total % 3600 / 60;
		time_info->sec = sec_total % 60;
		time_info->tenth_sec = ticks_total % CONFIG_SYS_CLOCK_TICKS_PER_SEC * 10 / 100;
	}
}

static void view_service_stop_watch_op_handle(VIEW_EVENT_TYPE type, void *data)
{
	switch (*(uint8_t *)data) {
	case STOP_WATCH_OP_STATUS_CHG:
		if ((stop_watch_info.status == STOP_WATCH_STATUS_STOPPED) ||
			(stop_watch_info.status == STOP_WATCH_STATUS_INIT)) {
			stop_watch_info.status = STOP_WATCH_STATUS_RUNNING;
			stop_watch_info.start_ticks = sys_clock_tick_get();
			if (stop_watch_info.status == STOP_WATCH_STATUS_INIT) {
				stop_watch_info.counts = 0;
				stop_watch_info.ticks_before = 0;
			}
			k_timer_start(&sync_timer, K_MSEC(0), K_MSEC(100));
		} else if (stop_watch_info.status == STOP_WATCH_STATUS_RUNNING) {
			k_timer_status_sync(&sync_timer);
			k_timer_stop(&sync_timer);
			stop_watch_info.status = STOP_WATCH_STATUS_STOPPED;
			stop_watch_info.ticks_before = sys_clock_tick_get() - stop_watch_info.start_ticks + stop_watch_info.ticks_before;
		} else {
			LOG_ERR("wrong status!");
			return;
		}
		break;
	case STOP_WATCH_OP_MARK_OR_RESUME:
		if (stop_watch_info.status == STOP_WATCH_STATUS_INIT) {
			LOG_INF("no use if in init status!");
		} else if (stop_watch_info.status == STOP_WATCH_STATUS_STOPPED) {
			k_timer_status_sync(&sync_timer);
			k_timer_stop(&sync_timer);
			stop_watch_info.counts = 0;
			stop_watch_info.total_ticks = 0;
			stop_watch_info.status = STOP_WATCH_STATUS_INIT;
			stop_watch_info.ticks_before = 0;
		} else if (stop_watch_info.status == STOP_WATCH_STATUS_RUNNING) {
			if (stop_watch_info.counts < MAX_STOP_WATCH_RECORD) {
				stop_watch_info.counts_value[stop_watch_info.counts] = stop_watch_info.total_ticks;
				stop_watch_info.counts++;
			} else {
				LOG_INF("max record cnt is %d!", MAX_STOP_WATCH_RECORD);
			}
		}
		break;
	default:
		return;
	}
	void *info = &stop_watch_info;
	data_service_event_submit(DATA_SERVICE_EVT_STOP_WATCH_INFO_CHG, &info, 4);
}

static void stop_watch_sync_timer(struct k_timer *timer)
{
	if (stop_watch_info.status == STOP_WATCH_STATUS_RUNNING) {
		stop_watch_info.total_ticks = stop_watch_info.ticks_before + sys_clock_tick_get() - stop_watch_info.start_ticks;
		void *info = &stop_watch_info;
		data_service_event_submit(DATA_SERVICE_EVT_STOP_WATCH_INFO_CHG, &info, 4);
	}
}
void view_service_stop_watch_init(void)
{
	stop_watch_info.counts = 0;
	stop_watch_info.status = STOP_WATCH_STATUS_INIT;
	stop_watch_info.ticks_before = 0;
	k_timer_init(&sync_timer, stop_watch_sync_timer, NULL);
	view_service_callback_reg(VIEW_EVENT_TYPE_STOP_WATCH_OP, view_service_stop_watch_op_handle);
}

void view_service_stop_watch_info_sync(void)
{
	void *info = &stop_watch_info;
	data_service_event_submit(DATA_SERVICE_EVT_STOP_WATCH_INFO_CHG, &info, 4);
}