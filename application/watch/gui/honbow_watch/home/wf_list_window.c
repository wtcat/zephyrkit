#include "wf_list_window.h"
#include "custom_horizontal_total_rows.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "gx_window.h"
#include "stdio.h"
#include "string.h"
#include "watch_face_manager.h"
#include "windows_manager.h"
typedef struct TIME_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	GX_WIDGET thumb;
	GX_PROMPT theme_prompt;
	UCHAR theme_index;
} WF_LIST_ROW;
WF_LIST_ROW wf_list_row_memory[WF_LIST_VISIBLE_COLUMNS + 1];
extern char test_name[6][10];
extern GX_WINDOW_ROOT *root;
VOID column_widget_draw(GX_WIDGET *widget)
{
#if 1
	gx_widget_children_draw(widget);
#else
	WF_LIST_ROW *column = CONTAINER_OF(widget, WF_LIST_ROW, widget);
	uint8_t theme_index = column->theme_index;
	GX_STRING string;
	string.gx_string_ptr = test_name[theme_index];
	string.gx_string_length = strlen(string.gx_string_ptr);
	// gx_prompt_text_set_ext(&column->theme_prompt, &string);

	GX_RECTANGLE dirty;
	dirty.gx_rectangle_left = widget->gx_widget_size.gx_rectangle_left + 10;
	dirty.gx_rectangle_right = widget->gx_widget_size.gx_rectangle_right - 10;
	dirty.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_top;
	dirty.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_bottom;

	gx_canvas_drawing_initiate(root->gx_window_root_canvas, widget, &dirty);
	gx_context_fill_color_set(GX_COLOR_ID_SILVERY_GRAY);
	gx_canvas_rectangle_draw(&dirty);

	GX_PIXELMAP *map = NULL;
#if 0
	wf_mgr_theme_thumb_get(theme_index, &map);
#else
	wf_mgr_theme_thumb_get(0, &map);
#endif
	if (map != NULL) {
		GX_RECTANGLE *rect = &widget->gx_widget_size;
		GX_VALUE top = rect->gx_rectangle_top + 80;
		GX_VALUE left = rect->gx_rectangle_left + 10;
		gx_canvas_pixelmap_draw(left, top, map);
	}
	gx_canvas_drawing_complete(root->gx_window_root_canvas, GX_FALSE);

	// gx_prompt_draw(&column->theme_prompt);
#endif
}

VOID thumb_widget_draw(GX_WIDGET *widget)
{
	gx_widget_draw(widget);

	WF_LIST_ROW *column = CONTAINER_OF(widget, WF_LIST_ROW, thumb);

	uint8_t theme_index = column->theme_index;

	GX_PIXELMAP *map = NULL;
#if 0
	wf_mgr_theme_thumb_get(theme_index, &map);
#else
	wf_mgr_theme_thumb_get(0, &map);
#endif

	if (map != NULL) {
		GX_RECTANGLE *rect = &widget->gx_widget_size;
		GX_VALUE top = rect->gx_rectangle_top;
		GX_VALUE left = rect->gx_rectangle_left;
		gx_canvas_pixelmap_draw(left, top, map);
		gx_widget_border_draw(widget, GX_COLOR_ID_HONBOW_GRAY, GX_COLOR_ID_HONBOW_GRAY, GX_COLOR_ID_HONBOW_GRAY, false);
	}
}

