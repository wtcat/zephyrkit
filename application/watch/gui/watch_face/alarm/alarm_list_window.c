#include "../guix_simple_resources.h"
#include "../guix_simple_specifications.h"
#include "../utils/custom_checkbox.h"
#include "../utils/custom_util.h"
#include "./alarm_list_ctl.h"
#include "gx_api.h"
#include "gx_button.h"
#include "gx_prompt.h"
#include "gx_widget.h"
#include "gx_window.h"
#include "sys/printk.h"
#include <stdint.h>
#include <stdio.h>

#define ALARM_LIST_VISIBLE_ROWS 4

#define SWITCH_BTTON_ID 0xf0

typedef struct TIME_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	CUSTOM_CHECKBOX checkbox;
	GX_PROMPT time_hour;
	GX_PROMPT time_minute;
	GX_PROMPT time_am_pm;
	GX_ICON dot_up;
	GX_ICON dot_down;
	UCHAR index;
} ALARM_LIST_ROW;

ALARM_LIST_ROW alarm_list_row_memory[ALARM_LIST_VISIBLE_ROWS + 1];

CUSTOM_CHECKBOX_INFO checkbox_info = {SWITCH_BTTON_ID,
									  GX_PIXELMAP_ID_SWITCH_BG,
									  GX_PIXELMAP_ID_SWITCH_ACTIVE,
									  GX_PIXELMAP_ID_SWITCH_DISACTIVE,
									  4,
									  24};

extern GX_WINDOW_ROOT *root;

/*************************************************************************************/
VOID row_widget_draw(GX_WIDGET *widget)
{
	GX_RECTANGLE dirty;
	gx_widget_background_draw(widget);

	dirty.gx_rectangle_left = widget->gx_widget_size.gx_rectangle_left + 10;
	dirty.gx_rectangle_right = widget->gx_widget_size.gx_rectangle_right - 10;
	dirty.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_bottom - 7;
	dirty.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_bottom - 7;

	gx_canvas_drawing_initiate(root->gx_window_root_canvas, widget, &dirty);
	gx_context_fill_color_set(GX_COLOR_ID_BLUE);
	gx_canvas_rectangle_draw(&dirty);
	gx_canvas_drawing_complete(root->gx_window_root_canvas, GX_FALSE);

	gx_widget_children_draw(widget);
}

