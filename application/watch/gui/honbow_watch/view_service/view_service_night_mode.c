#include "view_service_custom.h"
#include <logging/log.h>
#include "data_service_custom.h"
#include "setting_service_custom.h"

LOG_MODULE_DECLARE(guix_view_service);

static void view_service_night_mode_cfg_handle(VIEW_EVENT_TYPE type, void *data)
{
	LOG_INF("need add night mode cfg change process!");
}
void view_service_night_mode_init(void)
{
	view_service_callback_reg(VIEW_EVENT_TYPE_NIGHT_MODE_CFG_CHG, view_service_night_mode_cfg_handle);
}