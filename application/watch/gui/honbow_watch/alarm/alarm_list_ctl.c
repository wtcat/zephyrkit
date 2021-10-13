#include "./alarm_list_ctl.h"
#include "device.h"
#include "drivers/counter.h"
#include "drivers_ext/rtc.h"
#include "errno.h"
#if 0
#include "gx_button.h"
#include "gx_prompt.h"
#include "gx_widget.h"
#include "gx_window.h"
#else
#include "gx_api.h"
#endif
#include "posix/sys/time.h"
#include "sys/printk.h"
#include "sys/sem.h"
#include "sys/timeutil.h"
#include <posix/time.h>
#include <stdint.h>

#define ALARM_TABLE_MAX_CNT 10

static uint8_t alarm_list_total_cnts;

static uint8_t init_flag = 0;

static alarm_callback_fn callback_fn = NULL;

struct alarm_setting {
	uint8_t hour;
	uint8_t min;
	uint8_t weekday;
};

static void alarm_list_ctl_alarm_update(void);
static void alarms_load_from_ext_flash(void);

#if defined(CONFIG_MULTITHREADING)
/* semaphore for locking flash resources (tickers) */
static struct sys_sem sem_lock;
#define SYNC_INIT() sys_sem_init(&sem_lock, 1, 1)
#define SYNC_LOCK() sys_sem_take(&sem_lock, K_FOREVER)
#define SYNC_UNLOCK() sys_sem_give(&sem_lock)
#else
#define SYNC_INIT()
#define SYNC_LOCK()
#define SYNC_UNLOCK()
#endif

ALARM_MANAGER alarm_manager = {
	.alarm_table =
		{
			{0xaa, "11", "03", GX_STRING_ID_AM, 0x7f, ALARM_ON},
			{0xff, "11", "02", GX_STRING_ID_AM, 0x7f, ALARM_ON},
			{0xff, "33", "33", GX_STRING_ID_AM, 0x7f, ALARM_OFF},
			{0xff, "44", "44", GX_STRING_ID_AM, 0x0f, ALARM_OFF},
			{0xff, "55", "55", GX_STRING_ID_AM, 0x0f, ALARM_OFF},
			{0xff, "66", "66", GX_STRING_ID_AM, 0x0f, ALARM_OFF},
			{0xff, "77", "77", GX_STRING_ID_AM, 0x0f, ALARM_OFF},
			{0xff, "88", "88", GX_STRING_ID_AM, 0x0f, ALARM_OFF},
			{0xff, "99", "99", GX_STRING_ID_AM, 0x0f, ALARM_OFF},
			{0xff, "00", "00", GX_STRING_ID_AM, 0x0f, ALARM_OFF},
		},
	.magic_num = 0x7682953A,
};

static int alarms_settings_set(const char *name, size_t len,
							   settings_read_cb read_cb, void *cb_arg)
{

	const char *next;
	int rc;

	if (settings_name_steq(name, "alarms", &next) && !next) {
		if (len != sizeof(ALARM_MANAGER)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, (void *)&alarm_manager, sizeof(ALARM_MANAGER));
		if (rc >= 0) {
			return 0;
		}

		return rc;
	}

	return -ENOENT;
}

static struct settings_handler alarms_conf = {.name = "setting/alarm",
											  .h_set = alarms_settings_set};

