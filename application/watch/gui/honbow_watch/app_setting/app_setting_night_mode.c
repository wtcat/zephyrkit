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
#include "custom_widget_scrollable.h"
#include "custom_animation.h"
#include "data_service_custom.h"
#include "view_service_custom.h"
#include "setting_service_custom.h"

LOG_MODULE_DECLARE(guix_log);

// ui组建相关定义
static GX_WIDGET curr_page;
static CUSTOM_SCROLLABLE_WIDGET scroll_page;
static GX_SCROLLBAR_APPEARANCE app_list_scrollbar_properties = {
	9,						  /* scroll width                   */
	9,						  /* thumb width                    */
	0,						  /* thumb travel min               */
	0,						  /* thumb travel max               */
	4,						  /* thumb border style             */
	0,						  /* scroll fill pixelmap           */
	0,						  /* scroll thumb pixelmap          */
	0,						  /* scroll up pixelmap             */
	0,						  /* scroll down pixelmap           */
	GX_COLOR_ID_HONBOW_WHITE, /* scroll thumb color             */
	GX_COLOR_ID_HONBOW_WHITE, /* scroll thumb border color      */
	GX_COLOR_ID_HONBOW_WHITE, /* scroll button color            */
};
static GX_SCROLLBAR app_list_scrollbar;

#define ROUND_BUTTON_ID1 1
#define ROUND_BUTTON_ID2 2
#define ROUND_BUTTON_ID3 3
#define ROUND_BUTTON_ID4 4

static Custom_RoundedButton_TypeDef round_button;
static GX_PROMPT night_mode_prompt;
static GX_MULTI_LINE_TEXT_VIEW multiline;
static CUSTOM_CHECKBOX checkbox;

static Custom_RoundedButton_TypeDef round_button2;
static CUSTOM_CHECKBOX checkbox2;
#if 0
static Custom_RoundedButton_TypeDef round_button3;
static Custom_RoundedButton_TypeDef round_button4;
#else
typedef struct time_custom_round_button {
	Custom_RoundedButton_TypeDef button;
	GX_PROMPT time_prompt;
	GX_ICON icon;
} TIME_CUSTOM_ROUND_BUTTON;
static TIME_CUSTOM_ROUND_BUTTON round_button3;
static char prompt_buff_for_start_time[8];

static TIME_CUSTOM_ROUND_BUTTON round_button4;
static char prompt_buff_for_end_time[8];

#endif
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

//逻辑定义
void app_setting_night_mode_time_refresh(GX_BOOL time_setting_flag);
extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_language_setting_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "夜晚模式";
	case LANGUAGE_ENGLISH:
		return "night mode";
	default:
		return "night mode";
	}
}

static UINT event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static GX_VALUE gx_point_x_first;
	static GX_VALUE delta_x;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		char nightmode[5] = {0};
		setting_service_get(SETTING_CUSTOM_DATA_ID_NIGHT_MODE, &nightmode, 5);

		if (nightmode[0]) {
			checkbox.gx_widget_status |= GX_STYLE_BUTTON_PUSHED;
		} else {
			checkbox.gx_widget_status &= ~GX_STYLE_BUTTON_PUSHED;
		}

		setting_service_get(SETTING_CUSTOM_DATA_ID_NIGHT_MODE_CUSTOM, &nightmode, 5);
		if (nightmode[0]) {
			checkbox2.gx_widget_status |= GX_STYLE_BUTTON_PUSHED;
		} else {
			checkbox2.gx_widget_status &= ~GX_STYLE_BUTTON_PUSHED;
		}
		app_setting_night_mode_time_refresh(nightmode[0]);

		snprintk(prompt_buff_for_start_time, 8, "%02d:%02d", nightmode[1], nightmode[2]);
		snprintk(prompt_buff_for_end_time, 8, "%02d:%02d", nightmode[3], nightmode[4]);
		GX_STRING string;
		string.gx_string_ptr = prompt_buff_for_start_time;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&round_button3.time_prompt, &string);
		string.gx_string_ptr = prompt_buff_for_end_time;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&round_button4.time_prompt, &string);
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
			setting_disp_page_children_return((GX_WIDGET *)&curr_page);
		}
		break;
	}
	case GX_EVENT_LANGUAGE_CHANGE:
		break;
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}
static void jump_to_time_select_page(uint8_t type);

