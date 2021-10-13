#include "gx_api.h"
#include "custom_rounded_button.h"
#include "custom_window_title.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "app_alarm_page_mgr.h"
#include "stdio.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include "guix_language_resources_custom.h"
#include "logging/log.h"

LOG_MODULE_DECLARE(guix_log);
#define HOUR_SCROLL_ID 1
#define MIN_SCROLL_ID 2
#define NEXT_OR_FINISH_BUTTON_ID 3
static GX_WIDGET alarm_time_select_page;
static ALARM_EDIT_TYPE edit_type;
static ALARM_INFO_T *alarm_info;
static Custom_title_icon_t icon_tips = {
	.bg_color = GX_COLOR_ID_HONBOW_BLUE,
	.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
};
static GX_PROMPT prompt_time_select;

static GX_NUMERIC_SCROLL_WHEEL hour_scroll_wheel;
static GX_NUMERIC_SCROLL_WHEEL min_scroll_wheel;
static Custom_RoundedButton_TypeDef next_finish_button;
static int alarm_time_sel_page_info_update(ALARM_EDIT_TYPE type, ALARM_INFO_T *info);
extern UINT _gx_utility_string_length_check(GX_CONST GX_CHAR *input_string, UINT *string_length,
											UINT max_string_length);
UINT custom_numeric_scroll_wheel_text_get(GX_NUMERIC_SCROLL_WHEEL *wheel, INT row, GX_STRING *string)
{
	INT step;
	INT val;

	if (row < wheel->gx_scroll_wheel_total_rows) {
		step = wheel->gx_numeric_scroll_wheel_end_val - wheel->gx_numeric_scroll_wheel_start_val;

		if (step == (wheel->gx_scroll_wheel_total_rows - 1)) {
			step = 1;
		} else if (step == (1 - wheel->gx_scroll_wheel_total_rows)) {
			step = -1;
		} else if (wheel->gx_scroll_wheel_total_rows > 1) {
			step /= (wheel->gx_scroll_wheel_total_rows - 1);
		}

		val = wheel->gx_numeric_scroll_wheel_start_val + (step * row);

		snprintf(wheel->gx_numeric_scroll_wheel_string_buffer, GX_NUMERIC_SCROLL_WHEEL_STRING_BUFFER_SIZE, "%02d", val);

		string->gx_string_ptr = wheel->gx_numeric_scroll_wheel_string_buffer;
		_gx_utility_string_length_check(string->gx_string_ptr, &string->gx_string_length,
										GX_NUMERIC_SCROLL_WHEEL_STRING_BUFFER_SIZE - 1);
	} else {
		string->gx_string_ptr = GX_NULL;
		string->gx_string_length = 0;
	}

	return (GX_SUCCESS);
}

static UINT alarm_time_sel_page_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static GX_VALUE gx_point_x_first;
	static GX_VALUE delta_x;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_HIDE: {
	} break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}
	case GX_EVENT_PEN_UP: {
		delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x >= 100) {
			app_alarm_page_jump(ALARM_PAGE_TIME_SEL, ALARM_OP_CANCEL);
		}
		break;
	}
	case GX_EVENT_LANGUAGE_CHANGE:
		break;
	case GX_SIGNAL(NEXT_OR_FINISH_BUTTON_ID, GX_EVENT_CLICKED): {
		LOG_INF("button next clicked!\n");
		if (alarm_info != NULL) {
			INT hour, min;
			gx_scroll_wheel_selected_get(&hour_scroll_wheel, &hour);
			gx_scroll_wheel_selected_get(&min_scroll_wheel, &min);
			alarm_info->hour = hour;
			alarm_info->min = min;
		}
		app_alarm_page_jump(ALARM_PAGE_TIME_SEL, ALARM_OP_NEXT_OR_FINISH);
		break;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

extern GX_WINDOW_ROOT *root;

static const GX_CHAR *get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "时间";
	case LANGUAGE_ENGLISH:
		return "Time";
	default:
		return "Time";
	}
}

void alarm_time_sel_page_init(void)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);

	gx_widget_create(&alarm_time_select_page, NULL, NULL, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
					 &childSize);
	gx_widget_event_process_set(&alarm_time_select_page, alarm_time_sel_page_event_process);

	GX_WIDGET *parent = (GX_WIDGET *)&alarm_time_select_page;
	custom_window_title_create((GX_WIDGET *)parent, &icon_tips, &prompt_time_select, NULL);

	GX_STRING string;
	string.gx_string_ptr = get_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&prompt_time_select, &string);

	gx_utility_rectangle_define(&childSize, 40, 70, 50 + 79 - 1, 270 - 1);
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

	gx_utility_rectangle_define(&childSize, 190, 70, 190 + 79 - 1, 270 - 1);
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

	gx_utility_rectangle_define(&childSize, 12, 283, 12 + 296 - 1, 283 + 65 - 1);
	custom_rounded_button_create(&next_finish_button, NEXT_OR_FINISH_BUTTON_ID, parent, &childSize,
								 GX_COLOR_ID_HONBOW_BLUE, GX_PIXELMAP_ID_APP_ALARM__NEXT, NULL,
								 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
	app_alarm_page_reg(ALARM_PAGE_TIME_SEL, &alarm_time_select_page, alarm_time_sel_page_info_update);
}

static int alarm_time_sel_page_info_update(ALARM_EDIT_TYPE type, ALARM_INFO_T *info)
{
	edit_type = type;
	alarm_info = info;

	INT hour = info->hour;
	INT min = info->min;
	gx_scroll_wheel_selected_set(&hour_scroll_wheel, hour);
	gx_scroll_wheel_selected_set(&min_scroll_wheel, min);

	if (edit_type == ALARM_EDIT_TYPE_SINGLE) {
		next_finish_button.icon = GX_PIXELMAP_ID_APP_ALARM_CHECK_BOX_SELECTED;
	} else {
		next_finish_button.icon = GX_PIXELMAP_ID_APP_ALARM__NEXT;
	}
	gx_system_dirty_mark(&next_finish_button.widget);
	return 0;
}