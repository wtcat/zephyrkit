#include "gx_api.h"
#include "custom_window_title.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include <logging/log.h>
#include "app_setting.h"
#include "app_setting_disp_page.h"
#include "guix_language_resources_custom.h"
#include "custom_rounded_button.h"
#include "custom_checkbox.h"

LOG_MODULE_DECLARE(guix_log);
static GX_WIDGET curr_page;
static Custom_RoundedButton_TypeDef wrist_raise_button;

#define ROUND_BUTTON_ID 1

static CUSTOM_CHECKBOX checkbox;
#define SWITCH_BTTON_ID 0xf0
static CUSTOM_CHECKBOX_INFO checkbox_info = {SWITCH_BTTON_ID,
											 GX_PIXELMAP_ID_APP_SETTING_SWITCH_BG,
											 GX_PIXELMAP_ID_APP_SETTING_SWITCH_ACTIVE,
											 GX_PIXELMAP_ID_APP_SETTING_SWITCH_DISACTIVE,
											 4,
											 24};

static Custom_title_icon_t icon_title = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
};
static GX_PROMPT setting_prompt;
static GX_PROMPT time_prompt;
static char time_prompt_buff[8];

extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_language_setting_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "抬腕亮屏";
	case LANGUAGE_ENGLISH:
		return "wake config";
	default:
		return "wake config";
	}
}
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);

static UINT event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
	} break;
	case GX_EVENT_HIDE: {
	} break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}
	case GX_EVENT_PEN_UP: {
		delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x >= 50) {
			setting_disp_page_children_return(&curr_page);
		}
		break;
	}
	case GX_SIGNAL(ROUND_BUTTON_ID, GX_EVENT_CLICKED): {
		custom_checkbox_status_change(&checkbox);
	} break;
	case GX_EVENT_LANGUAGE_CHANGE:
		break;
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}
void app_setting_wrist_raise_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_page, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	setting_disp_page_children_reg(&curr_page, DISP_SETTING_CHILD_PAGE_WRIST_RAISE);

	GX_WIDGET *parent = &curr_page;
	custom_window_title_create(parent, &icon_title, &setting_prompt, NULL);

	GX_STRING string;
	string.gx_string_ptr = get_language_setting_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&setting_prompt, &string);

	gx_utility_rectangle_define(&childSize, 10, 60, 310, 60 + 77);

	string.gx_string_ptr = "wake up";
	string.gx_string_length = strlen(string.gx_string_ptr);
	custom_rounded_button_create(&wrist_raise_button, ROUND_BUTTON_ID, &curr_page, &childSize,
								 GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string,
								 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	gx_utility_rectangle_define(&childSize, 230, 83, 230 + 58 - 1, 83 + 32);
	custom_checkbox_create(&checkbox, &wrist_raise_button.widget, &checkbox_info, &childSize);
	custom_checkbox_event_from_external_enable(&checkbox, 1);
	gx_widget_event_process_set(parent, event_processing_function);
}