static UINT scroll_page_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(ROUND_BUTTON_ID1, GX_EVENT_CLICKED): {
		custom_checkbox_status_change(&checkbox);
		char nightmode[5] = {0};
		setting_service_get(SETTING_CUSTOM_DATA_ID_NIGHT_MODE, &nightmode, 5);
		if (checkbox.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
			nightmode[0] = 1;
		} else {
			nightmode[0] = 0;
		}
		setting_service_set(SETTING_CUSTOM_DATA_ID_NIGHT_MODE, &nightmode, 5, false);
		view_service_event_submit(VIEW_EVENT_TYPE_NIGHT_MODE_CFG_CHG, NULL, 0);
		return 0;
	}
	case GX_SIGNAL(ROUND_BUTTON_ID2, GX_EVENT_CLICKED): {
		custom_checkbox_status_change(&checkbox2);
		char nightmode[5] = {0};
		setting_service_get(SETTING_CUSTOM_DATA_ID_NIGHT_MODE_CUSTOM, &nightmode, 5);
		if (checkbox2.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
			nightmode[0] = 1;
		} else {
			nightmode[0] = 0;
		}
		setting_service_set(SETTING_CUSTOM_DATA_ID_NIGHT_MODE_CUSTOM, &nightmode, 5, false);
		app_setting_night_mode_time_refresh(nightmode[0]);
		view_service_event_submit(VIEW_EVENT_TYPE_NIGHT_MODE_CFG_CHG, NULL, 0);
		return 0;
	}
	case GX_SIGNAL(ROUND_BUTTON_ID3, GX_EVENT_CLICKED):
		jump_to_time_select_page(0);
		return 0;
	case GX_SIGNAL(ROUND_BUTTON_ID4, GX_EVENT_CLICKED):
		jump_to_time_select_page(1);
		return 0;
	default:
		break;
	}
	return custom_scrollable_widget_event_process((CUSTOM_SCROLLABLE_WIDGET *)widget, event_ptr);
}