// find earliest alarm and return delta value with now.
static uint32_t find_earliest_alarm(struct tm *tm_now, ALARM_INFO *alarm_info,
									uint8_t *hour, uint8_t *min,
									uint8_t *weekday)
{

	uint32_t delata_min = 0;
	uint8_t hour_to_set = atoi(alarm_info->alarm_hour_text);
	uint8_t min_to_set = atoi(alarm_info->alarm_minute_text);
	uint8_t weekday_to_set = 0xff;
	if (alarm_info->alarm_repeat & ALARM_RPT_ONCE_MASK) { // once repeat!
		if ((tm_now->tm_hour < hour_to_set) ||
			((tm_now->tm_hour == hour_to_set) &&
			 (tm_now->tm_min < min_to_set))) {
			weekday_to_set = tm_now->tm_wday;
		} else {
			weekday_to_set = tm_now->tm_wday + 1 >= 7 ? 0 : tm_now->tm_wday + 1;
		}
	} else {
		// find sequence 1
		for (uint8_t i = tm_now->tm_wday; i < 7; i++) {
			if (alarm_info->alarm_repeat & (1 << i)) {
				if (tm_now->tm_wday == i) { // same weekday?
					if ((tm_now->tm_hour < hour_to_set) ||
						((tm_now->tm_hour == hour_to_set) &&
						 (tm_now->tm_min < min_to_set))) {
						weekday_to_set = tm_now->tm_wday;
						break;
					}
				} else {
					weekday_to_set = i;
					break;
				}
			}
		}
		// find sequence 2
		if (weekday_to_set == 0xff) { // no find avalid alarm, continue find!
			for (uint8_t i = 0; i < tm_now->tm_wday; i++) {
				if (alarm_info->alarm_repeat & (1 << i)) {
					weekday_to_set = i;
					break;
				}
			}
		}
	}

	*hour = hour_to_set;
	*min = min_to_set;
	*weekday = weekday_to_set;

	// delta calc
	uint8_t delat_days = weekday_to_set > tm_now->tm_wday
							 ? weekday_to_set - tm_now->tm_wday
							 : weekday_to_set + 7 - tm_now->tm_wday;

	delata_min = (delat_days - 1) * 24 * 60;

	uint8_t delta_hours = 24 - tm_now->tm_hour;

	delata_min += (delta_hours - 1) * 60;

	delata_min += 60 - tm_now->tm_min;

	delata_min += hour_to_set * 60;

	delata_min += min_to_set;

	if (delata_min >= 10080) {
		delata_min -= 10080;
	}

	return delata_min;
}

static void alarm_callback(const struct device *dev, uint8_t chan_id,
						   uint32_t ticks, void *user_data)
{

	if (callback_fn != NULL) {
		callback_fn(user_data);
	}

	alarm_list_ctl_alarm_update();
}

static void alarm_list_ctl_alarm_update(void)
{

	ALARM_INFO *alarm_to_set = NULL;
	struct alarm_setting alarm_earliest;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	time_t now = tv.tv_sec;
	struct tm *tm_now = gmtime(&now);

	alarm_earliest.weekday = 0xff;

	UCHAR total_cnt, max_cnt;
	alarm_list_ctl_get_total(&total_cnt, &max_cnt);

	SYNC_LOCK();
	uint32_t delta = 0xffffffff;
	for (uint8_t i = 0; i < total_cnt; i++) {
		if (alarm_manager.alarm_table[i].alarm_status == ALARM_ON) {
			uint8_t hour, min, weekday;
			uint32_t delta_curr = find_earliest_alarm(
				tm_now, &alarm_manager.alarm_table[i], &hour, &min, &weekday);
			if (delta_curr <= delta) {
				delta = delta_curr;
				alarm_to_set = &alarm_manager.alarm_table[i];
				// store alarm to set
				alarm_earliest.hour = hour;
				alarm_earliest.min = min;
				alarm_earliest.weekday = weekday;
			}
		}
	}
	SYNC_UNLOCK();

	// set alarm to real clock device
	const struct device *rtc = device_get_binding("apollo3p_rtc");
	if (NULL == rtc) {
		printk("rtc driver binding fail!\n");
		return;
	}
	counter_cancel_channel_alarm(rtc, 0); // cancel alarm first!
	if (alarm_to_set != NULL) {			  // indicate find earliest alarm
		struct counter_alarm_wk_rpt_cfg alarm_cfg;
		alarm_cfg.callback = alarm_callback;
		alarm_cfg.weekday = alarm_earliest.weekday;
		alarm_cfg.hour = alarm_earliest.hour;
		alarm_cfg.min = alarm_earliest.min;
		alarm_cfg.user_data = alarm_to_set;

		counter_cancel_channel_alarm(rtc, 0);
		counter_set_alarm_wk_rpt(rtc, 0, &alarm_cfg);
	}
}

void alarm_list_ctl_init(void)
{

	SYNC_INIT();

	SYNC_LOCK();

	alarms_load_from_ext_flash();

	alarm_list_total_cnts = 0;

	for (uint8_t i = 0; i < ALARM_TABLE_MAX_CNT; i++) {
		if (alarm_manager.alarm_table[i].avalid != 0xaa) {
			break;
		} else {
			alarm_list_total_cnts += 1;
		}
	}

	init_flag = 0xaa;

	SYNC_UNLOCK();

	alarm_list_ctl_alarm_update();

	return;
}