extern UINT vertical_list_child_event(GX_WIDGET *widget, GX_EVENT *event_ptr);
/*************************************************************************************/
VOID alarm_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	ALARM_LIST_ROW *row = (ALARM_LIST_ROW *)widget;
	GX_BOOL result;
	GX_STRING string;

	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 252, 75);
		gx_widget_create(&row->widget, NULL, list,
						 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
						 &childsize);
		gx_widget_draw_set(&row->widget, row_widget_draw);
		gx_widget_event_process_set(&row->widget, vertical_list_child_event);

		gx_utility_rectangle_define(&childsize, 20, 15, 77, 47);
		custom_checkbox_create(&row->checkbox, &row->widget, &checkbox_info,
							   &childsize);

		gx_utility_rectangle_define(&childsize, 130, 5, 175, 43);
		gx_prompt_create(&row->time_hour, NULL, &row->widget, 0,
						 GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_RIGHT |
							 GX_STYLE_BORDER_NONE,
						 SWITCH_BTTON_ID, &childsize);
		gx_prompt_text_color_set(&row->time_hour, GX_COLOR_ID_MILK_WHITE,
								 GX_COLOR_ID_MILK_WHITE,
								 GX_COLOR_ID_MILK_WHITE);
		gx_prompt_font_set(&row->time_hour, GX_FONT_ID_SIZE_40);

		gx_utility_rectangle_define(&childsize, 171, 0, 190, 40);
		gx_icon_create(&row->dot_up, NULL, &row->widget, GX_PIXELMAP_ID_DOT,
					   GX_STYLE_TRANSPARENT, 0, 180, 15);
		gx_icon_create(&row->dot_down, NULL, &row->widget, GX_PIXELMAP_ID_DOT,
					   GX_STYLE_TRANSPARENT, 0, 180, 30);
		gx_widget_fill_color_set((GX_WIDGET *)&row->dot_up, GX_COLOR_ID_WHITE,
								 GX_COLOR_ID_WHITE, GX_COLOR_ID_WHITE);
		gx_widget_fill_color_set((GX_WIDGET *)&row->dot_down, GX_COLOR_ID_WHITE,
								 GX_COLOR_ID_WHITE, GX_COLOR_ID_WHITE);

		gx_utility_rectangle_define(&childsize, 190, 5, 245, 43);
		gx_prompt_create(&row->time_minute, NULL, &row->widget, 0,
						 GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT |
							 GX_STYLE_BORDER_NONE,
						 0, &childsize);
		gx_prompt_text_color_set(&row->time_minute, GX_COLOR_ID_MILK_WHITE,
								 GX_COLOR_ID_MILK_WHITE,
								 GX_COLOR_ID_MILK_WHITE);
		gx_prompt_font_set(&row->time_minute, GX_FONT_ID_SIZE_40);

		gx_utility_rectangle_define(&childsize, 200, 40, 240, 70);
		gx_prompt_create(&row->time_am_pm, NULL, &row->widget, 0,
						 GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_RIGHT |
							 GX_STYLE_BORDER_NONE,
						 0, &childsize);
		gx_prompt_text_color_set(&row->time_am_pm, GX_COLOR_ID_MILK_WHITE,
								 GX_COLOR_ID_MILK_WHITE,
								 GX_COLOR_ID_MILK_WHITE);
		gx_prompt_font_set(&row->time_am_pm, GX_FONT_ID_SYSTEM);
	}
	ALARM_INFO *alarm_info;
	alarm_list_ctl_get_info(index, &alarm_info);

	/* binding widget to alarm info */
	row->index = index;

	if (alarm_info) {
		gx_widget_show(&row->widget); // test
		// gx_utility_rectangle_define(&childsize,
		// row->widget.gx_widget_size.gx_rectangle_left,
		// row->widget.gx_widget_size.gx_rectangle_top,
		//                         row->widget.gx_widget_size.gx_rectangle_left+252,
		//                         row->widget.gx_widget_size.gx_rectangle_top+75);
		// gx_widget_resize(&row->widget, &childsize);

		/* Set AM/PM label. */
		gx_prompt_text_id_set(&row->time_am_pm, alarm_info->alarm_am_pm);

		/* Set hour. */
		string.gx_string_ptr = alarm_info->alarm_hour_text;
		string.gx_string_length =
			string_length_get(string.gx_string_ptr, MAX_TIME_TEXT_LENGTH);
		gx_prompt_text_set_ext(&row->time_hour, &string);

		/* Set minute. */
		string.gx_string_ptr = alarm_info->alarm_minute_text;
		string.gx_string_length =
			string_length_get(string.gx_string_ptr, MAX_TIME_TEXT_LENGTH);
		gx_prompt_text_set_ext(&row->time_minute, &string);

		if (alarm_info->alarm_status == ALARM_ON) {
			gx_widget_style_add((GX_WIDGET *)&row->checkbox,
								GX_STYLE_BUTTON_PUSHED);
		} else {
			gx_widget_style_remove((GX_WIDGET *)&row->checkbox,
								   GX_STYLE_BUTTON_PUSHED);
		}
	} else {
		gx_widget_hide(&row->widget); // test
									  // gx_utility_rectangle_define(&childsize,
		// row->widget.gx_widget_size.gx_rectangle_left,
		// row->widget.gx_widget_size.gx_rectangle_top,
		//                         row->widget.gx_widget_size.gx_rectangle_left,
		//                         row->widget.gx_widget_size.gx_rectangle_top);
		// gx_widget_resize(&row->widget, &childsize);
	}
}

/*************************************************************************************/
VOID alarm_list_children_create(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < ALARM_LIST_VISIBLE_ROWS + 1; count++) {
		alarm_list_row_create(list, (GX_WIDGET *)&alarm_list_row_memory[count],
							  count);
	}
}