static void time_select_widget_create(void); // time select page create
void app_setting_night_mode_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);

	gx_widget_create(&curr_page, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	setting_disp_page_children_reg((GX_WIDGET *)&curr_page, DISP_SETTING_CHILD_PAGE_NIGHT_MODE);

	GX_WIDGET *parent = (GX_WIDGET *)&curr_page;
	custom_window_title_create((GX_WIDGET *)parent, &icon_title, &setting_prompt, NULL);
	GX_STRING string;
	string.gx_string_ptr = get_language_setting_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&setting_prompt, &string);

	gx_utility_rectangle_define(&childSize, 12, 58, 316, 359);
	custom_scrollable_widget_create(&scroll_page, parent, &childSize);
	gx_widget_event_process_set(&scroll_page, scroll_page_event);
	gx_widget_event_process_set(parent, event_processing_function);

	parent = (GX_WIDGET *)&scroll_page;

	gx_vertical_scrollbar_create(&app_list_scrollbar, NULL, parent, &app_list_scrollbar_properties,
								 GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB |
									 GX_SCROLLBAR_VERTICAL);

	gx_widget_fill_color_set(&app_list_scrollbar, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
							 GX_COLOR_ID_HONBOW_DARK_GRAY);
	extern VOID custom_scrollbar_vertical_draw(GX_SCROLLBAR * scrollbar);
	gx_widget_draw_set(&app_list_scrollbar, custom_scrollbar_vertical_draw);

	GX_VALUE top, left, bottom, right;
	left = 12;
	right = 300;
	top = 60;
	bottom = top + 77;

	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	string.gx_string_ptr = "night mode";
	string.gx_string_length = strlen(string.gx_string_ptr);
	custom_rounded_button_create2(&round_button, ROUND_BUTTON_ID1, parent, &childSize, GX_COLOR_ID_HONBOW_DARK_GRAY,
								  GX_PIXELMAP_NULL, &string, GX_FONT_ID_SIZE_26, CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	left = 230;
	right = 230 + 58 - 1;
	top = 83;
	bottom = 83 + 32;
	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	custom_checkbox_create(&checkbox, &round_button.widget, &checkbox_info, &childSize);
	custom_checkbox_event_from_external_enable(&checkbox, 1);
	GX_STRING nightmode;
	nightmode.gx_string_ptr = "When night mode is in effect,brightness is adjusted to 10%.";
	nightmode.gx_string_length = strlen(nightmode.gx_string_ptr);

	left = 12;
	right = 300;
	top = 60 + 77 + 10;
	bottom = top + 110 - 1;
	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	gx_multi_line_text_view_create(&multiline, NULL, parent, 0,
								   GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TEXT_LEFT, 0, &childSize);
	gx_multi_line_text_view_font_set(&multiline, GX_FONT_ID_SIZE_26);
	gx_multi_line_text_view_text_color_set(&multiline, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
										   GX_COLOR_ID_HONBOW_WHITE);
	gx_multi_line_text_view_text_set_ext(&multiline, &nightmode);
	gx_widget_fill_color_set(&multiline, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

	left = 12;
	right = 300;
	top = 60 + 77 + 120 - 1 + 10;
	bottom = top + 77;
	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	string.gx_string_ptr = "range setting";
	string.gx_string_length = strlen(string.gx_string_ptr);
	custom_rounded_button_create2(&round_button2, ROUND_BUTTON_ID2, parent, &childSize, GX_COLOR_ID_HONBOW_DARK_GRAY,
								  GX_PIXELMAP_NULL, &string, GX_FONT_ID_SIZE_26, CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	top = bottom + 10;
	bottom = top + 77;
	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	string.gx_string_ptr = "start time";
	string.gx_string_length = strlen(string.gx_string_ptr);
	custom_rounded_button_create2(&round_button3.button, ROUND_BUTTON_ID3, parent, &childSize,
								  GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string, GX_FONT_ID_SIZE_26,
								  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	gx_utility_rectangle_define(&childSize, left + 180, top, right, bottom);
	gx_prompt_create(&round_button3.time_prompt, NULL, &round_button3.button.widget, 0,
					 GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_prompt_font_set(&round_button3.time_prompt, GX_FONT_ID_SIZE_26);
	string.gx_string_ptr = "22:00";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&round_button3.time_prompt, &string);
	gx_icon_create(&round_button3.icon, NULL, &round_button3.button.widget, GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
				   GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 0, left + 246, top + 25);

	top = bottom + 10;
	bottom = top + 77;
	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	string.gx_string_ptr = "end time";
	string.gx_string_length = strlen(string.gx_string_ptr);
	custom_rounded_button_create2(&round_button4.button, ROUND_BUTTON_ID4, parent, &childSize,
								  GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string, GX_FONT_ID_SIZE_26,
								  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
	gx_utility_rectangle_define(&childSize, left + 180, top, right, bottom);
	gx_prompt_create(&round_button4.time_prompt, NULL, &round_button4.button.widget, 0,
					 GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_prompt_font_set(&round_button4.time_prompt, GX_FONT_ID_SIZE_26);
	string.gx_string_ptr = "07:00";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&round_button4.time_prompt, &string);
	gx_icon_create(&round_button4.icon, NULL, &round_button4.button.widget, GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
				   GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 0, left + 246, top + 25);

	left = 230;
	right = left + 58 - 1;
	top = 60 + 77 + 120 - 1 + 10 + 23;
	bottom = top + 32;
	gx_utility_rectangle_define(&childSize, 230, 60 + 77 + 120 - 1 + 10 + 23, 230 + 58 - 1,
								60 + 77 + 120 - 1 + 10 + 23 + 32);
	custom_checkbox_create(&checkbox2, &round_button2.widget, &checkbox_info, &childSize);
	custom_checkbox_event_from_external_enable(&checkbox2, 1);

	time_select_widget_create();
}
void app_setting_night_mode_time_refresh(GX_BOOL time_setting_flag)
{
	if (time_setting_flag) {
		gx_widget_show(&round_button3);
		gx_widget_show(&round_button4);
		gx_widget_show(&app_list_scrollbar);
	} else {
		gx_widget_hide(&round_button3);
		gx_widget_hide(&round_button4);
		gx_widget_hide(&app_list_scrollbar);
		custom_scrollable_widget_reset(&scroll_page);
	}
	gx_system_dirty_mark(&scroll_page);
}

/***************************************************************************
								时间选择界面
 ***************************************************************************/
static GX_WIDGET time_select_page;
static Custom_title_icon_t icon_title_time_select = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
};
static GX_PROMPT prompt_time_select;
#define SCROLL_WHEEL_ID 1
static GX_TEXT_SCROLL_WHEEL hour_adjust_wheel;
static GX_TEXT_SCROLL_WHEEL min_adjust_wheel;

#define ROUND_BUTTON_ID 1
static Custom_RoundedButton_TypeDef button_confirm;

static uint8_t time_select_page_type;

extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_time_setting_title(uint8_t type)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		if (type == 0) {
			return "开始时间";
		} else {
			return "结束时间";
		}
	case LANGUAGE_ENGLISH:
		if (type == 0) {
			return "start time";
		} else {
			return "end time";
		}
	default:
		if (type == 0) {
			return "start time";
		} else {
			return "end time";
		}
	}
}

const static GX_CHAR *string_list[] = {"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11",
									   "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23"};

static UINT scroll_wheel_callback(GX_TEXT_SCROLL_WHEEL *wheel, INT id, GX_STRING *string)
{
	string->gx_string_ptr = string_list[id];
	string->gx_string_length = strlen(string->gx_string_ptr);
	return 0;
}
static void jump_to_night_mode_page(void);
static UINT time_select_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static GX_VALUE gx_point_x_first;
	static GX_VALUE delta_x;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		char nightmode_custom[5] = {0};
		setting_service_get(SETTING_CUSTOM_DATA_ID_NIGHT_MODE_CUSTOM, &nightmode_custom, 5);
		if (0 == time_select_page_type) {
			gx_scroll_wheel_selected_set(&hour_adjust_wheel, nightmode_custom[1]);
			gx_scroll_wheel_selected_set(&min_adjust_wheel, nightmode_custom[2]);
		} else {
			gx_scroll_wheel_selected_set(&hour_adjust_wheel, nightmode_custom[3]);
			gx_scroll_wheel_selected_set(&min_adjust_wheel, nightmode_custom[4]);
		}
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
			jump_to_night_mode_page();
		}
		break;
	}
	case GX_SIGNAL(ROUND_BUTTON_ID, GX_EVENT_CLICKED): {
		char nightmode_custom[5] = {0};
		setting_service_get(SETTING_CUSTOM_DATA_ID_NIGHT_MODE_CUSTOM, &nightmode_custom, 5);
		if (0 == time_select_page_type) {
			INT hour, min;
			gx_scroll_wheel_selected_get(&hour_adjust_wheel, &hour);
			gx_scroll_wheel_selected_get(&min_adjust_wheel, &min);
			nightmode_custom[1] = hour;
			nightmode_custom[2] = min;
		} else {
			INT hour, min;
			gx_scroll_wheel_selected_get(&hour_adjust_wheel, &hour);
			gx_scroll_wheel_selected_get(&min_adjust_wheel, &min);
			nightmode_custom[3] = hour;
			nightmode_custom[4] = min;
		}
		setting_service_set(SETTING_CUSTOM_DATA_ID_NIGHT_MODE_CUSTOM, &nightmode_custom, 5, false);
		view_service_event_submit(VIEW_EVENT_TYPE_NIGHT_MODE_CFG_CHG, NULL, 0);
		jump_to_night_mode_page();
	} break;

	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}
