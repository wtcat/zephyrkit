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

LOG_MODULE_DECLARE(guix_log);

#define CONFIRM_BUTTON_ID 1
#define CANCEL_BUTTON_ID 2
static GX_WIDGET curr_page;

static GX_PROMPT tips;
static Custom_RoundedButton_TypeDef confirm_button;
static Custom_RoundedButton_TypeDef cacel_button;
extern GX_WINDOW_ROOT *root;
static UINT event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static GX_VALUE gx_point_x_first;
	static GX_VALUE delta_x;

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
			void setting_system_page_children_return(GX_WIDGET * curr);
			// setting_system_page_children_return(&curr_page);
		}
		return 0;
	}
	case GX_EVENT_PEN_DRAG:
		return 0;
	case GX_SIGNAL(CONFIRM_BUTTON_ID, GX_EVENT_CLICKED): {
		// send to view service!
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
void app_setting_system_factory_reset_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_page, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	void setting_system_page_children_reg(GX_WIDGET * dst, uint8_t id);
	setting_system_page_children_reg(&curr_page, 2);

	GX_WIDGET *parent = &curr_page;

	GX_STRING string_for_tips;
	string_for_tips.gx_string_ptr = "factory reset?";
	string_for_tips.gx_string_length = strlen(string_for_tips.gx_string_ptr);

	gx_utility_rectangle_define(&childSize, 0, 140, 320 - 1, 140 + 30 - 1);
	gx_prompt_create(&tips, NULL, parent, 0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 0,
					 &childSize);
	gx_prompt_text_color_set(&tips, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&tips, GX_FONT_ID_SIZE_30);
	gx_prompt_text_set_ext(&tips, &string_for_tips);

	gx_utility_rectangle_define(&childSize, 165, 283, 165 + 143 - 1, 283 + 65 - 1);
	custom_rounded_button_create(&confirm_button, CONFIRM_BUTTON_ID, parent, &childSize, GX_COLOR_ID_HONBOW_GREEN,
								 GX_PIXELMAP_ID_APP_SETTING_BTN_S_CONFIRM, NULL, CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	gx_utility_rectangle_define(&childSize, 12, 283, 12 + 143 - 1, 283 + 65 - 1);
	custom_rounded_button_create(&cacel_button, CANCEL_BUTTON_ID, parent, &childSize, GX_COLOR_ID_HONBOW_DARK_GRAY,
								 GX_PIXELMAP_ID_APP_SETTING_BTN_S_CANCEL, NULL, CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	gx_widget_event_process_set(parent, event_processing_function);
}