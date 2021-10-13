#include "view_service_custom.h"
#include <logging/log.h>
#include "data_service_custom.h"
#include "setting_service_custom.h"
#include "power/reboot.h"
#include "view_service_alarm.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "view_service_rtc_setting.h"
LOG_MODULE_DECLARE(guix_view_service);

static ALARM_INFO_T alarm_info_list[ALARM_INFO_SIZE];
static int alarms_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int rc;
	if (settings_name_steq(name, "alarms", &next) && !next) {
		if (len != sizeof(ALARM_INFO_SIZE * sizeof(ALARM_INFO_T))) {
			return -EINVAL;
		}
		rc = read_cb(cb_arg, (void *)&alarm_info_list[0], len);
		if (rc >= 0) {
			return 0;
		}
		return rc;
	}
	return -ENOENT;
}
static struct settings_handler alarms_conf = {.name = "setting/alarm", .h_set = alarms_settings_set};
static void view_service_alarm_add_handle(VIEW_EVENT_TYPE type, void *data)
{
	LOG_INF("[%s] alarm add!", __FUNCTION__);
	ALARM_INFO_T *info_new = (ALARM_INFO_T *)(*(uint32_t *)data);
	// tell ui, alarm info have changed!
	for (uint8_t i = 0; i < ALARM_INFO_SIZE; i++) {
		if (alarm_info_list[i].info_avalid != ALARM_INFO_VALID_FLAG) {
			alarm_info_list[i].alarm_flag = info_new->alarm_flag;
			strncpy(alarm_info_list[i].desc, info_new->desc, sizeof(alarm_info_list[i].desc));
			alarm_info_list[i].hour = info_new->hour;
			alarm_info_list[i].min = info_new->min;
			alarm_info_list[i].info_avalid = info_new->info_avalid;
			alarm_info_list[i].repeat_flags = info_new->repeat_flags;
			alarm_info_list[i].vib_duration = info_new->vib_duration;
			break;
		}
	}
	data_service_event_submit(DATA_SERVICE_EVT_ALARM_INFO_CHG, NULL, 0);
	rtc_dev_setting_update();
}
static void view_service_alarm_del_handle(VIEW_EVENT_TYPE type, void *data)
{
	LOG_INF("[%s] alarm del!", __FUNCTION__);
	// tell ui, alarm info have changed!
	ALARM_INFO_T *info_del = (ALARM_INFO_T *)(*(uint32_t *)data);
	for (uint8_t i = 0; i < ALARM_INFO_SIZE; i++) {
		if (&alarm_info_list[i] == info_del) {
			alarm_info_list[i].info_avalid = 0;
			break;
		}
	}
	data_service_event_submit(DATA_SERVICE_EVT_ALARM_INFO_CHG, NULL, 0);
	rtc_dev_setting_update();
}

static void view_service_alarm_edit_handle(VIEW_EVENT_TYPE type, void *data)
{
	LOG_INF("[%s] alarm edit!", __FUNCTION__);
	// data_service_event_submit(DATA_SERVICE_EVT_ALARM_INFO_CHG, NULL, 0);
	rtc_dev_setting_update();
}

void view_service_alarm_info_get(ALARM_INFO_T **alarm_info, uint8_t *info_max_num)
{
	*alarm_info = alarm_info_list;
	*info_max_num = ALARM_INFO_SIZE;
}

void view_service_alarm_init(void)
{
	int ret = settings_subsys_init();
	if (ret != 0) {
		int rc;
		const struct flash_area *_fa_p_ext_flash;
		rc = flash_area_open(FLASH_AREA_ID(storage), &_fa_p_ext_flash);
		if (!rc) {
			if (!flash_area_erase(_fa_p_ext_flash, 0, _fa_p_ext_flash->fa_size)) {
				settings_subsys_init();
			}
		}
	}
	settings_register(&alarms_conf);
	settings_load();

	view_service_callback_reg(VIEW_EVENT_TYPE_ALARM_ADD, view_service_alarm_add_handle);
	view_service_callback_reg(VIEW_EVENT_TYPE_ALARM_DEL, view_service_alarm_del_handle);
	view_service_callback_reg(VIEW_EVENT_TYPE_ALARM_EDITED, view_service_alarm_edit_handle);
	rtc_dev_setting_update();
}