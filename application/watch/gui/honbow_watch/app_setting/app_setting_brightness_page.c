#include "gx_api.h"
#include "custom_window_title.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include <logging/log.h>
#include "app_setting.h"
#include "app_setting_disp_page.h"
#include "guix_language_resources_custom.h"
#include "view_service_custom.h"
#include "data_service_custom.h"
#include "setting_service_custom.h"

LOG_MODULE_DECLARE(guix_log);
static GX_WIDGET disp_setting_brightness_page;
static Custom_title_icon_t icon_title = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
};
static GX_PROMPT setting_prompt;
static GX_PROMPT time_prompt;
static char time_prompt_buff[8];

// main info
static GX_PROMPT background;
static GX_TEXT_SCROLL_WHEEL brightness_adjust_wheel;
#define SCROLL_WHEEL_ID 1

extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_language_setting_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "亮度";
	case LANGUAGE_ENGLISH:
		return "Brightness";
	default:
		return "Brightness";
	}
}
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
static UINT event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		extern uint8_t lcd_brightness_get(void);
		uint8_t brightness = 0;
		setting_service_get(SETTING_CUSTOM_DATA_ID_BRIGHTNESS, &brightness, 1);
		uint8_t id = brightness / 25 - 1;
		gx_scroll_wheel_selected_set(&brightness_adjust_wheel, id);
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
			setting_disp_page_children_return(&disp_setting_brightness_page);
		}
		break;
	}
	case GX_SIGNAL(SCROLL_WHEEL_ID, GX_EVENT_LIST_SELECT): {
		uint8_t data = (event_ptr->gx_event_payload.gx_event_longdata + 1) * 25;
		view_service_event_submit(VIEW_EVENT_TYPE_BRIGHRNESS_CHG, &data, 1);
		break;
	}
	case DATA_SERVICE_EVT_BRIGHTNESS_CHG: {
	} break;
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

static GX_CHAR *string_list[] = {"10", "20", "30", "40", "50", "60", "70", "80", "90", "100"};

static UINT scroll_wheel_callback(GX_TEXT_SCROLL_WHEEL *wheel, INT id, GX_STRING *string)
{
	string->gx_string_ptr = string_list[id];
	string->gx_string_length = strlen(string->gx_string_ptr);
}

void app_setting_brightness_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&disp_setting_brightness_page, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
					 &childSize);
	setting_disp_page_children_reg(&disp_setting_brightness_page, DISP_SETTING_CHILD_PAGE_BRIGHTNESS);
	// child page need init

	GX_WIDGET *parent = &disp_setting_brightness_page;
	custom_window_title_create(parent, &icon_title, &setting_prompt, NULL);

	GX_STRING string;
	string.gx_string_ptr = get_language_setting_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&setting_prompt, &string);

	GX_RECTANGLE childsize;

	gx_utility_rectangle_define(&childsize, 219, 190, 300, 259);

	gx_prompt_create(&background, NULL, parent, 0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 0,
					 &childsize);
	gx_prompt_text_color_set(&background, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&background, GX_FONT_ID_SIZE_30);
	string.gx_string_ptr = "%";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&background, &string);

	gx_utility_rectangle_define(&childSize, 90, 100, 209, 309);
	gx_text_scroll_wheel_create(&brightness_adjust_wheel, GX_NULL, parent, 10,
								GX_STYLE_BORDER_NONE | GX_STYLE_TRANSPARENT | GX_STYLE_ENABLED | GX_STYLE_TEXT_CENTER,
								SCROLL_WHEEL_ID, &childSize);
	gx_text_scroll_wheel_font_set(&brightness_adjust_wheel, GX_FONT_ID_SIZE_60, GX_FONT_ID_SIZE_60);
	gx_text_scroll_wheel_text_color_set(&brightness_adjust_wheel, GX_COLOR_ID_DISABLED_FILL, GX_COLOR_ID_HONBOW_WHITE,
										GX_COLOR_ID_HONBOW_WHITE);
	gx_text_scroll_wheel_callback_set_ext(&brightness_adjust_wheel, scroll_wheel_callback);
	gx_scroll_wheel_row_height_set(&brightness_adjust_wheel, 70);
	gx_scroll_wheel_selected_background_set(&brightness_adjust_wheel, GX_PIXELMAP_ID_SCROLL_WHEEL_SELECTED_BG);
	if (parent) {
		gx_widget_attach(parent, &brightness_adjust_wheel);
	}
	gx_widget_event_process_set(parent, event_processing_function);
}