extern void alarm_add_del_test(void);
extern void alarm_table_refresh(void);
/*************************************************************************************/
/* Handle alarm add screen */
/*************************************************************************************/
UINT alarm_add_screen_event_handler(GX_WINDOW *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(ID_ADD, GX_EVENT_CLICKED):
		alarm_add_del_test();
		alarm_table_refresh();
		break;
	}

	gx_window_event_process(window, event_ptr);
	return 0;
}
UINT alarm_add_event(GX_PIXELMAP_BUTTON *widget, GX_EVENT *event_ptr)
{
	return gx_pixelmap_button_event_process(widget, event_ptr);
}

void alarm_add_del_test(void)
{
#if 1
	static uint8_t test = 1;

	ALARM_INFO alarm_add;
	alarm_add.avalid = 0xaa;
	alarm_add.alarm_am_pm = GX_STRING_ID_TIME_AM;
	char time[3];

	snprintf(time, 3, "%02d", test);

	memcpy(alarm_add.alarm_hour_text, time, 3);
	memcpy(alarm_add.alarm_minute_text, time, 3);
	alarm_add.alarm_repeat = 0x7f;
	alarm_add.alarm_status = ALARM_ON;
	alarm_list_ctl_add(alarm_add);

	test++;
#else
	UCHAR total_cnt, max_cnt;
	alarm_list_ctl_get_total(&total_cnt, &max_cnt);
	if (total_cnt > 0) {
		alarm_list_ctl_del(1);
	}
#endif
}

#include "custom_vertical_total_rows.h"
void alarm_table_refresh(void)
{
	UCHAR total_cnt, max_cnt;

	alarm_list_ctl_get_total(&total_cnt, &max_cnt);

	if (alarm_window.alarm_window_alarm_list.gx_vertical_list_total_rows !=
		total_cnt) {

		for (uint8_t i = 0; i < ALARM_LIST_VISIBLE_ROWS + 1; i++) {
			gx_widget_detach(&alarm_list_row_memory[i].widget);
		}
		uint8_t cnt_child = total_cnt >= (ALARM_LIST_VISIBLE_ROWS + 1)
								? (ALARM_LIST_VISIBLE_ROWS + 1)
								: total_cnt;
		for (uint8_t i = 0; i < cnt_child; i++) {
			gx_widget_attach((GX_WIDGET *)&alarm_window.alarm_window_alarm_list,
							 &alarm_list_row_memory[i].widget);
		}

		custom_vertical_list_total_rows_set(
			&alarm_window.alarm_window_alarm_list, total_cnt);
	}
}

void ui_alarm_callback(void *data)
{
	if (data != NULL) {
		printk("alarm coming, need switch to new window!\n");
		printk("ALARM INFO: hour: %s MIN: %s\n",
			   ((ALARM_INFO *)data)->alarm_hour_text,
			   ((ALARM_INFO *)data)->alarm_minute_text);
	}
}

/*************************************************************************************/
/* Handler alarm screen */
/*************************************************************************************/
UINT alarm_screen_event_handler(GX_WINDOW *window, GX_EVENT *event_ptr)
{

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		alarm_list_ctl_init();
		alarm_table_refresh();
		break;
	default:
		break;
	}

	return gx_window_event_process(window, event_ptr);
}

UINT alarm_list_win_event(GX_VERTICAL_LIST *widget, GX_EVENT *event_ptr)
{
	gx_vertical_list_event_process(widget, event_ptr);
	return 0;
}

UINT vertical_list_child_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(SWITCH_BTTON_ID, GX_EVENT_TOGGLE_ON): {
		ALARM_LIST_ROW *row = (ALARM_LIST_ROW *)widget;
		UCHAR index = row->index;
		alarm_list_ctl_status_change(index, ALARM_ON);
	} break;

	case GX_SIGNAL(SWITCH_BTTON_ID, GX_EVENT_TOGGLE_OFF): {
		ALARM_LIST_ROW *row = (ALARM_LIST_ROW *)widget;
		UCHAR index = row->index;
		alarm_list_ctl_status_change(index, ALARM_OFF);
	} break;

	default:
		gx_widget_event_process(widget, event_ptr);
		break;
	}
	return 0;
}