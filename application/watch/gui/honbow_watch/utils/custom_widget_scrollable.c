#include "custom_widget_scrollable.h"
#include "gx_window.h"
#include "gx_system.h"
#include "gx_widget.h"
#include "gx_window.h"
#include "logging/log.h"
LOG_MODULE_DECLARE(guix_log);

#define CUSTOM_SLIDE_BACK_SPEED 60
UINT custom_scrollable_widget_event_process(CUSTOM_SCROLLABLE_WIDGET *widget, GX_EVENT *event_ptr);
VOID custom_scrollable_widget_create(CUSTOM_SCROLLABLE_WIDGET *widget, GX_WIDGET *parent, GX_RECTANGLE *size)
{
	GX_WINDOW *window = (GX_WINDOW *)widget;
	gx_window_create(window, NULL, parent, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, size);

	gx_widget_fill_color_set(widget, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);

	gx_widget_event_process_set(widget, custom_scrollable_widget_event_process);
}

static GX_WIDGET *custom_first_visible_client_child_get(GX_WIDGET *parent)
{
	GX_WIDGET *test = parent->gx_widget_first_child;

	while (test && ((test->gx_widget_status & GX_STATUS_NONCLIENT) || !(test->gx_widget_status & GX_STATUS_VISIBLE))) {
		test = test->gx_widget_next;
	}
	return test;
}
static GX_WIDGET *custom_next_visible_client_child_get(GX_WIDGET *start)
{
	GX_WIDGET *child = start->gx_widget_next;

	while (child) {
		if (!(child->gx_widget_status & GX_STATUS_NONCLIENT) && (child->gx_widget_status & GX_STATUS_VISIBLE)) {
			break;
		}
		child = child->gx_widget_next;
	}
	return child;
}
static GX_WIDGET *custom_last_visible_client_child_get(GX_WIDGET *parent)
{
	GX_WIDGET *test = parent->gx_widget_last_child;

	while (test && ((test->gx_widget_status & GX_STATUS_NONCLIENT) || !(test->gx_widget_status & GX_STATUS_VISIBLE))) {
		test = test->gx_widget_previous;
	}
	return test;
}
#define TIMER_ID_CUSTOM_SNAP 1
#define TIMER_ID_CUSTOM_FLICK 2

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);
extern GX_WIDGET *_gx_widget_first_client_child_get(GX_WIDGET *parent);
extern GX_WIDGET *_gx_widget_next_client_child_get(GX_WIDGET *start);
GX_WIDGET *_gx_widget_last_client_child_get(GX_WIDGET *parent);

UINT custom_child_info_get(GX_WIDGET *widget, GX_VALUE *child_top_min, GX_VALUE *child_bottom_max)
{
	if ((child_top_min == NULL) || (child_bottom_max == NULL)) {
		return -1;
	}

	GX_WIDGET *child;
	child = custom_first_visible_client_child_get((GX_WIDGET *)widget);
	if (child == NULL) {
		return -1;
	}
	*child_top_min = child->gx_widget_size.gx_rectangle_top;
	*child_bottom_max = child->gx_widget_size.gx_rectangle_bottom;
	while (child) {
		child = custom_next_visible_client_child_get(child);

		if (child != NULL) {
			if (child->gx_widget_size.gx_rectangle_top < *child_top_min) {
				*child_top_min = child->gx_widget_size.gx_rectangle_top;
			}
			if (child->gx_widget_size.gx_rectangle_bottom > *child_bottom_max) {
				*child_bottom_max = child->gx_widget_size.gx_rectangle_bottom;
			}
		}
	}
	return 0;
}