static void time_select_widget_create(void)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);

	gx_widget_create(&time_select_page, NULL, NULL, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);

	GX_WIDGET *parent = (GX_WIDGET *)&time_select_page;
	custom_window_title_create((GX_WIDGET *)parent, &icon_title_time_select, &prompt_time_select, NULL);

	gx_utility_rectangle_define(&childSize, 30, 60, 149, 269);
	gx_text_scroll_wheel_create(&hour_adjust_wheel, GX_NULL, parent, 24,
								GX_STYLE_BORDER_NONE | GX_STYLE_TRANSPARENT | GX_STYLE_ENABLED | GX_STYLE_TEXT_CENTER |
									GX_STYLE_WRAP,
								SCROLL_WHEEL_ID, &childSize);
	gx_text_scroll_wheel_font_set(&hour_adjust_wheel, GX_FONT_ID_SIZE_60, GX_FONT_ID_SIZE_60);
	gx_text_scroll_wheel_text_color_set(&hour_adjust_wheel, GX_COLOR_ID_DISABLED_FILL, GX_COLOR_ID_HONBOW_WHITE,
										GX_COLOR_ID_HONBOW_WHITE);
	gx_text_scroll_wheel_callback_set_ext(&hour_adjust_wheel, scroll_wheel_callback);
	gx_scroll_wheel_row_height_set(&hour_adjust_wheel, 70);
	gx_scroll_wheel_selected_background_set(&hour_adjust_wheel, GX_PIXELMAP_ID_SCROLL_WHEEL_SELECTED_BG);
	if (parent) {
		gx_widget_attach(parent, &hour_adjust_wheel);
	}

	gx_utility_rectangle_define(&childSize, 170, 60, 289, 269);
	gx_text_scroll_wheel_create(&min_adjust_wheel, GX_NULL, parent, 24,
								GX_STYLE_BORDER_NONE | GX_STYLE_TRANSPARENT | GX_STYLE_ENABLED | GX_STYLE_TEXT_CENTER |
									GX_STYLE_WRAP,
								SCROLL_WHEEL_ID, &childSize);
	gx_text_scroll_wheel_font_set(&min_adjust_wheel, GX_FONT_ID_SIZE_60, GX_FONT_ID_SIZE_60);
	gx_text_scroll_wheel_text_color_set(&min_adjust_wheel, GX_COLOR_ID_DISABLED_FILL, GX_COLOR_ID_HONBOW_WHITE,
										GX_COLOR_ID_HONBOW_WHITE);
	gx_text_scroll_wheel_callback_set_ext(&min_adjust_wheel, scroll_wheel_callback);
	gx_scroll_wheel_row_height_set(&min_adjust_wheel, 70);
	gx_scroll_wheel_selected_background_set(&min_adjust_wheel, GX_PIXELMAP_ID_SCROLL_WHEEL_SELECTED_BG);
	if (parent) {
		gx_widget_attach(parent, &min_adjust_wheel);
	}

	gx_utility_rectangle_define(&childSize, 12, 283, 12 + 296 - 1, 283 + 65 - 1);
	custom_rounded_button_create(&button_confirm, ROUND_BUTTON_ID, parent, &childSize, GX_COLOR_ID_HONBOW_GREEN,
								 GX_PIXELMAP_ID_APP_SETTING_BTN_L_OK, NULL, CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	gx_widget_event_process_set(parent, time_select_processing_function);
}

static GX_WIDGET *animation_screen_list[] = {
	&curr_page,
	&time_select_page,
	GX_NULL,
};
static void jump_to_time_select_page(uint8_t type)
{
	time_select_page_type = type;
	GX_STRING string;
	string.gx_string_ptr = get_time_setting_title(type);
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&prompt_time_select, &string);
	custom_animation_start(animation_screen_list, (GX_WIDGET *)&app_setting_window, SCREEN_SLIDE_LEFT);
}

static void jump_to_night_mode_page(void)
{
	custom_animation_start(animation_screen_list, (GX_WIDGET *)&app_setting_window, SCREEN_SLIDE_RIGHT);
}