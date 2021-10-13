#include "custom_slide_delete_widget.h"
#include "gx_system.h"
#include "sys/util.h"
#include "zephyr.h"
#define MIN_DELTA 20
#define CUSTOM_LANDING_TIMER_ID 1

UINT custom_deletable_slide_widget_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	static GX_VALUE delta_x, delta_y;
	static GX_BOOL enter_slide_flag = GX_FALSE;
	INT snap_dist;
	GX_WIDGET **stackptr;
	GX_EVENT input_release_event;

	CUSTOM_SLIDE_DEL_WIDGET *custom_widget = CONTAINER_OF(widget, CUSTOM_SLIDE_DEL_WIDGET, widget_main);
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		gx_widget_first_child_get(widget, &custom_widget->child1);
		gx_widget_last_child_get(widget, &custom_widget->child2);
		return gx_widget_event_process(widget, event_ptr);
	case GX_EVENT_PEN_DOWN:
		if (widget->gx_widget_style & GX_STYLE_ENABLED) {
			_gx_system_input_capture(widget);
			custom_widget->widget_move_start = event_ptr->gx_event_payload.gx_event_pointdata;
		}
		if (custom_widget->child1->gx_widget_size.gx_rectangle_left == widget->gx_widget_size.gx_rectangle_left) {
			status = gx_widget_event_to_parent(widget, event_ptr);
		}
		break;
	case GX_EVENT_PEN_UP:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release(widget);
			enter_slide_flag = GX_FALSE;
			if (custom_widget->child2->gx_widget_size.gx_rectangle_right <= widget->gx_widget_size.gx_rectangle_right) {
				custom_widget->snap_back_distance = widget->gx_widget_size.gx_rectangle_right -
													custom_widget->child2->gx_widget_size.gx_rectangle_right;
			} else {
				custom_widget->snap_back_distance =
					widget->gx_widget_size.gx_rectangle_left - custom_widget->child1->gx_widget_size.gx_rectangle_left;
			}

			if (custom_widget->snap_back_distance) {
				gx_system_timer_start(widget, CUSTOM_LANDING_TIMER_ID, 1, 1);
			}
		}
		// need check clicked event happended or not
		status = gx_widget_event_to_parent(widget, event_ptr);
		break;
	case GX_EVENT_TIMER: {
		if ((event_ptr->gx_event_payload.gx_event_timer_id == CUSTOM_LANDING_TIMER_ID) &&
			(event_ptr->gx_event_target == widget)) {
			if (GX_ABS(custom_widget->snap_back_distance) < 50) {
				snap_dist = custom_widget->snap_back_distance;
				custom_widget->snap_back_distance = 0;
				gx_system_timer_stop(widget, CUSTOM_LANDING_TIMER_ID);
			} else {
				snap_dist = custom_widget->snap_back_distance / 5;
				custom_widget->snap_back_distance = custom_widget->snap_back_distance - snap_dist;
			}
			gx_widget_shift(custom_widget->child1, snap_dist, 0, GX_TRUE);
			gx_widget_shift(custom_widget->child2, snap_dist, 0, GX_TRUE);
			return status;
		}
	} break;
	case GX_EVENT_PEN_DRAG:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			delta_y =
				event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y - custom_widget->widget_move_start.gx_point_y;
			delta_x =
				event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - custom_widget->widget_move_start.gx_point_x;
			if ((GX_ABS(delta_x) > MIN_DELTA) || (GX_ABS(delta_y) > MIN_DELTA)) {
				if ((GX_ABS(delta_y) > GX_ABS(delta_x)) && (enter_slide_flag == GX_FALSE)) {
					status = gx_widget_event_to_parent(widget, event_ptr);
				} else {
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

					if (custom_widget->child1->gx_widget_size.gx_rectangle_left + delta_x >
						widget->gx_widget_size.gx_rectangle_left) {
						delta_x = widget->gx_widget_size.gx_rectangle_left -
								  custom_widget->child1->gx_widget_size.gx_rectangle_left;
					}

					gx_widget_shift(custom_widget->child1, delta_x, 0, GX_TRUE);
					gx_widget_shift(custom_widget->child2, delta_x, 0, GX_TRUE);
					custom_widget->widget_move_start = event_ptr->gx_event_payload.gx_event_pointdata;
					if (enter_slide_flag == GX_FALSE)
						enter_slide_flag = GX_TRUE;
				}
			}
			break;
		}
	case GX_EVENT_INPUT_RELEASE:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release(widget);
		}
		break;
	default:
		return gx_widget_event_process(widget, event_ptr);
	}
	return status;
}

void custom_deletable_slide_widget_reset(CUSTOM_SLIDE_DEL_WIDGET *widget, GX_POINT *point, GX_BOOL force_flag)
{
	if ((widget->child1 != NULL) && (widget->child2 != NULL)) {
		if ((GX_FALSE == force_flag)) {
			if (NULL != point) {
				if (gx_utility_rectangle_point_detect(&widget->widget_main.gx_widget_size, *point)) {
					return;
				}
			} else {
				return;
			}
			if (widget->child1->gx_widget_size.gx_rectangle_left !=
				widget->widget_main.gx_widget_size.gx_rectangle_left) {
				if (widget->snap_back_distance == 0) {
					widget->snap_back_distance = widget->widget_main.gx_widget_size.gx_rectangle_left -
												 widget->child1->gx_widget_size.gx_rectangle_left;
					if (widget->snap_back_distance) {
						gx_system_timer_start(widget, CUSTOM_LANDING_TIMER_ID, 1, 1);
						gx_system_dirty_mark(&widget->widget_main);
					}
				}
			}
		} else {
			if (widget->snap_back_distance == 0) {
				GX_VALUE delta = widget->widget_main.gx_widget_size.gx_rectangle_left -
								 widget->child1->gx_widget_size.gx_rectangle_left;
				gx_widget_shift(widget->child1, delta, 0, GX_FALSE);
				gx_widget_shift(widget->child2, delta, 0, GX_FALSE);
			}
		}
	}
}

/******************************************************
		***************
		*    parent   *
		***************

		*************** ***********
		*    child1   * * child2  *
		*************** ***********
	child1一般与父窗口有相同的size，即默认显示child1
	child2一般默认不显示(显示区域外)，在拖动时候会显示出来。
 ******************************************************/
void custom_deletable_slide_widget_create(CUSTOM_SLIDE_DEL_WIDGET *widget, USHORT widget_id, GX_WIDGET *parent,
										  GX_RECTANGLE *size, GX_WIDGET *child1, GX_WIDGET *child2)
{
	gx_widget_create(&widget->widget_main, NULL, parent, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, widget_id, size);
	gx_widget_fill_color_set(&widget->widget_main, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);
	gx_widget_event_process_set(&widget->widget_main, custom_deletable_slide_widget_event_process);

	gx_widget_attach(&widget->widget_main, child1);
	gx_widget_attach(&widget->widget_main, child2);

	widget->child1 = child1;
	widget->child2 = child2;
}