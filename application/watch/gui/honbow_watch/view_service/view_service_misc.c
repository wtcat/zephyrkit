#include "view_service_custom.h"
#include <logging/log.h>
#include "data_service_custom.h"
#include "setting_service_custom.h"
#include "power/reboot.h"
LOG_MODULE_DECLARE(guix_view_service);

static void view_service_app_view_cfg_handle(VIEW_EVENT_TYPE type, void *data)
{
	// now just write to setting.
	setting_service_set(SETTING_CUSTOM_DATA_ID_APP_VIEW_TYPE, data, 1, false);
	LOG_INF("[%s]need do more about app view cfg change process!", __FUNCTION__);
}

static void view_service_system_reboot_handle(VIEW_EVENT_TYPE type, void *data)
{
#if CONFIG_REBOOT
	sys_reboot(SYS_REBOOT_COLD);
#else
	LOG_ERR("[%s] reboot func not enabled!", __FUNCTION__);
#endif
}
void view_service_misc_init(void)
{
	view_service_callback_reg(VIEW_EVENT_TYPE_APP_VIEW_CFG_CHG, view_service_app_view_cfg_handle);
	view_service_callback_reg(VIEW_EVENT_TYPE_REBOOT, view_service_system_reboot_handle);
}