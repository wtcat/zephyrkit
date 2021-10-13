#include "gx_api.h"
#include "custom_slidebutton_basic.h"
#include "custom_rounded_button.h"

#include "custom_window_title.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include <logging/log.h>
#include "app_setting.h"
#include "app_setting_disp_page.h"
#include "guix_language_resources_custom.h"
#include "view_service_custom.h"

LOG_MODULE_DECLARE(guix_log);

#define SHUTDOWN_BUTTON_ID 1
#define CANCEL_BUTTON_ID 2
static GX_WIDGET curr_page;
static Custom_SlideButton_TypeDef reboot_button;
static Custom_RoundedButton_TypeDef cacel_button;
extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_language_setting_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "demo";
	case LANGUAGE_ENGLISH:
		return "demo";
	default:
		return "demo";
	}
}
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
static UINT event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
	} break;
	case GX_EVENT_HIDE: {
	} break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		return 0;
	}
	case GX_EVENT_PEN_UP: {
		delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x >= 50) {
			// void setting_system_page_children_return(GX_WIDGET * curr);
		}
		return 0;
	}
	case GX_EVENT_PEN_DRAG:
		return 0;
	case GX_SIGNAL(SHUTDOWN_BUTTON_ID, GX_EVENT_CLICKED): {
		view_service_event_submit(VIEW_EVENT_TYPE_REBOOT, NULL, 0);
		return 0;
	}
	case GX_SIGNAL(CANCEL_BUTTON_ID, GX_EVENT_CLICKED): {
		void setting_system_page_children_return(GX_WIDGET * curr);
		setting_system_page_children_return(&curr_page);
		return 0;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}
void app_setting_system_shutdown_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_page, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	void setting_system_page_children_reg(GX_WIDGET * dst, uint8_t id);
	setting_system_page_children_reg(&curr_page, 1);

	GX_WIDGET *parent = &curr_page;

	GX_STRING string_for_button;
	string_for_button.gx_string_ptr = "shutdown";
	string_for_button.gx_string_length = strlen(string_for_button.gx_string_ptr);

	gx_utility_rectangle_define(&childSize, 12, 120, 12 + 296 - 1, 120 + 80 - 1);
	custom_slide_button_create(&reboot_button, SHUTDOWN_BUTTON_ID, parent, &childSize, GX_COLOR_ID_HONBOW_DARK_GRAY,
							   GX_COLOR_ID_HONBOW_GREEN, GX_PIXELMAP_ID_APP_SETTING_BTN_SHUTDOWN, &string_for_button,
							   GX_FONT_ID_SIZE_30, GX_COLOR_ID_HONBOW_WHITE, 6);

	gx_utility_rectangle_define(&childSize, 12, 283, 12 + 296 - 1, 283 + 65 - 1);
	custom_rounded_button_create(&cacel_button, CANCEL_BUTTON_ID, parent, &childSize, GX_COLOR_ID_HONBOW_DARK_GRAY,
								 GX_PIXELMAP_ID_APP_SETTING_BTN_L_CANCEL, NULL, CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	gx_widget_event_process_set(parent, event_processing_function);
}