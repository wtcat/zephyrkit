#include "view_service_custom.h"
#include <logging/log.h>
#include "data_service_custom.h"
#include "setting_service_custom.h"
#include "power/reboot.h"
#include "view_service_reminders.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "view_service_rtc_setting.h"
LOG_MODULE_DECLARE(guix_view_service);

static REMINDER_INFO_T reminders_info_list[REMINDERS_INFO_SIZE];
static int reminders_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int rc;
	if (settings_name_steq(name, "reminders", &next) && !next) {
		if (len != sizeof(REMINDERS_INFO_SIZE * sizeof(REMINDER_INFO_T))) {
			return -EINVAL;
		}
		rc = read_cb(cb_arg, (void *)&reminders_info_list[0], len);
		if (rc >= 0) {
			return 0;
		}
		return rc;
	}
	return -ENOENT;
}
static struct settings_handler reminders_conf = {.name = "setting/reminders", .h_set = reminders_settings_set};

static void view_service_reminders_add_handle(VIEW_EVENT_TYPE type, void *data)
{
	LOG_INF("[%s] reminder add!", __FUNCTION__);
	REMINDER_INFO_T *info_new = (REMINDER_INFO_T *)(*(uint32_t *)data);
	// tell ui, alarm info have changed!
	for (uint8_t i = 0; i < REMINDERS_INFO_SIZE; i++) {
		if (reminders_info_list[i].info_avalid != REMINDERS_INFO_VALID_FLAG) {
			reminders_info_list[i].alarm_flag = info_new->alarm_flag;
			strncpy(reminders_info_list[i].desc, info_new->desc, sizeof(reminders_info_list[i].desc));
			reminders_info_list[i].hour = info_new->hour;
			reminders_info_list[i].min = info_new->min;
			reminders_info_list[i].info_avalid = info_new->info_avalid;
			reminders_info_list[i].repeat_flags = info_new->repeat_flags;
			reminders_info_list[i].vib_duration = info_new->vib_duration;
			break;
		}
	}
	data_service_event_submit(DATA_SERVICE_EVT_REMINDERS_INFO_CHG, NULL, 0);
}
static void view_service_reminders_del_handle(VIEW_EVENT_TYPE type, void *data)
{
	LOG_INF("[%s] reminder del!", __FUNCTION__);
	// tell ui, alarm info have changed!
	REMINDER_INFO_T *info_del = (REMINDER_INFO_T *)(*(uint32_t *)data);
	for (uint8_t i = 0; i < REMINDERS_INFO_SIZE; i++) {
		if (&reminders_info_list[i] == info_del) {
			reminders_info_list[i].info_avalid = 0;
			data_service_event_submit(DATA_SERVICE_EVT_REMINDERS_INFO_CHG, NULL, 0);
			break;
		}
	}
}

static void view_service_reminders_edit_handle(VIEW_EVENT_TYPE type, void *data)
{
	LOG_INF("[%s] reminder edit!", __FUNCTION__);
	// data_service_event_submit(DATA_SERVICE_EVT_ALARM_INFO_CHG, NULL, 0);
}

void view_service_reminders_info_get(REMINDER_INFO_T **alarm_info, uint8_t *info_num)
{
	*alarm_info = reminders_info_list;
	*info_num = REMINDERS_INFO_SIZE;
}

void view_service_reminders_init(void)
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
	settings_register(&reminders_conf);
	settings_load();

	view_service_callback_reg(VIEW_EVENT_TYPE_REMINDERS_ADD, view_service_reminders_add_handle);
	view_service_callback_reg(VIEW_EVENT_TYPE_REMINDERS_DEL, view_service_reminders_del_handle);
	view_service_callback_reg(VIEW_EVENT_TYPE_REMINDERS_EDITED, view_service_reminders_edit_handle);
}