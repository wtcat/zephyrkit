#include "view_service_custom.h"
#include <logging/log.h>
#include "data_service_custom.h"
#include "setting_service_custom.h"

LOG_MODULE_DECLARE(guix_view_service);

static void view_bright_time_change_handle(VIEW_EVENT_TYPE type, void *data)
{
	LOG_ERR("Bright time changed!");
}

static void view_brightness_adjust_handle(VIEW_EVENT_TYPE type, void *data)
{
	LOG_INF("brightness change to %d", ((uint8_t *)data)[0]);

	// step 1: call drv edit brightness
	extern int lcd_brightness_adjust(uint8_t value);
	uint8_t brightness = ((uint8_t *)data)[0];
	lcd_brightness_adjust(brightness);

	// step 2: store brightness to setting sub system
	setting_service_set(SETTING_CUSTOM_DATA_ID_BRIGHTNESS, &brightness, 1, false);

	// step 3: notify ui by calling data service
	data_service_event_submit(DATA_SERVICE_EVT_BRIGHTNESS_CHG, data, 1);
}

void view_service_display_init(void)
{
	view_service_callback_reg(VIEW_EVENT_TYPE_BRIGHT_TIME_CHG, view_bright_time_change_handle);
	view_service_callback_reg(VIEW_EVENT_TYPE_BRIGHRNESS_CHG, view_brightness_adjust_handle);
}
