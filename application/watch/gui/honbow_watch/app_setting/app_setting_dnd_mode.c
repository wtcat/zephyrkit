#include "gx_api.h"
#include "custom_window_title.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include <logging/log.h>
#include "app_setting.h"
#include "guix_language_resources_custom.h"
#include "custom_rounded_button.h"
#include "custom_checkbox.h"
#include "custom_widget_scrollable.h"
#include "custom_animation.h"
#include "data_service_custom.h"
#include "view_service_custom.h"
#include "setting_service_custom.h"
#include "app_list_define.h"

#define ROUND_BUTTON_ID1 1
#define ROUND_BUTTON_ID2 2
#define ROUND_BUTTON_ID3 3
#define ROUND_BUTTON_ID4 4

const static GX_SCROLLBAR_APPEARANCE scrollbar_properties = {
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
#define SWITCH_BTTON_ID 0xf0
const static CUSTOM_CHECKBOX_INFO checkbox_info = {SWITCH_BTTON_ID,
												   GX_PIXELMAP_ID_APP_SETTING_SWITCH_BG,
												   GX_PIXELMAP_ID_APP_SETTING_SWITCH_ACTIVE,
												   GX_PIXELMAP_ID_APP_SETTING_SWITCH_DISACTIVE,
												   4,
												   24};

struct page_info {
	GX_WIDGET curr_main_page;		   // main page
	Custom_title_icon_t icon_of_title; // children of curr_main_page
	GX_PROMPT prompt_title;			   // children of curr_main_page

	CUSTOM_SCROLLABLE_WIDGET scroll_page; // children of curr_main_page
	GX_SCROLLBAR scrollbar;

	Custom_RoundedButton_TypeDef button_dnd_switch; // children of scroll_page
	CUSTOM_CHECKBOX checkbox_for_dnd_switch;
	GX_MULTI_LINE_TEXT_VIEW info_prompt;				   // children of scroll_page
	Custom_RoundedButton_TypeDef button_dnd_custom_switch; // children of scroll_page
	CUSTOM_CHECKBOX checkbox_for_dnd_custom_switch;

	Custom_RoundedButton_TypeDef button_dnd_time_start; // children of scroll_page
	GX_PROMPT time_prompt_for_time_start;
	GX_ICON icon_for_time_start;

	Custom_RoundedButton_TypeDef button_dnd_time_end; // children of scroll_page
	GX_PROMPT time_prompt_for_time_end;
	GX_ICON icon_for_time_end;
};
static struct page_info curr_page;

extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "勿扰模式";
	case LANGUAGE_ENGLISH:
		return "DND Mode";
	default:
		return "DND Mode";
	}
}
static void app_setting_night_mode_time_refresh(GX_BOOL time_setting_flag);
static void jump_to_time_select_page(uint8_t type);
static UINT scroll_page_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(ROUND_BUTTON_ID1, GX_EVENT_CLICKED): {
		custom_checkbox_status_change(&curr_page.checkbox_for_dnd_switch);
		if (curr_page.checkbox_for_dnd_switch.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		} else {
		}
		return 0;
	}
	case GX_SIGNAL(ROUND_BUTTON_ID2, GX_EVENT_CLICKED): {
		custom_checkbox_status_change(&curr_page.checkbox_for_dnd_custom_switch);
		// setting_service_get(SETTING_CUSTOM_DATA_ID_NIGHT_MODE_CUSTOM, &nightmode, 5);
		if (curr_page.checkbox_for_dnd_custom_switch.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
			app_setting_night_mode_time_refresh(1);
		} else {
			app_setting_night_mode_time_refresh(0);
		}
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
extern void jump_setting_main_page(GX_WIDGET *now);
static UINT event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static GX_VALUE gx_point_x_first;
	static GX_VALUE delta_x;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		if (curr_page.checkbox_for_dnd_custom_switch.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
			app_setting_night_mode_time_refresh(1);
		} else {
			app_setting_night_mode_time_refresh(0);
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
		if (delta_x >= 100) {
			jump_setting_main_page(&curr_page.curr_main_page);
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
static void time_select_widget_create(void);

void app_setting_dnd_mode_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);

	gx_widget_create(&curr_page.curr_main_page, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
					 &childSize);
	extern void setting_main_page_jump_reg(GX_WIDGET * dst, SETTING_CHILD_PAGE_T id);
	setting_main_page_jump_reg(&curr_page.curr_main_page, SETTING_CHILD_PAGE_DND);

	GX_WIDGET *parent = (GX_WIDGET *)&curr_page.curr_main_page;

	curr_page.icon_of_title.bg_color = GX_COLOR_ID_HONBOW_GREEN;
	curr_page.icon_of_title.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK;

	custom_window_title_create((GX_WIDGET *)parent, &curr_page.icon_of_title, &curr_page.prompt_title, NULL);
	GX_STRING string;
	string.gx_string_ptr = get_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&curr_page.prompt_title, &string);

	gx_utility_rectangle_define(&childSize, 12, 58, 315, 359);
	custom_scrollable_widget_create(&curr_page.scroll_page, parent, &childSize);
	gx_widget_event_process_set(&curr_page.scroll_page, scroll_page_event);
	gx_widget_event_process_set(parent, event_processing_function);

	parent = (GX_WIDGET *)&curr_page.scroll_page;
	gx_vertical_scrollbar_create(&curr_page.scrollbar, NULL, parent, &scrollbar_properties,
								 GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB |
									 GX_SCROLLBAR_VERTICAL);
	gx_widget_fill_color_set(&curr_page.scrollbar, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
							 GX_COLOR_ID_HONBOW_DARK_GRAY);
	extern VOID custom_scrollbar_vertical_draw(GX_SCROLLBAR * scrollbar);
	gx_widget_draw_set(&curr_page.scrollbar, custom_scrollbar_vertical_draw);

	GX_VALUE top, left, bottom, right;
	left = 12;
	right = 300;
	top = 60;
	bottom = top + 77;

	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	string.gx_string_ptr = "DND Mode";
	string.gx_string_length = strlen(string.gx_string_ptr);
	custom_rounded_button_create2(&curr_page.button_dnd_switch, ROUND_BUTTON_ID1, parent, &childSize,
								  GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string, GX_FONT_ID_SIZE_30,
								  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	left = 230;
	right = 230 + 58 - 1;
	top = 83;
	bottom = 83 + 32;
	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	custom_checkbox_create(&curr_page.checkbox_for_dnd_switch, &curr_page.button_dnd_switch.widget, &checkbox_info,
						   &childSize);
	custom_checkbox_event_from_external_enable(&curr_page.checkbox_for_dnd_switch, 1);

	GX_STRING nightmode;
	nightmode.gx_string_ptr = "When DND is turned on, the device will stop receiving Message alerts,\
							  except alarm clocks and high heart rate alerts .";
	nightmode.gx_string_length = strlen(nightmode.gx_string_ptr);

	left = 12;
	right = 300;
	top = 60 + 77 + 10;
	bottom = top + 110 - 1;
	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	gx_multi_line_text_view_create(&curr_page.info_prompt, NULL, parent, 0,
								   GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TEXT_LEFT, 0, &childSize);
	gx_multi_line_text_view_font_set(&curr_page.info_prompt, GX_FONT_ID_SYSTEM);
	gx_multi_line_text_view_text_color_set(&curr_page.info_prompt, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
										   GX_COLOR_ID_HONBOW_WHITE);
	gx_multi_line_text_view_text_set_ext(&curr_page.info_prompt, &nightmode);
	gx_widget_fill_color_set(&curr_page.info_prompt, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
							 GX_COLOR_ID_HONBOW_BLACK);

	left = 12;
	right = 300;
	top = 60 + 77 + 120 - 1 + 10;
	bottom = top + 77;
	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	string.gx_string_ptr = "Range Setting";
	string.gx_string_length = strlen(string.gx_string_ptr);
	custom_rounded_button_create2(&curr_page.button_dnd_custom_switch, ROUND_BUTTON_ID2, parent, &childSize,
								  GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string, GX_FONT_ID_SIZE_26,
								  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	top = bottom + 10;
	bottom = top + 77;
	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	string.gx_string_ptr = "start time";
	string.gx_string_length = strlen(string.gx_string_ptr);
	custom_rounded_button_create2(&curr_page.button_dnd_time_start, ROUND_BUTTON_ID3, parent, &childSize,
								  GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string, GX_FONT_ID_SIZE_26,
								  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	gx_utility_rectangle_define(&childSize, left + 180, top, right, bottom);
	gx_prompt_create(&curr_page.time_prompt_for_time_start, NULL, &curr_page.button_dnd_time_start.widget, 0,
					 GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_prompt_font_set(&curr_page.time_prompt_for_time_start, GX_FONT_ID_SIZE_26);
	string.gx_string_ptr = "22:00";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&curr_page.time_prompt_for_time_start, &string);
	gx_icon_create(&curr_page.icon_for_time_start, NULL, &curr_page.button_dnd_time_start.widget,
				   GX_PIXELMAP_ID_APP_SETTING_BTN_BACK, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 0, left + 246,
				   top + 25);

	top = bottom + 10;
	bottom = top + 77;
	gx_utility_rectangle_define(&childSize, left, top, right, bottom);
	string.gx_string_ptr = "end time";
	string.gx_string_length = strlen(string.gx_string_ptr);
	custom_rounded_button_create2(&curr_page.button_dnd_time_end, ROUND_BUTTON_ID4, parent, &childSize,
								  GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string, GX_FONT_ID_SIZE_26,
								  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
	gx_utility_rectangle_define(&childSize, left + 180, top, right, bottom);
	gx_prompt_create(&curr_page.time_prompt_for_time_end, NULL, &curr_page.button_dnd_time_end.widget, 0,
					 GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_prompt_font_set(&curr_page.time_prompt_for_time_end, GX_FONT_ID_SIZE_26);
	string.gx_string_ptr = "07:00";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&curr_page.time_prompt_for_time_end, &string);
	gx_icon_create(&curr_page.icon_for_time_end, NULL, &curr_page.button_dnd_time_end.widget,
				   GX_PIXELMAP_ID_APP_SETTING_BTN_BACK, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 0, left + 246,
				   top + 25);

	left = 230;
	right = left + 58 - 1;
	top = 60 + 77 + 120 - 1 + 10 + 23;
	bottom = top + 32;
	gx_utility_rectangle_define(&childSize, 230, 60 + 77 + 120 - 1 + 10 + 23, 230 + 58 - 1,
								60 + 77 + 120 - 1 + 10 + 23 + 32);
	custom_checkbox_create(&curr_page.checkbox_for_dnd_custom_switch, &curr_page.button_dnd_custom_switch.widget,
						   &checkbox_info, &childSize);
	custom_checkbox_event_from_external_enable(&curr_page.checkbox_for_dnd_custom_switch, 1);

	time_select_widget_create();
}

static void app_setting_night_mode_time_refresh(GX_BOOL time_setting_flag)
{
	if (time_setting_flag) {
		gx_widget_show(&curr_page.button_dnd_time_start);
		gx_widget_show(&curr_page.button_dnd_time_end);
		gx_widget_show(&curr_page.scrollbar);
	} else {
		gx_widget_hide(&curr_page.button_dnd_time_start);
		gx_widget_hide(&curr_page.button_dnd_time_end);
		gx_widget_hide(&curr_page.scrollbar);
		custom_scrollable_widget_reset(&curr_page.scroll_page);
	}
	gx_system_dirty_mark(&curr_page.scroll_page);
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

static uint8_t time_select_pafe_type;

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
		if (0 == time_select_pafe_type) {
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
		if (0 == time_select_pafe_type) {
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
	&curr_page.curr_main_page,
	&time_select_page,
	GX_NULL,
};
static void jump_to_time_select_page(uint8_t type)
{
	time_select_pafe_type = type;
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