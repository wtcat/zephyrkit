
#include "gx_api.h"
#include "gx_system.h"
#include "sys/util.h"
#include "gx_window.h"
extern GX_WIDGET *_gx_widget_first_client_child_get(GX_WIDGET *parent);
extern GX_WIDGET *_gx_widget_last_client_child_get(GX_WIDGET *parent);
extern VOID _gx_vertical_list_scroll(GX_VERTICAL_LIST *list, INT amount);
extern INT _gx_widget_client_index_get(GX_WIDGET *parent, GX_WIDGET *child);
extern UINT _gx_vertical_list_selected_set(GX_VERTICAL_LIST *vertical_list, INT index);
extern GX_WIDGET *_gx_widget_next_client_child_get(GX_WIDGET *start);
extern UINT _gx_scrollbar_reset(GX_SCROLLBAR *scrollbar, GX_SCROLL_INFO *info);

static VOID custom_vertical_list_slide_back_check(GX_VERTICAL_LIST *list)
{
	GX_WIDGET *child;
	GX_WIDGET *child_record;

	INT page_index;

	if (list->gx_vertical_list_child_count <= 0) {
		return;
	}

	/* Calculate page index. */
	page_index = list->gx_vertical_list_top_index;
	child = _gx_widget_first_client_child_get((GX_WIDGET *)list);

	while (child) {
		if ((child->gx_widget_size.gx_rectangle_bottom > list->gx_widget_size.gx_rectangle_top) ||
			(child->gx_widget_size.gx_rectangle_top > list->gx_widget_size.gx_rectangle_top)) {
			break;
		} else {
			page_index++;
			child = _gx_widget_next_client_child_get(child);
		}
	}

	list->gx_vertical_list_snap_back_distance = 0;

	if (page_index == 0) {
		child = _gx_widget_first_client_child_get((GX_WIDGET *)list);
		if (child && (child->gx_widget_size.gx_rectangle_bottom - list->gx_window_client.gx_rectangle_top) >=
						 (list->gx_vertical_list_child_height >> 1)) {
			list->gx_vertical_list_snap_back_distance =
				(GX_VALUE)(list->gx_window_client.gx_rectangle_top - child->gx_widget_size.gx_rectangle_top);
			_gx_system_timer_start((GX_WIDGET *)list, GX_SNAP_TIMER, 1, 1);
		} else {
			if (child)
				child = _gx_widget_next_client_child_get(child);
			if (child) {
				list->gx_vertical_list_snap_back_distance =
					(GX_VALUE)(list->gx_window_client.gx_rectangle_top - child->gx_widget_size.gx_rectangle_top);
				_gx_system_timer_start((GX_WIDGET *)list, GX_SNAP_TIMER, 1, 1);
			}
		}
	} else {
		if (list->gx_vertical_list_top_index + list->gx_vertical_list_child_count >=
			list->gx_vertical_list_total_rows - 1) {
			child_record = child;
			child = _gx_widget_last_client_child_get((GX_WIDGET *)list);
			if (child && (child->gx_widget_size.gx_rectangle_bottom < list->gx_window_client.gx_rectangle_bottom)) {
				list->gx_vertical_list_snap_back_distance =
					(GX_VALUE)(list->gx_window_client.gx_rectangle_bottom - child->gx_widget_size.gx_rectangle_bottom);
				_gx_system_timer_start((GX_WIDGET *)list, GX_SNAP_TIMER, 1, 1);
			} else {
				child = child_record;
				if (child && (child->gx_widget_size.gx_rectangle_bottom - list->gx_window_client.gx_rectangle_top) >=
								 (list->gx_vertical_list_child_height >> 1)) {
					list->gx_vertical_list_snap_back_distance =
						(GX_VALUE)(list->gx_window_client.gx_rectangle_top - child->gx_widget_size.gx_rectangle_top);
					_gx_system_timer_start((GX_WIDGET *)list, GX_SNAP_TIMER, 1, 1);
				} else {
					if (child)
						child = _gx_widget_next_client_child_get(child);
					if (child) {
						list->gx_vertical_list_snap_back_distance = (GX_VALUE)(list->gx_window_client.gx_rectangle_top -
																			   child->gx_widget_size.gx_rectangle_top);
						_gx_system_timer_start((GX_WIDGET *)list, GX_SNAP_TIMER, 1, 1);
					}
				}
			}
		}
	}
}
UINT custom_vertical_event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	GX_VERTICAL_LIST *list = (GX_VERTICAL_LIST *)widget;

	GX_SCROLLBAR *pScroll;
	GX_WIDGET *child;
	INT list_height;
	INT widget_height;
	GX_WIDGET **stackptr;
	GX_EVENT input_release_event;
	INT new_pen_index;
	INT page_index;
	GX_VALUE list_top;
	GX_VALUE list_bottom;
	GX_VALUE delta = 0;
	UINT timer_id;
	INT snap_dist;
	UINT status = GX_SUCCESS;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN:
		_gx_system_input_capture(widget);
		list->gx_window_move_start = event_ptr->gx_event_payload.gx_event_pointdata;

		/* determine which child widget has been selected for future list select event */
		child = _gx_system_top_widget_find((GX_WIDGET *)list, event_ptr->gx_event_payload.gx_event_pointdata,
										   GX_STATUS_SELECTABLE);

		while (child && child->gx_widget_parent != (GX_WIDGET *)list) {
			child = child->gx_widget_parent;
		}

		if (child) {
			list->gx_vertical_list_pen_index =
				list->gx_vertical_list_top_index + _gx_widget_client_index_get(widget, child);
			if (list->gx_vertical_list_pen_index >= list->gx_vertical_list_total_rows) {
				list->gx_vertical_list_pen_index -= list->gx_vertical_list_total_rows;
			}
		}
		gx_widget_event_to_parent((GX_WIDGET *)list, event_ptr);
		return status;
	case GX_EVENT_PEN_DRAG:
		if ((widget->gx_widget_status & GX_STATUS_OWNS_INPUT) &&
			(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y - list->gx_window_move_start.gx_point_y) != 0) {
			list_height = list->gx_vertical_list_child_count * list->gx_vertical_list_child_height;
			widget_height = list->gx_window_client.gx_rectangle_bottom - list->gx_window_client.gx_rectangle_top + 1;

			if (list_height > widget_height) {
				stackptr = _gx_system_input_capture_stack;
				memset(&input_release_event, 0, sizeof(GX_EVENT));
				input_release_event.gx_event_type = GX_EVENT_INPUT_RELEASE;

				while (*stackptr) {
					if (*stackptr != widget) {
						input_release_event.gx_event_target = *stackptr;
						_gx_system_event_send(&input_release_event);
					}
					stackptr++;
				}

				page_index = list->gx_vertical_list_top_index;
				child = _gx_widget_first_client_child_get((GX_WIDGET *)list);

				if (!child) {
					return GX_FALSE;
				}

				list_top = child->gx_widget_size.gx_rectangle_top - page_index * list->gx_vertical_list_child_height;
				list_bottom =
					child->gx_widget_size.gx_rectangle_bottom +
					(list->gx_vertical_list_total_rows - 1 - page_index) * list->gx_vertical_list_child_height;
				delta =
					event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y - list->gx_window_move_start.gx_point_y;
				GX_VALUE limit = list->gx_vertical_list_child_height - (list->gx_vertical_list_child_height >> 2);
				if (delta >= 0) {
					if ((list_top + delta) >= widget->gx_widget_size.gx_rectangle_top + limit) {
						delta = widget->gx_widget_size.gx_rectangle_top + limit - list_top;
					}
				} else {
					if ((list_bottom + delta) <= widget->gx_widget_size.gx_rectangle_bottom - limit) {
						delta = widget->gx_widget_size.gx_rectangle_bottom - limit - list_bottom;
					}
				}
				_gx_vertical_list_scroll(list, delta);
				gx_window_scrollbar_find((GX_WINDOW *)list, GX_TYPE_VERTICAL_SCROLL, &pScroll);

				if (pScroll) {
					gx_scrollbar_reset(pScroll, GX_NULL);
				}
				list->gx_window_move_start = event_ptr->gx_event_payload.gx_event_pointdata;
				list->gx_vertical_list_pen_index = -1;
			}
		} else {
			gx_widget_event_to_parent((GX_WIDGET *)list, event_ptr);
		}
		return GX_SUCCESS;
	case GX_EVENT_PEN_UP:
		if (list->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release(widget);

			list_height = list->gx_vertical_list_child_count * list->gx_vertical_list_child_height;
			widget_height = list->gx_window_client.gx_rectangle_bottom - list->gx_window_client.gx_rectangle_top + 1;

			if (list_height > widget_height) {
				custom_vertical_list_slide_back_check(list);
			}
			if (list->gx_vertical_list_pen_index >= 0 && list->gx_vertical_list_snap_back_distance == 0) {
				/* test to see if pen-up is over same child widget as pen-down */
				child = _gx_system_top_widget_find((GX_WIDGET *)list, event_ptr->gx_event_payload.gx_event_pointdata,
												   GX_STATUS_SELECTABLE);
				while (child && child->gx_widget_parent != (GX_WIDGET *)list) {
					child = child->gx_widget_parent;
				}

				if (child) {
					new_pen_index = list->gx_vertical_list_top_index + _gx_widget_client_index_get(widget, child);
					if (new_pen_index >= list->gx_vertical_list_total_rows) {
						new_pen_index -= list->gx_vertical_list_total_rows;
					}
					if (new_pen_index == list->gx_vertical_list_pen_index) {
						_gx_vertical_list_selected_set(list, list->gx_vertical_list_pen_index);
					}
				}
			}
		}
		gx_widget_event_to_parent((GX_WIDGET *)list, event_ptr);
		return GX_SUCCESS;
	case GX_EVENT_VERTICAL_FLICK:
		list_height = list->gx_vertical_list_child_count * list->gx_vertical_list_child_height;
		widget_height = list->gx_window_client.gx_rectangle_bottom - list->gx_window_client.gx_rectangle_top + 1;
		if (list_height > widget_height) {
			page_index = list->gx_vertical_list_top_index;
			child = _gx_widget_first_client_child_get((GX_WIDGET *)list);
			if (!child) {
				return GX_FALSE;
			}
			list_top = child->gx_widget_size.gx_rectangle_top - page_index * list->gx_vertical_list_child_height;
			list_bottom = child->gx_widget_size.gx_rectangle_bottom +
						  (list->gx_vertical_list_total_rows - 1 - page_index) * list->gx_vertical_list_child_height;
			GX_VALUE limit = list->gx_vertical_list_child_height - (list->gx_vertical_list_child_height >> 2);
			delta = (GX_VALUE)(GX_FIXED_VAL_TO_INT(event_ptr->gx_event_payload.gx_event_intdata[0]) * 8);
			if (delta >= 0) {
				if ((list_top + delta) >= widget->gx_widget_size.gx_rectangle_top + limit) {
					delta = widget->gx_widget_size.gx_rectangle_top + limit - list_top;
				}
			} else {
				if ((list_bottom + delta) <= widget->gx_widget_size.gx_rectangle_bottom - limit) {
					delta = widget->gx_widget_size.gx_rectangle_bottom - limit - list_bottom;
				}
			}
			list->gx_vertical_list_snap_back_distance = delta;
			_gx_system_timer_start((GX_WIDGET *)list, GX_FLICK_TIMER, 1, 1);
		}
		list->gx_vertical_list_pen_index = -1;
		return GX_SUCCESS;
	case GX_EVENT_VERTICAL_SCROLL:
		return GX_SUCCESS;
	case GX_EVENT_TIMER:
		timer_id = event_ptr->gx_event_payload.gx_event_timer_id;
		if (timer_id == GX_FLICK_TIMER || timer_id == GX_SNAP_TIMER) {
			if (GX_ABS(list->gx_vertical_list_snap_back_distance) < GX_ABS(list->gx_vertical_list_child_height) / 3) {
				_gx_system_timer_stop(widget, event_ptr->gx_event_payload.gx_event_timer_id);
				_gx_vertical_list_scroll(list, list->gx_vertical_list_snap_back_distance);

				if (event_ptr->gx_event_payload.gx_event_timer_id == GX_FLICK_TIMER) {
					custom_vertical_list_slide_back_check(list);
				}
			} else {
				snap_dist = (list->gx_vertical_list_snap_back_distance) / 5;
				list->gx_vertical_list_snap_back_distance =
					(GX_VALUE)(list->gx_vertical_list_snap_back_distance - snap_dist);
				_gx_vertical_list_scroll(list, snap_dist);
			}
			_gx_window_scrollbar_find((GX_WINDOW *)list, GX_TYPE_VERTICAL_SCROLL, &pScroll);

			if (pScroll) {
				_gx_scrollbar_reset(pScroll, GX_NULL);
			}
		} else {
			status = _gx_window_event_process((GX_WINDOW *)list, event_ptr);
		}
		return status;

	default:
		break;
	}
	return gx_vertical_list_event_process(list, event_ptr);
}