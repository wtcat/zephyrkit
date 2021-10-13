#include "view_service_alarm.h"
#include "view_service_reminders.h"
#include "device.h"
#include "drivers/counter.h"
#include "drivers_ext/rtc.h"
#include "errno.h"
#include "posix/sys/time.h"
#include "sys/printk.h"
#include "sys/sem.h"
#include "sys/timeutil.h"
#include <posix/time.h>
#include <stdint.h>
#include "logging/log.h"
LOG_MODULE_REGISTER(view_service_rtc);
void rtc_dev_setting_update(void);

static uint32_t calc_delta_with_rtc(struct tm *tm_now, void *alarm_info_in, uint8_t *hour, uint8_t *min,
									uint8_t *weekday)
{
	ALARM_INFO_T *alarm_info = (ALARM_INFO_T *)alarm_info_in;
	uint32_t delta_min = 0;
	uint8_t hour_to_set = alarm_info->hour;
	uint8_t min_to_set = alarm_info->min;
	uint8_t weekday_to_set = 0xff;
	if (alarm_info->repeat_flags & (1 << 7)) { // once repeat!
		if ((tm_now->tm_hour < hour_to_set) || ((tm_now->tm_hour == hour_to_set) && (tm_now->tm_min < min_to_set))) {
			weekday_to_set = tm_now->tm_wday;
		} else {
			weekday_to_set = tm_now->tm_wday + 1 >= 7 ? 0 : tm_now->tm_wday + 1;
		}
	} else {
		for (uint8_t i = tm_now->tm_wday; i < 7; i++) {
			if (alarm_info->repeat_flags & (1 << i)) {
				if (tm_now->tm_wday == i) { // same weekday?
					if ((tm_now->tm_hour < hour_to_set) ||
						((tm_now->tm_hour == hour_to_set) && (tm_now->tm_min < min_to_set))) {
						weekday_to_set = tm_now->tm_wday;
						break;
					}
				} else {
					weekday_to_set = i;
					break;
				}
			}
		}
		if (weekday_to_set == 0xff) { // no find avalid alarm, continue find!
			for (uint8_t i = 0; i < tm_now->tm_wday; i++) {
				if (alarm_info->repeat_flags & (1 << i)) {
					weekday_to_set = i;
					break;
				}
			}
			// find error, only thi is possible
			if (alarm_info->repeat_flags & (1 << tm_now->tm_wday)) {
				weekday_to_set = tm_now->tm_wday;
			} else {
				LOG_ERR("[%s] error, can't find avalible alarm to set!", __FUNCTION__);
				*hour = 0xff;
				*min = 0xff;
				*weekday = 0xff;
				return 0xffffffff;
			}
		}
	}
	*hour = hour_to_set;
	*min = min_to_set;
	*weekday = weekday_to_set;
	uint8_t delta_days =
		weekday_to_set > tm_now->tm_wday ? weekday_to_set - tm_now->tm_wday : weekday_to_set + 7 - tm_now->tm_wday;
	delta_min = (delta_days - 1) * 24 * 60;
	uint8_t delta_hours = 24 - tm_now->tm_hour;
	delta_min += (delta_hours - 1) * 60;
	delta_min += 60 - tm_now->tm_min;
	delta_min += hour_to_set * 60;
	delta_min += min_to_set;
	if (delta_min > 10080) {
		delta_min -= 10080;
	}
	LOG_DBG("[%s] calc delta:%d", __FUNCTION__, delta_min);
	return delta_min;
}

static void alarm_callback(const struct device *dev, uint8_t chan_id, uint32_t ticks, void *user_data)
{
	printk("alarm happen!\n");
	rtc_dev_setting_update();
}
struct alarm_setting {
	uint8_t hour;
	uint8_t min;
	uint8_t weekday;
};
static void find_earliest_from_reminder_and_alarm(void)
{
	uint8_t hour_to_set, min_to_set, weekday_to_set;
	uint32_t min_delta = 0xffffffff;
	struct counter_alarm_wk_rpt_cfg alarm_cfg;
	ALARM_INFO_T *alarm_to_set = NULL;
	struct alarm_setting alarm_earliest;
	ALARM_INFO_T *alarm_lsit;
	uint8_t alarm_max_num;
	REMINDER_INFO_T *reminder_list;
	uint8_t reminders_max_num;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t now = tv.tv_sec;
	struct tm *tm_now = gmtime(&now);

	const struct device *rtc = device_get_binding("apollo3p_rtc");
	if (NULL == rtc) {
		printk("rtc driver binding fail!\n");
		return;
	} else {
		counter_cancel_channel_alarm(rtc, 0); // cancel alarm first!
	}

	view_service_alarm_info_get(&alarm_lsit, &alarm_max_num);
	for (uint8_t i = 0; i < alarm_max_num; i++) {
		if ((alarm_lsit[i].alarm_flag & (1 << ACTIVE_FLAG_BIT)) &&
			(alarm_lsit[i].info_avalid == ALARM_INFO_VALID_FLAG)) {
			uint32_t delta_curr =
				calc_delta_with_rtc(tm_now, &alarm_lsit[i], &hour_to_set, &min_to_set, &weekday_to_set);
			if (delta_curr <= min_delta) {
				min_delta = delta_curr;
				alarm_to_set = &alarm_lsit[i];
				alarm_earliest.hour = hour_to_set;
				alarm_earliest.min = min_to_set;
				alarm_earliest.weekday = weekday_to_set;
			}
		}
	}

	view_service_reminders_info_get(&reminder_list, &reminders_max_num);
	for (uint8_t i = 0; i < reminders_max_num; i++) {
		if ((reminder_list[i].alarm_flag & (1 << ACTIVE_FLAG_BIT)) &&
			(reminder_list[i].info_avalid == ALARM_INFO_VALID_FLAG)) {
			uint32_t delta_curr =
				calc_delta_with_rtc(tm_now, &reminder_list[i], &hour_to_set, &min_to_set, &weekday_to_set);
			if (delta_curr <= min_delta) {
				min_delta = delta_curr;
				alarm_to_set = (ALARM_INFO_T *)&reminder_list[i];
				alarm_earliest.hour = hour_to_set;
				alarm_earliest.min = min_to_set;
				alarm_earliest.weekday = weekday_to_set;
			}
		}
	}

	if ((alarm_to_set != NULL) && (alarm_earliest.weekday != 0xff)) {
		alarm_cfg.callback = alarm_callback;
		alarm_cfg.weekday = alarm_earliest.weekday;
		alarm_cfg.hour = alarm_earliest.hour;
		alarm_cfg.min = alarm_earliest.min;
		alarm_cfg.user_data = alarm_to_set;
		if (NULL != rtc) {
			counter_cancel_channel_alarm(rtc, 0);
			counter_set_alarm_wk_rpt(rtc, 0, &alarm_cfg);
		} else {
			LOG_ERR("[%s] rtc para err!", __FUNCTION__);
		}
	}
}

void rtc_dev_setting_update(void)
{
	// add more func,for example: smart awake,delay awake
	find_earliest_from_reminder_and_alarm();
}

void rtc_dev_alarm_delay_awake(ALARM_INFO_T *curr_alarming, uint8_t delay_min)
{
}