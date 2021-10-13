#include "custom_checkbox_basic.h"
#include "sys/util.h"

#define CUSTOM_CHECKBOX_TIMER 2

void custom_checkbox_basic_draw_func(GX_WIDGET *widget)
{
	CUSTOM_CHECKBOX_BASIC *checkbox = CONTAINER_OF(widget, CUSTOM_CHECKBOX_BASIC, widget);
	GX_RESOURCE_ID bg_clolor;

	if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		bg_clolor = checkbox->background_checked;
	} else {
		bg_clolor = checkbox->background_uchecked;
	}

	gx_context_brush_define(bg_clolor, bg_clolor, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);

	uint16_t width = abs(widget->gx_widget_size.gx_rectangle_bottom - widget->gx_widget_size.gx_rectangle_top) + 1;
	gx_context_brush_width_set(width);

	uint16_t temp = (width >> 1) + 1;
	// draw bg color
	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + temp, widget->gx_widget_size.gx_rectangle_top + temp,
						widget->gx_widget_size.gx_rectangle_right - temp,
						widget->gx_widget_size.gx_rectangle_top + temp);

	// draw rolling ball
	gx_context_brush_define(checkbox->rolling_ball_color, checkbox->rolling_ball_color,
							GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
	gx_context_brush_width_set(1);
	INT r = checkbox->rolling_ball_r;
	INT x = widget->gx_widget_size.gx_rectangle_left + r + checkbox->pos_offset + checkbox->curr_offset;
	INT y = widget->gx_widget_size.gx_rectangle_top +
			((widget->gx_widget_size.gx_rectangle_bottom - widget->gx_widget_size.gx_rectangle_top + 1) >> 1);
	gx_canvas_circle_draw(x, y, r);
}

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);
UINT custom_checkbox_basic_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	CUSTOM_CHECKBOX_BASIC *checkbox = CONTAINER_OF(widget, CUSTOM_CHECKBOX_BASIC, widget);
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN:
	case GX_EVENT_PEN_DRAG:
	case GX_EVENT_PEN_UP:
		// do not process this message. only support change from externnal.
		status = gx_widget_event_to_parent(widget, event_ptr);
		return status;
	case GX_EVENT_TIMER:
		if ((event_ptr->gx_event_payload.gx_event_timer_id == CUSTOM_CHECKBOX_TIMER) &&
			(event_ptr->gx_event_target == widget)) {
			if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
				checkbox->curr_offset += 6;
				if (widget->gx_widget_size.gx_rectangle_left + checkbox->curr_offset + (checkbox->rolling_ball_r << 1) +
						(checkbox->pos_offset << 1) >=
					widget->gx_widget_size.gx_rectangle_right) {
					checkbox->curr_offset = widget->gx_widget_size.gx_rectangle_right - (checkbox->pos_offset << 1) -
											(checkbox->rolling_ball_r << 1) - widget->gx_widget_size.gx_rectangle_left;
					gx_system_timer_stop(checkbox, CUSTOM_CHECKBOX_TIMER);
					gx_widget_event_generate((GX_WIDGET *)checkbox, GX_EVENT_TOGGLE_ON, 0);
				}
			} else {
				checkbox->curr_offset -= 6;
				if (checkbox->curr_offset <= 0) {
					checkbox->curr_offset = 0;
					gx_system_timer_stop(checkbox, CUSTOM_CHECKBOX_TIMER);
					gx_widget_event_generate((GX_WIDGET *)checkbox, GX_EVENT_TOGGLE_OFF, 0);
				}
			}
			gx_system_dirty_mark(checkbox);
			return status;
		}
		break;
	default:
		break;
	}
	return gx_widget_event_process((GX_WIDGET *)checkbox, event_ptr);
}

void custom_checkbox_basic_toggle(CUSTOM_CHECKBOX_BASIC *checkbox)
{
	if (!(checkbox->widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED)) {
		checkbox->widget.gx_widget_style |= GX_STYLE_BUTTON_PUSHED;
	} else {
		checkbox->widget.gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
	}
	gx_system_timer_start(checkbox, CUSTOM_CHECKBOX_TIMER, 1, 1);
}

VOID custom_checkbox_basic_create(CUSTOM_CHECKBOX_BASIC *checkbox, GX_WIDGET *parent, CUSTOM_CHECKBOX_BASIC_INFO *info,
								  GX_RECTANGLE *size)
{
	gx_widget_create(&checkbox->widget, "custom checkbox", parent, GX_STYLE_ENABLED, info->widget_id, size);
	gx_widget_draw_set(&checkbox->widget, custom_checkbox_basic_draw_func);

	gx_widget_event_process_set(&checkbox->widget, custom_checkbox_basic_event_process);

	checkbox->background_checked = info->background_checked;
	checkbox->background_uchecked = info->background_uchecked;
	checkbox->rolling_ball_color = info->rolling_ball_color;
	checkbox->curr_offset = 0;
	checkbox->pos_offset = info->pos_offset;
	checkbox->effect_from_external = GX_FALSE;
	checkbox->rolling_ball_r = ((size->gx_rectangle_bottom - size->gx_rectangle_top + 1) >> 1) - 1 - 4;
}