static VOID custom_slide_back_check(CUSTOM_SCROLLABLE_WIDGET *custom_widget)
{
	GX_WIDGET *widget = (GX_WIDGET *)custom_widget;
	GX_VALUE top_min;
	GX_VALUE bottom_max;
	if (0 != custom_child_info_get((GX_WIDGET *)widget, &top_min, &bottom_max)) {
		return;
	}

	if (top_min < widget->gx_widget_size.gx_rectangle_top) {
		if (bottom_max < widget->gx_widget_size.gx_rectangle_bottom) {
			custom_widget->gx_snap_back_distance = widget->gx_widget_size.gx_rectangle_bottom - bottom_max;
		} else {
			custom_widget->gx_snap_back_distance = 0;
		}
	} else {
		custom_widget->gx_snap_back_distance = widget->gx_widget_size.gx_rectangle_top - top_min;
	}
	if (custom_widget->gx_snap_back_distance)
		gx_system_timer_start(widget, TIMER_ID_CUSTOM_SNAP, 1, 1);
}

UINT custom_scrollable_widget_event_process(CUSTOM_SCROLLABLE_WIDGET *custom_widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	INT timer_id;
	INT snap_dist;
	GX_WIDGET **stackptr;
	GX_EVENT input_release_event;
	GX_WINDOW *widget = (GX_WINDOW *)custom_widget;
	INT widget_height = widget->gx_widget_size.gx_rectangle_bottom - widget->gx_widget_size.gx_rectangle_top + 1;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN:
		gx_widget_event_to_parent(widget, event_ptr);
		if (widget->gx_widget_style & GX_STYLE_ENABLED) {
			_gx_system_input_capture((GX_WIDGET *)widget);
			widget->gx_window_move_start = event_ptr->gx_event_payload.gx_event_pointdata; // record
			gx_system_timer_stop(widget, TIMER_ID_CUSTOM_SNAP);
			return status;
		}
		break;
	case GX_EVENT_PEN_UP:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release((GX_WIDGET *)widget);
			custom_slide_back_check(custom_widget);
		}
		gx_widget_event_to_parent(widget, event_ptr);
		return status;
	case GX_EVENT_VERTICAL_FLICK: {
		GX_VALUE delta = (GX_VALUE)(GX_FIXED_VAL_TO_INT(event_ptr->gx_event_payload.gx_event_intdata[0]) * 8);
		GX_VALUE child_top_min, child_bottom_max;
		if (0 != custom_child_info_get((GX_WIDGET *)widget, &child_top_min, &child_bottom_max)) {
			return 0;
		}
		GX_VALUE valid_height = child_bottom_max - child_top_min + 1;
		if (valid_height <= widget_height) {
			return 0;
		}
		if (delta > 0) {
			if (child_top_min + delta > widget->gx_widget_size.gx_rectangle_bottom) {
				delta = widget->gx_widget_size.gx_rectangle_bottom - child_top_min;
			}
		} else {
			if (child_bottom_max + delta < widget->gx_widget_size.gx_rectangle_top) {
				delta = widget->gx_widget_size.gx_rectangle_top - child_bottom_max;
			}
		}
		custom_widget->gx_snap_back_distance = delta;
		gx_system_timer_start(widget, TIMER_ID_CUSTOM_FLICK, 1, 1);
		return status;
	}
	case GX_EVENT_PEN_DRAG: {

		if ((widget->gx_widget_status & GX_STATUS_OWNS_INPUT)) {

			GX_VALUE child_top_min, child_bottom_max;
			if (0 != custom_child_info_get((GX_WIDGET *)widget, &child_top_min, &child_bottom_max)) {
				return 0;
			}
			GX_VALUE valid_height = child_bottom_max - child_top_min + 1;
			if (valid_height <= widget_height) {
				return 0;
			}
			GX_VALUE delta =
				event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y - widget->gx_window_move_start.gx_point_y;

			if (delta > 0) {
				if (child_top_min + delta > widget->gx_widget_size.gx_rectangle_bottom) {
					delta = widget->gx_widget_size.gx_rectangle_bottom - child_top_min;
				}
			} else if (child_bottom_max + delta < widget->gx_widget_size.gx_rectangle_top) {
				delta = widget->gx_widget_size.gx_rectangle_top - child_bottom_max;
			}

			if (delta != 0) {
				/* Start sliding, remove other widgets from input capture stack.  */
				stackptr = _gx_system_input_capture_stack;
				memset(&input_release_event, 0, sizeof(GX_EVENT));
				input_release_event.gx_event_type = GX_EVENT_INPUT_RELEASE;

				while (*stackptr) {
					if (*stackptr != (GX_WIDGET *)widget) {
						input_release_event.gx_event_target = *stackptr;
						gx_system_event_send(&input_release_event);
					}
					stackptr++;
				}

				gx_window_client_scroll(widget, 0, delta);
				widget->gx_window_move_start = event_ptr->gx_event_payload.gx_event_pointdata; // record
			}
		}
		return status;
	}
	case GX_EVENT_INPUT_RELEASE:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release((GX_WIDGET *)widget);
		}
		return status;
	case GX_EVENT_TIMER:
		timer_id = (INT)(event_ptr->gx_event_payload.gx_event_timer_id);
		if (((timer_id == TIMER_ID_CUSTOM_SNAP) || (timer_id == TIMER_ID_CUSTOM_FLICK)) &&
			(event_ptr->gx_event_target == (GX_WIDGET *)custom_widget)) {
			if (GX_ABS(custom_widget->gx_snap_back_distance) < CUSTOM_SLIDE_BACK_SPEED) {
				gx_system_timer_stop(widget, event_ptr->gx_event_payload.gx_event_timer_id);
				gx_window_client_scroll(widget, 0, custom_widget->gx_snap_back_distance);
				custom_widget->gx_snap_back_distance = 0;
				if (timer_id == TIMER_ID_CUSTOM_FLICK) {
					custom_slide_back_check(custom_widget);
				}
			} else {
				if (custom_widget->gx_snap_back_distance > 0) {
					snap_dist = CUSTOM_SLIDE_BACK_SPEED;
				} else {
					snap_dist = -CUSTOM_SLIDE_BACK_SPEED;
				}
				custom_widget->gx_snap_back_distance = (GX_VALUE)(custom_widget->gx_snap_back_distance - snap_dist);
				gx_window_client_scroll(widget, 0, snap_dist);
			}
			return status;
		}
		break;
	default:
		break;
	}
	return _gx_window_event_process(widget, event_ptr);
}