VOID wf_list_column_create(GX_HORIZONTAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	WF_LIST_ROW *column = (WF_LIST_ROW *)widget;
	GX_BOOL result;

	gx_widget_created_test(&column->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 219, 305);
		gx_widget_create(&column->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_fill_color_set(&column->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								 GX_COLOR_ID_HONBOW_BLACK);
		// gx_widget_draw_set(&column->widget, column_widget_draw);

#if 1
		gx_utility_rectangle_define(&childsize, 0, 20, 219, 59);
		gx_prompt_create(&column->theme_prompt, NULL, &column->widget, 0,
						 GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 0, &childsize);
		gx_prompt_text_color_set(&column->theme_prompt, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
								 GX_COLOR_ID_HONBOW_WHITE);
		gx_prompt_font_set(&column->theme_prompt, GX_FONT_ID_SIZE_26);
#endif
#if 1
		gx_utility_rectangle_define(&childsize, 10, 80, 209, 305);
		gx_widget_create(&column->thumb, NULL, &column->widget, GX_STYLE_ENABLED | GX_STYLE_BORDER_THIN, GX_ID_NONE,
						 &childsize);
		gx_widget_fill_color_set(&column->thumb, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								 GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_draw_set(&column->thumb, thumb_widget_draw);
#endif
	}

	column->theme_index = index;
}

void wf_list_refresh(GX_HORIZONTAL_LIST *list, UCHAR total_cnt)
{
	if (list->gx_horizontal_list_total_columns != total_cnt) {

		for (uint8_t i = 0; i < WF_LIST_VISIBLE_COLUMNS + 1; i++) {
			gx_widget_detach(&wf_list_row_memory[i].widget);
		}
		uint8_t cnt_child = total_cnt >= (WF_LIST_VISIBLE_COLUMNS + 1) ? (WF_LIST_VISIBLE_COLUMNS + 1) : total_cnt;
		for (uint8_t i = 0; i < cnt_child; i++) {
			gx_widget_attach((GX_WIDGET *)list, &wf_list_row_memory[i].widget);
		}
		custom_horizontal_list_total_rows_set(list, total_cnt);
	}
}
VOID wf_list_children_create(GX_HORIZONTAL_LIST *list)
{
	INT count;
	for (count = 0; count < WF_LIST_VISIBLE_COLUMNS + 1; count++) {
		wf_list_column_create(list, (GX_WIDGET *)&wf_list_row_memory[count], count);
	}
}

char test_name[6][10] = {"demo1", "demo2", "demo3", "demo4", "demo5", "demo6"};
VOID wf_list_callback(GX_HORIZONTAL_LIST *list, GX_WIDGET *widget, INT index)
{
	WF_LIST_ROW *column = (WF_LIST_ROW *)widget;

	column->theme_index = index;

	GX_STRING string;
	string.gx_string_ptr = test_name[index];
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&column->theme_prompt, &string);
}

extern VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
extern UINT _gx_system_input_release(GX_WIDGET *owner);
extern GX_WIDGET *_gx_system_top_widget_find(GX_WIDGET *root, GX_POINT test_point, ULONG status);
extern VOID _gx_horizontal_list_slide_back_check(GX_HORIZONTAL_LIST *list);
extern INT _gx_widget_client_index_get(GX_WIDGET *parent, GX_WIDGET *child);
extern UINT _gx_horizontal_list_selected_set(GX_HORIZONTAL_LIST *horizontal_list, INT index);
extern GX_WIDGET *_gx_widget_first_client_child_get(GX_WIDGET *parent);
extern GX_WIDGET *_gx_widget_next_client_child_get(GX_WIDGET *start);

