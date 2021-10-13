#include "gx_api.h"
#include "home_window.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "custom_rounded_button.h"
#include "custom_window_title.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include <stdio.h>
#include "windows_manager.h"
#include "data_service_custom.h"
#include "view_service_custom.h"
#include "view_service_stop_watch.h"
#include "custom_vertical_scroll_limit.h"
#include "logging/log.h"
#include "custom_vertical_total_rows.h"
LOG_MODULE_DECLARE(guix_log);

extern VOID custom_circle_pixelmap_button_draw(GX_PIXELMAP_BUTTON *button);
typedef struct APP_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	GX_PIXELMAP_BUTTON id_tips;
	GX_PROMPT id_tips_prompt;
	GX_PROMPT record_info;
} APP_SETTING_LIST_ROW;

static GX_WIDGET curr_widget;
static GX_PROMPT stop_watch_value;
static GX_CHAR buff_for_display[16];
static GX_WIDGET splite_line;
static GX_VERTICAL_LIST record_list;
#define APP_STOP_WATCH_LIST_VISIBLE_ROWS 3
static APP_SETTING_LIST_ROW app_setting_list_row_memory[APP_STOP_WATCH_LIST_VISIBLE_ROWS + 1];
static STOP_WATCH_INFO_T *g_info = NULL;
static Custom_RoundedButton_TypeDef count_and_reset_button;
static Custom_RoundedButton_TypeDef start_and_pause_button;
#define BUTTON_COUNT_RESET_ID 1
#define BUTTON_START_PAUSE_ID 2
#define CLOCK_TIMER_ID 1
#define TIMER_FOR_APP_ID 2
static UINT stop_watch_page_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static WM_QUIT_JUDGE_T pen_info_judge;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		view_service_stop_watch_info_sync();
		break;
	case GX_SIGNAL(BUTTON_COUNT_RESET_ID, GX_EVENT_CLICKED): {
		GX_CHAR op = STOP_WATCH_OP_MARK_OR_RESUME;
		view_service_event_submit(VIEW_EVENT_TYPE_STOP_WATCH_OP, &op, 1);
	} break;
	case GX_SIGNAL(BUTTON_START_PAUSE_ID, GX_EVENT_CLICKED): {
		GX_CHAR op = STOP_WATCH_OP_STATUS_CHG;
		view_service_event_submit(VIEW_EVENT_TYPE_STOP_WATCH_OP, &op, 1);
	} break;
	case GX_EVENT_PEN_DOWN:
		wm_point_down_record(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata);
		break;
	case GX_EVENT_PEN_UP:
		if (wm_quit_judge(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata) == GX_TRUE) {
			windows_mgr_page_jump((GX_WINDOW *)&app_stop_watch_window, NULL, WINDOWS_OP_BACK);
		}
		break;
	case DATA_SERVICE_EVT_STOP_WATCH_INFO_CHG: {
		STOP_WATCH_INFO_T *info;
		info = (STOP_WATCH_INFO_T *)event_ptr->gx_event_payload.gx_event_intdata[0];
		g_info = info;
		TIME_INFO_T info_time;
		util_ticks_to_time(info->total_ticks, &info_time);
		GX_STRING init_value;
		snprintf(buff_for_display, sizeof(buff_for_display), "%02d:%02d:%02d.%d", info_time.hour, info_time.min,
				 info_time.sec, info_time.tenth_sec);
		init_value.gx_string_ptr = buff_for_display;
		init_value.gx_string_length = strlen(init_value.gx_string_ptr);
		gx_prompt_text_set_ext(&stop_watch_value, &init_value);
		if (info->status == STOP_WATCH_STATUS_RUNNING) {
			if (start_and_pause_button.icon != GX_PIXELMAP_ID_APP_SPORT_BTN_S_STOP) {
				start_and_pause_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_S_PAUSE;
				gx_system_dirty_mark(&start_and_pause_button);
			}
			if (info->counts < MAX_STOP_WATCH_RECORD) {
				if ((count_and_reset_button.icon != GX_PIXELMAP_ID_WIDGETS_COUNT_ICON) ||
					(count_and_reset_button.disable_flag)) {
					count_and_reset_button.icon = GX_PIXELMAP_ID_WIDGETS_COUNT_ICON;
					count_and_reset_button.disable_flag = 0;
					gx_system_dirty_mark(&count_and_reset_button);
				}
			} else {
				if ((count_and_reset_button.icon != GX_PIXELMAP_ID_WIDGETS_COUNT_ICON) ||
					(!count_and_reset_button.disable_flag)) {
					count_and_reset_button.icon = GX_PIXELMAP_ID_WIDGETS_COUNT_ICON;
					count_and_reset_button.disable_flag = 1;
					gx_system_dirty_mark(&count_and_reset_button);
				}
			}
		} else if (info->status == STOP_WATCH_STATUS_INIT) {
			if (start_and_pause_button.icon != GX_PIXELMAP_ID_WIDGETS_START_ICON) {
				start_and_pause_button.icon = GX_PIXELMAP_ID_WIDGETS_START_ICON;
				gx_system_dirty_mark(&start_and_pause_button);
			}
			if ((count_and_reset_button.icon != GX_PIXELMAP_ID_WIDGETS_COUNT_ICON) ||
				(!count_and_reset_button.disable_flag)) {
				count_and_reset_button.icon = GX_PIXELMAP_ID_WIDGETS_COUNT_ICON;
				count_and_reset_button.disable_flag = 1;
				gx_system_dirty_mark(&count_and_reset_button);
			}
		} else if (info->status == STOP_WATCH_STATUS_STOPPED) {
			if (start_and_pause_button.icon != GX_PIXELMAP_ID_WIDGETS_START_ICON) {
				start_and_pause_button.icon = GX_PIXELMAP_ID_WIDGETS_START_ICON;
				gx_system_dirty_mark(&start_and_pause_button);
			}
			if ((count_and_reset_button.icon != GX_PIXELMAP_ID_APP_TIMER_BTN_S_REFRESH) ||
				(count_and_reset_button.disable_flag)) {
				count_and_reset_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_S_REFRESH;
				count_and_reset_button.disable_flag = 0;
				gx_system_dirty_mark(&count_and_reset_button);
			}
		} else {
			LOG_ERR("app stop watch unkonwn status!");
		}
		uint8_t count = info->counts;
		if (record_list.gx_vertical_list_total_rows != count) {
			for (uint8_t i = 0; i < APP_STOP_WATCH_LIST_VISIBLE_ROWS + 1; i++) {
				gx_widget_detach(&app_setting_list_row_memory[i].widget);
			}
			uint8_t cnt_child =
				count >= (APP_STOP_WATCH_LIST_VISIBLE_ROWS + 1) ? (APP_STOP_WATCH_LIST_VISIBLE_ROWS + 1) : count;
			for (uint8_t i = 0; i < cnt_child; i++) {
				gx_widget_attach((GX_WIDGET *)&record_list, &app_setting_list_row_memory[i].widget);
			}
			custom_vertical_list_total_rows_set(&record_list, count);
		}
		return 0;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}
