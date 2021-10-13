#include "gx_api.h"
#include "timer_widget.h"
#include "custom_widget_scrollable.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include "custom_window_title.h"
#include "app_timer_language.h"
#include "stdio.h"
#include "windows_manager.h"
#include "guix_simple_specifications.h"
#include "custom_rounded_button.h"
#include "guix_simple_resources.h"
#include "app_timer.h"
#include "custom_rounded_button.h"
#include "logging/log.h"

LOG_MODULE_DECLARE(guix_log);

static GX_WIDGET curr_widget;
static GX_PROMPT app_prompt;
static GX_NUMERIC_SCROLL_WHEEL hour_scroll_wheel;
static GX_NUMERIC_SCROLL_WHEEL min_scroll_wheel;
static GX_NUMERIC_SCROLL_WHEEL sec_scroll_wheel;
static Custom_RoundedButton_TypeDef next_finish_button;

#define HOUR_SCROLL_ID 1
#define MIN_SCROLL_ID 2
#define SEC_SCROLL_ID 3
#define NEXT_OR_FINISH_BUTTON_ID 4

static UINT timer_page_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static WM_QUIT_JUDGE_T pen_info_judge;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN:
		wm_point_down_record(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata);
		break;
	case GX_EVENT_PEN_UP:
		if (wm_quit_judge(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata) == GX_TRUE) {
			app_timer_page_jump_internal(&curr_widget, APP_TIMER_PAGE_JUMP_BACK);
		}
		break;
	case GX_SIGNAL(NEXT_OR_FINISH_BUTTON_ID, GX_EVENT_CLICKED):
		LOG_INF("start custom timer!");
		INT hour, min, sec;
		TIMER_PARA_TRANS_T para;
		gx_scroll_wheel_selected_get(&hour_scroll_wheel, &hour);
		gx_scroll_wheel_selected_get(&min_scroll_wheel, &min);
		gx_scroll_wheel_selected_get(&sec_scroll_wheel, &sec);
		if ((hour != 0) || (min != 0) || (sec != 0)) {
			app_timer_custom_add(hour, min, sec);
			para.hour = hour;
			para.min = min;
			para.sec = sec;
			para.tenth_of_second = 0;
			app_timer_main_page_show(GX_FALSE, (GX_WINDOW *)&app_timer_window, &curr_widget, para);
		}
		return 0;
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}
extern UINT custom_numeric_scroll_wheel_text_get(GX_NUMERIC_SCROLL_WHEEL *wheel, INT row, GX_STRING *string);
void app_timer_custom_page_init(void)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_widget, NULL, NULL, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_widget_event_process_set(&curr_widget, timer_page_event_process);

	GX_WIDGET *parent = &curr_widget;
	custom_window_title_create(parent, NULL, &app_prompt, NULL);
	GX_STRING string;
	string.gx_string_ptr = app_timer_custom_get_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&app_prompt, &string);

	gx_utility_rectangle_define(&childSize, 22, 70, 22 + 69 - 1, 270 - 1);
	gx_numeric_scroll_wheel_create(&hour_scroll_wheel, NULL, parent, 0, 23,
								   GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_WRAP | GX_STYLE_TEXT_CENTER,
								   HOUR_SCROLL_ID, &childSize);
	gx_widget_fill_color_set(&hour_scroll_wheel, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);
	gx_text_scroll_wheel_font_set(&hour_scroll_wheel, GX_FONT_ID_SIZE_60, GX_FONT_ID_SIZE_60);
	gx_text_scroll_wheel_text_color_set(&hour_scroll_wheel, GX_COLOR_ID_DISABLED_FILL, GX_COLOR_ID_HONBOW_WHITE,
										GX_COLOR_ID_HONBOW_WHITE);
	gx_scroll_wheel_row_height_set(&hour_scroll_wheel, 70);
	hour_scroll_wheel.gx_text_scroll_wheel_text_get =
		(UINT(*)(GX_TEXT_SCROLL_WHEEL *, INT, GX_STRING *))custom_numeric_scroll_wheel_text_get;

	gx_utility_rectangle_define(&childSize, 126, 70, 126 + 69 - 1, 270 - 1);
	gx_numeric_scroll_wheel_create(&min_scroll_wheel, NULL, parent, 0, 59,
								   GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_WRAP | GX_STYLE_TEXT_CENTER,
								   MIN_SCROLL_ID, &childSize);
	gx_widget_fill_color_set(&min_scroll_wheel, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);
	gx_text_scroll_wheel_font_set(&min_scroll_wheel, GX_FONT_ID_SIZE_60, GX_FONT_ID_SIZE_60);
	gx_text_scroll_wheel_text_color_set(&min_scroll_wheel, GX_COLOR_ID_DISABLED_FILL, GX_COLOR_ID_HONBOW_WHITE,
										GX_COLOR_ID_HONBOW_WHITE);
	gx_scroll_wheel_row_height_set(&min_scroll_wheel, 70);
	min_scroll_wheel.gx_text_scroll_wheel_text_get =
		(UINT(*)(GX_TEXT_SCROLL_WHEEL *, INT, GX_STRING *))custom_numeric_scroll_wheel_text_get;

	gx_utility_rectangle_define(&childSize, 229, 70, 229 + 69 - 1, 270 - 1);
	gx_numeric_scroll_wheel_create(&sec_scroll_wheel, NULL, parent, 0, 59,
								   GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_WRAP | GX_STYLE_TEXT_CENTER,
								   SEC_SCROLL_ID, &childSize);
	gx_widget_fill_color_set(&sec_scroll_wheel, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);
	gx_text_scroll_wheel_font_set(&sec_scroll_wheel, GX_FONT_ID_SIZE_60, GX_FONT_ID_SIZE_60);
	gx_text_scroll_wheel_text_color_set(&sec_scroll_wheel, GX_COLOR_ID_DISABLED_FILL, GX_COLOR_ID_HONBOW_WHITE,
										GX_COLOR_ID_HONBOW_WHITE);
	gx_scroll_wheel_row_height_set(&sec_scroll_wheel, 70);
	sec_scroll_wheel.gx_text_scroll_wheel_text_get =
		(UINT(*)(GX_TEXT_SCROLL_WHEEL *, INT, GX_STRING *))custom_numeric_scroll_wheel_text_get;

	gx_utility_rectangle_define(&childSize, 12, 283, 12 + 296 - 1, 283 + 65 - 1);
	custom_rounded_button_create(&next_finish_button, NEXT_OR_FINISH_BUTTON_ID, parent, &childSize,
								 GX_COLOR_ID_HONBOW_GREEN, GX_PIXELMAP_ID_APP_TIMER_BTN_L_START, NULL,
								 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	app_timer_page_reg(APP_TIMER_PAGE_CUSTOM, &curr_widget);
}