void alarm_list_ctl_reg_callback(alarm_callback_fn fn)
{
	callback_fn = fn;
}

void alarm_ctl_exit(void)
{
#if CONFIG_SETTINGS
	settings_save_one("setting/alarm/alarms", (void *)&alarm_manager,
					  sizeof(ALARM_MANAGER));
#endif
	return;
}

uint8_t alarm_list_ctl_get_total(uint8_t *total_cnt, uint8_t *max_cnt)
{

	if (0xaa != init_flag) {
		alarm_list_ctl_init();
	}

	SYNC_LOCK();

	if (total_cnt != NULL) {
		*total_cnt = alarm_list_total_cnts;
	}

	if (max_cnt != NULL) {
		*max_cnt = ALARM_TABLE_MAX_CNT;
	}

	SYNC_UNLOCK();

	return 0;
}

void alarm_list_ctl_get_info(uint8_t index, ALARM_INFO **info)
{

	if (0xaa != init_flag) {
		alarm_list_ctl_init();
	}

	SYNC_LOCK();

	if (alarm_list_total_cnts == 0) {
		*info = NULL;
	}

	if (index > alarm_list_total_cnts - 1) {
		*info = NULL;
	} else {
		*info = &alarm_manager.alarm_table[index];
	}

	SYNC_UNLOCK();

	return;
}

void alarm_list_ctl_status_change(uint8_t index, UCHAR status_flag)
{

	if (0xaa != init_flag) {
		alarm_list_ctl_init();
	}

	SYNC_LOCK();

	if ((alarm_list_total_cnts > 0) && (index <= alarm_list_total_cnts - 1)) {
		alarm_manager.alarm_table[index].alarm_status = status_flag;
	}

	SYNC_UNLOCK();

	alarm_list_ctl_alarm_update();
#if CONFIG_SETTINGS
	settings_save_one("setting/alarm/alarms", (void *)&alarm_manager,
					  sizeof(ALARM_MANAGER));
#endif
}

uint8_t alarm_list_ctl_del(uint8_t index)
{

	if (0xaa != init_flag) {
		alarm_list_ctl_init();
	}

	SYNC_LOCK();

	if (alarm_list_total_cnts == 0) {
		SYNC_UNLOCK();
		return -ENOTSUP;
	}

	if (index >= alarm_list_total_cnts) {
		SYNC_UNLOCK();
		return -ENOTSUP;
	}

	for (uint8_t i = index; i < alarm_list_total_cnts - 1; i++) {
		alarm_manager.alarm_table[i] = alarm_manager.alarm_table[i + 1];
	}

	alarm_manager.alarm_table[alarm_list_total_cnts - 1].avalid = 0xff;

	alarm_list_total_cnts -= 1;

	SYNC_UNLOCK();

	alarm_list_ctl_alarm_update();
#if CONFIG_SETTINGS
	settings_save_one("setting/alarm/alarms", (void *)&alarm_manager,
					  sizeof(ALARM_MANAGER));
#endif
	return 0;
}

void alarm_list_ctl_add(ALARM_INFO new_alarm_info)
{

	if (0xaa != init_flag) {
		alarm_list_ctl_init();
	}

	SYNC_LOCK();

	if (alarm_list_total_cnts >= ALARM_TABLE_MAX_CNT) {
		SYNC_UNLOCK();
		return;
	}

	alarm_manager.alarm_table[alarm_list_total_cnts] = new_alarm_info;

	alarm_list_total_cnts += 1;

	SYNC_UNLOCK();

	alarm_list_ctl_alarm_update();
#if CONFIG_SETTINGS
	settings_save_one("setting/alarm/alarms", (void *)&alarm_manager,
					  sizeof(ALARM_MANAGER));
#endif
	return;
}

static void alarms_load_from_ext_flash(void)
{
#if CONFIG_SETTINGS
	int ret = settings_subsys_init();
	if (ret != 0) {
		int rc;
		const struct flash_area *_fa_p_ext_flash;
		rc = flash_area_open(FLASH_AREA_ID(storage), &_fa_p_ext_flash);
		if (!rc) {
			if (!flash_area_erase(_fa_p_ext_flash, 0,
								  _fa_p_ext_flash->fa_size)) {
				settings_subsys_init();
			}
		}
	}
	settings_register(&alarms_conf);
	settings_load();
#endif
}