static void splite_line_draw_func(GX_WIDGET *widget)
{
	gx_context_brush_define(GX_COLOR_ID_HONBOW_GRAY, GX_COLOR_ID_HONBOW_GRAY, 0);
	gx_context_brush_width_set(1);

	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left, widget->gx_widget_size.gx_rectangle_top,
						widget->gx_widget_size.gx_rectangle_right, widget->gx_widget_size.gx_rectangle_bottom);
}
static VOID record_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	APP_SETTING_LIST_ROW *row = (APP_SETTING_LIST_ROW *)widget;
	GX_BOOL result;

	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 295, 42 - 1);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);

		GX_WIDGET *parent = &row->widget;
		gx_utility_rectangle_define(&childsize, 0, 0, 40 - 1, 40 - 1);
		gx_pixelmap_button_create(&row->id_tips, NULL, parent, GX_PIXELMAP_NULL, GX_PIXELMAP_NULL, GX_PIXELMAP_NULL,
								  GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_VALIGN_CENTER |
									  GX_STYLE_HALIGN_CENTER,
								  GX_ID_NONE, &childsize);
		gx_widget_fill_color_set(&row->id_tips, GX_COLOR_ID_HONBOW_BLUE, GX_COLOR_ID_HONBOW_BLUE,
								 GX_COLOR_ID_HONBOW_BLUE);
		gx_widget_draw_set(&row->id_tips, custom_circle_pixelmap_button_draw);

		gx_prompt_create(&row->id_tips_prompt, NULL, &row->id_tips, GX_ID_NONE,
						 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, GX_ID_NONE, &childsize);
		gx_prompt_font_set(&row->id_tips_prompt, GX_FONT_ID_SIZE_26);
		gx_prompt_text_color_set(&row->id_tips_prompt, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
								 GX_COLOR_ID_HONBOW_WHITE);

		GX_STRING init_value;
		init_value.gx_string_ptr = "0";
		init_value.gx_string_length = strlen(init_value.gx_string_ptr);
		gx_prompt_text_set_ext(&row->id_tips_prompt, &init_value);

		gx_utility_rectangle_define(&childsize, 50, 0, 295, 40 - 1);
		gx_prompt_create(&row->record_info, NULL, parent, GX_ID_NONE,
						 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT, GX_ID_NONE, &childsize);
		gx_prompt_font_set(&row->record_info, GX_FONT_ID_SIZE_30);
		gx_prompt_text_color_set(&row->record_info, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
								 GX_COLOR_ID_HONBOW_WHITE);

		init_value.gx_string_ptr = "00:00:00.0";
		init_value.gx_string_length = strlen(init_value.gx_string_ptr);
		gx_prompt_text_set_ext(&row->record_info, &init_value);
	}
}