#define WF_LIST_POS_ALIGN 60
VOID gx_horizontal_list_slide_back_custom_check(GX_HORIZONTAL_LIST *list)
{
	GX_WIDGET *child;
	INT page_index;

	if (list->gx_horizontal_list_child_count <= 0) {
		return;
	}

	/* Calculate page index. */
	page_index = list->gx_horizontal_list_top_index;
	list->gx_horizontal_list_snap_back_distance = 0;

	child = _gx_widget_first_client_child_get((GX_WIDGET *)list);
	bool first_child = true;
#if 0
	while (child) {
		GX_WIDGET *child_tmp;
		if (child->gx_widget_size.gx_rectangle_left >= 0) {
			if (child->gx_widget_size.gx_rectangle_left == WF_LIST_POS_ALIGN) {
				list->gx_horizontal_list_snap_back_distance = 0;
				return;
			} else {
				list->gx_horizontal_list_snap_back_distance =
					WF_LIST_POS_ALIGN - child->gx_widget_size.gx_rectangle_left;
			}
			break;
		}
		child_tmp = _gx_widget_next_client_child_get(child);
		if (child_tmp != NULL) {
			child = child_tmp;
		} else {
			break;
		}
	}

	if (list->gx_horizontal_list_snap_back_distance == 0) {
		list->gx_horizontal_list_snap_back_distance =
			WF_LIST_POS_ALIGN - child->gx_widget_size.gx_rectangle_left;
	}
#else
	GX_CANVAS *canvas_tmp;
	gx_widget_canvas_get((GX_WIDGET *)list, &canvas_tmp);
	GX_VALUE width = canvas_tmp->gx_canvas_x_resolution >> 1;
	GX_VALUE x_max = canvas_tmp->gx_canvas_x_resolution - 1;
	GX_VALUE x_min = 0;
	while (child) {
		GX_WIDGET *child_tmp;
		GX_VALUE left, right;
		left =
			child->gx_widget_size.gx_rectangle_left >= x_min
				? (child->gx_widget_size.gx_rectangle_left <= x_max ? child->gx_widget_size.gx_rectangle_left : x_max)
				: x_min;
		right =
			child->gx_widget_size.gx_rectangle_right >= x_min
				? (child->gx_widget_size.gx_rectangle_right <= x_max ? child->gx_widget_size.gx_rectangle_right : x_max)
				: x_min;

		if (((first_child == true) && ((right == x_max) || (left == x_max))) || (right - left + 1 >= width)) {
			list->gx_horizontal_list_snap_back_distance = WF_LIST_POS_ALIGN - child->gx_widget_size.gx_rectangle_left;
			break;
		}
		child_tmp = _gx_widget_next_client_child_get(child);
		if (first_child == true) {
			first_child = false;
		}
		if (child_tmp != NULL) {
			child = child_tmp;
		} else {
			list->gx_horizontal_list_snap_back_distance = WF_LIST_POS_ALIGN - child->gx_widget_size.gx_rectangle_left;
			break;
		}
	}
#endif
	gx_system_timer_start((GX_WIDGET *)list, GX_SNAP_TIMER, 1, 1);
}
UINT wf_list_event(GX_HORIZONTAL_LIST *list, GX_EVENT *event_ptr)
{
	GX_WIDGET *widget = (GX_WIDGET *)list;
	GX_WIDGET *child;
	INT new_pen_index;
	INT timer_id;
	INT snap_dist;
	UINT status = GX_SUCCESS;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
#if 0
        uint8_t total_cnts = wf_mgr_cnts_get();
#else // for test
		uint8_t total_cnts = 5;
#endif
		uint8_t curr_id = wf_mgr_get_default_id();
		wf_list_refresh(list, total_cnts);
		gx_horizontal_list_page_index_set(list, curr_id);
		gx_horizontal_list_slide_back_custom_check(list);
		break;
	}
	case GX_EVENT_HIDE:
		break;
	case GX_EVENT_TIMER:
		timer_id = (INT)(event_ptr->gx_event_payload.gx_event_timer_id);

		if (timer_id == GX_FLICK_TIMER || timer_id == GX_SNAP_TIMER) {
			if (GX_ABS(list->gx_horizontal_list_snap_back_distance) <=
				GX_ABS(list->gx_horizontal_list_child_width) / 3) {
				gx_system_timer_stop(widget, event_ptr->gx_event_payload.gx_event_timer_id);
				_gx_horizontal_list_scroll(list, list->gx_horizontal_list_snap_back_distance);

				if (event_ptr->gx_event_payload.gx_event_timer_id == GX_FLICK_TIMER) {
					gx_horizontal_list_slide_back_custom_check(list);
				}
			} else {
				snap_dist = (list->gx_horizontal_list_snap_back_distance) / 3;
				list->gx_horizontal_list_snap_back_distance =
					(GX_VALUE)(list->gx_horizontal_list_snap_back_distance - snap_dist);
				_gx_horizontal_list_scroll(list, snap_dist);
			}
		} else {
			status = _gx_window_event_process((GX_WINDOW *)list, event_ptr);
		}
		return 0;
	case GX_EVENT_PEN_DOWN: {
		status = gx_horizontal_list_event_process(list, event_ptr);
		return status;
	}
	case GX_EVENT_PEN_UP:
		if (list->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release(widget);

			gx_horizontal_list_slide_back_custom_check(list);

			if (list->gx_horizontal_list_pen_index >= 0 && list->gx_horizontal_list_snap_back_distance == 0) {
				/* test to see if pen-up is over same child widget as pen-down */
				child = _gx_system_top_widget_find((GX_WIDGET *)list, event_ptr->gx_event_payload.gx_event_pointdata,
												   GX_STATUS_SELECTABLE);
				while (child && child->gx_widget_parent != (GX_WIDGET *)list) {
					child = child->gx_widget_parent;
				}

				if (child) {
					new_pen_index = list->gx_horizontal_list_top_index + _gx_widget_client_index_get(widget, child);
					if (new_pen_index >= list->gx_horizontal_list_total_columns) {
						new_pen_index -= list->gx_horizontal_list_total_columns;
					}
					if (new_pen_index == list->gx_horizontal_list_pen_index) {
						_gx_horizontal_list_selected_set(list, list->gx_horizontal_list_pen_index);
					}
				} else {
					new_pen_index = 0;
				}
				// switch to watch face!
				windows_mgr_page_jump((GX_WINDOW *)list, NULL, WINDOWS_OP_BACK);

				if (new_pen_index <= wf_mgr_cnts_get()) {
					wf_mgr_theme_select(new_pen_index, (GX_WIDGET *)&root_window.root_window_wf_window);
				} else {
					wf_mgr_theme_select(WF_THEME_DEFAULT_ID, (GX_WIDGET *)&root_window.root_window_wf_window);
				}
			}
		} else {
			gx_widget_event_to_parent(widget, event_ptr);
		}
		return 0;
	}
	return gx_horizontal_list_event_process(list, event_ptr);
}