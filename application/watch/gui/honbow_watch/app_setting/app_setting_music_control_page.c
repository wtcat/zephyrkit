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
static CUSTOM_CHECKBOX checkbox;
static GX_MULTI_LINE_TEXT_VIEW tips;
static Custom_title_icon_t icon_title = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
};
static GX_PROMPT setting_prompt;
static GX_PROMPT time_prompt;
static char time_prompt_buff[8];

#define ROUND_BUTTON_ID 1
#define SWITCH_BTTON_ID 0xf0
static CUSTOM_CHECKBOX_INFO checkbox_info = {SWITCH_BTTON_ID,
											 GX_PIXELMAP_ID_APP_SETTING_SWITCH_BG,
											 GX_PIXELMAP_ID_APP_SETTING_SWITCH_ACTIVE,
											 GX_PIXELMAP_ID_APP_SETTING_SWITCH_DISACTIVE,
											 4,
											 24};
extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "音乐控制";
	case LANGUAGE_ENGLISH:
		return "Music Control";
	default:
		return "Music Control";
	}
}

static const GX_CHAR english_tips[] =
	"if the function is enabled, when the phone plays music,Automatically turn on music control.";

static const GX_CHAR *get_tips_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;

	switch (id) {
	case LANGUAGE_ENGLISH:
		return english_tips;
	default:
		return english_tips;
	}
}

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
		break;
	}
	case GX_EVENT_PEN_UP: {
		delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x >= 50) {
			extern void jump_setting_main_page(GX_WIDGET * now);
			jump_setting_main_page(&curr_page);
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
void app_setting_music_control_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_page, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	extern void setting_main_page_jump_reg(GX_WIDGET * dst, SETTING_CHILD_PAGE_T id);
	setting_main_page_jump_reg(&curr_page, SETTING_CHILD_PAGE_MUSIC_CTRL);

	GX_WIDGET *parent = &curr_page;
	custom_window_title_create(parent, &icon_title, &setting_prompt, NULL);

	GX_STRING string;
	string.gx_string_ptr = get_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&setting_prompt, &string);

	gx_utility_rectangle_define(&childSize, 10, 60, 10 + 300 - 1, 60 + 78 - 1);
	string.gx_string_ptr = "Music Control";
	string.gx_string_length = strlen(string.gx_string_ptr);
	custom_rounded_button_create(&wrist_raise_button, ROUND_BUTTON_ID, &curr_page, &childSize,
								 GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string,
								 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	gx_utility_rectangle_define(&childSize, 230, 83, 230 + 58 - 1, 83 + 32);
	custom_checkbox_create(&checkbox, &wrist_raise_button.widget, &checkbox_info, &childSize);
	custom_checkbox_event_from_external_enable(&checkbox, 1);

	GX_VALUE top, left, bottom, right;
	left = 12;
	right = 300;
	top = 60 + 77 + 10;
	bottom = top + 110 - 1;
	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	gx_multi_line_text_view_create(&tips, NULL, parent, 0, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TEXT_LEFT,
								   0, &childSize);
	gx_multi_line_text_view_font_set(&tips, GX_FONT_ID_SYSTEM);
	gx_multi_line_text_view_text_color_set(&tips, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
										   GX_COLOR_ID_HONBOW_WHITE);

	GX_STRING tips_string;
	tips_string.gx_string_ptr = get_tips_string();
	tips_string.gx_string_length = strlen(tips_string.gx_string_ptr);
	gx_multi_line_text_view_text_set_ext(&tips, &tips_string);
	gx_widget_fill_color_set(&tips, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

	gx_widget_event_process_set(parent, event_processing_function);
}