static VOID app_setting_list_children_create(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < APP_STOP_WATCH_LIST_VISIBLE_ROWS + 1; count++) {
		record_list_row_create(list, (GX_WIDGET *)&app_setting_list_row_memory[count], count);
	}
}

static GX_CHAR buff_for_id_tips[MAX_STOP_WATCH_RECORD][4];
static GX_CHAR buff_for_record[MAX_STOP_WATCH_RECORD][12];
static VOID record_list_callback(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_SETTING_LIST_ROW *row = CONTAINER_OF(widget, APP_SETTING_LIST_ROW, widget);
	GX_STRING init_value;
	if (g_info != NULL) {
		snprintf(buff_for_id_tips[index], sizeof(buff_for_id_tips[index]), "%d", g_info->counts - index);
		init_value.gx_string_ptr = buff_for_id_tips[index];
		init_value.gx_string_length = strlen(init_value.gx_string_ptr);
		gx_prompt_text_set_ext(&row->id_tips_prompt, &init_value);

		TIME_INFO_T info_time;
		util_ticks_to_time(g_info->counts_value[g_info->counts - index - 1], &info_time);
		snprintf(buff_for_record[index], sizeof(buff_for_record[index]), "%02d:%02d:%02d.%d", info_time.hour,
				 info_time.min, info_time.sec, info_time.tenth_sec);
		init_value.gx_string_ptr = buff_for_record[index];
		init_value.gx_string_length = strlen(init_value.gx_string_ptr);
		gx_prompt_text_set_ext(&row->record_info, &init_value);
	}
}
void widget_stopwatch_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_widget, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_widget_event_process_set(&curr_widget, stop_watch_page_event_process);

	GX_WIDGET *parent = &curr_widget;

	gx_utility_rectangle_define(&childSize, 12, 283, 12 + 143 - 1, 283 + 65 - 1);
	custom_rounded_button_create_3(&count_and_reset_button, BUTTON_COUNT_RESET_ID, &curr_widget, &childSize,
								   GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_ID_WIDGETS_COUNT_ICON, NULL, GX_ID_NONE,
								   CUSTOM_ROUNDED_BUTTON_TEXT_LEFT, 20, 180);

	gx_utility_rectangle_define(&childSize, 165, 283, 165 + 143 - 1, 283 + 65 - 1);
	custom_rounded_button_create(&start_and_pause_button, BUTTON_START_PAUSE_ID, &curr_widget, &childSize,
								 GX_COLOR_ID_HONBOW_BLUE, GX_PIXELMAP_ID_WIDGETS_START_ICON, NULL,
								 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	gx_utility_rectangle_define(&childSize, 0, 68, 319, 68 + 60 - 1);
	gx_prompt_create(&stop_watch_value, GX_NULL, &curr_widget, GX_ID_NONE,
					 GX_STYLE_TRANSPARENT | GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TEXT_CENTER, GX_ID_NONE,
					 &childSize);
	gx_prompt_text_color_set(&stop_watch_value, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&stop_watch_value, GX_FONT_ID_SIZE_60);
	GX_STRING init_value;
	init_value.gx_string_ptr = "00:00:00.0";
	init_value.gx_string_length = strlen(init_value.gx_string_ptr);
	gx_prompt_text_set_ext(&stop_watch_value, &init_value);

	gx_utility_rectangle_define(&childSize, 12, 140, 12 + 296 - 1, 140);
	gx_widget_create(&splite_line, NULL, &curr_widget, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_widget_draw_set(&splite_line, splite_line_draw_func);

	gx_utility_rectangle_define(&childSize, 60, 151, 320 - 60 - 1, 276);
	gx_vertical_list_create(&record_list, NULL, parent, MAX_STOP_WATCH_RECORD, record_list_callback,
							GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &childSize);
	gx_widget_fill_color_set(&record_list, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);
	app_setting_list_children_create(&record_list);
	gx_widget_event_process_set(&record_list, custom_vertical_event_processing_function);

	for (uint8_t i = 0; i < APP_STOP_WATCH_LIST_VISIBLE_ROWS + 1; i++) {
		gx_widget_detach(&app_setting_list_row_memory[i].widget);
	}
	custom_vertical_list_total_rows_set(&record_list, 0);
	home_widget_type_reg(&curr_widget, HOME_WIDGET_TYPE_STOPWATCH);
}