void custom_scrollable_widget_reset(CUSTOM_SCROLLABLE_WIDGET *custom_widget)
{
	GX_WIDGET *child;
	child = custom_first_visible_client_child_get((GX_WIDGET *)custom_widget);
	if (child == NULL) {
		return;
	}
	GX_VALUE top = child->gx_widget_size.gx_rectangle_top;
	GX_VALUE bottom = child->gx_widget_size.gx_rectangle_bottom;
	GX_VALUE widget_height = custom_widget->widget.gx_widget_size.gx_rectangle_bottom -
							 custom_widget->widget.gx_widget_size.gx_rectangle_top + 1;

	while (child) {
		child = custom_next_visible_client_child_get(child);

		if (child != NULL) {
			if (child->gx_widget_size.gx_rectangle_top < top) {
				top = child->gx_widget_size.gx_rectangle_top;
			}
			if (child->gx_widget_size.gx_rectangle_bottom > bottom) {
				bottom = child->gx_widget_size.gx_rectangle_bottom;
			}
		}
	}
	GX_VALUE valid_height = bottom - top + 1;

	if (valid_height <= widget_height) {
		GX_VALUE delta = custom_widget->widget.gx_widget_size.gx_rectangle_top - top;

		gx_window_client_scroll(&custom_widget->widget, 0, delta);
	}

	GX_SCROLLBAR *pScroll;

	gx_window_scrollbar_find((GX_WINDOW *)custom_widget, GX_TYPE_VERTICAL_SCROLL, &pScroll);

	if (pScroll) {
		gx_scrollbar_reset(pScroll, GX_NULL);
	}
}