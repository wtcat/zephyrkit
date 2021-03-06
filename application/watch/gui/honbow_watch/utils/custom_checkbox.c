#include "custom_checkbox.h"
#include "gx_api.h"

#define CUSTOM_CHECKBOX_TIMER 2

VOID custom_checkbox_draw(CUSTOM_CHECKBOX *checkbox);
UINT custom_checkbox_event_process(CUSTOM_CHECKBOX *checkbox, GX_EVENT *event_ptr);
VOID custom_checkbox_select(GX_CHECKBOX *checkbox);

/*************************************************************************************/
VOID custom_checkbox_create(CUSTOM_CHECKBOX *checkbox, GX_WIDGET *parent, const CUSTOM_CHECKBOX_INFO *info,
							GX_RECTANGLE *size)
{
	gx_checkbox_create((GX_CHECKBOX *)checkbox, "", parent, GX_NULL, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT,
					   info->widget_id, size);

	checkbox->gx_button_select_handler = (VOID(*)(GX_WIDGET *))custom_checkbox_select;
	checkbox->gx_widget_draw_function = (VOID(*)(GX_WIDGET *))custom_checkbox_draw;
	checkbox->gx_widget_event_process_function = (UINT(*)(GX_WIDGET *, GX_EVENT *))custom_checkbox_event_process;
	checkbox->background_id = info->background_id;
	checkbox->cur_offset = info->start_offset;
	checkbox->start_offset = info->start_offset;
	checkbox->end_offset = info->end_offset;
	checkbox->gx_checkbox_checked_pixelmap_id = info->checked_map_id;
	checkbox->gx_checkbox_unchecked_pixelmap_id = info->unchecked_map_id;
	checkbox->state = CHECKBOX_STATE_NONE;
}

/*************************************************************************************/
VOID custom_checkbox_draw(CUSTOM_CHECKBOX *checkbox)
{
	GX_PIXELMAP *map;
	INT ypos;

	gx_context_pixelmap_get(checkbox->background_id, &map);

	if (map) {
		gx_canvas_pixelmap_draw(checkbox->gx_widget_size.gx_rectangle_left, checkbox->gx_widget_size.gx_rectangle_top,
								map);
	}

	if (checkbox->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		gx_context_pixelmap_get(checkbox->gx_checkbox_checked_pixelmap_id, &map);

		if (map) {
			ypos = (checkbox->gx_widget_size.gx_rectangle_bottom - checkbox->gx_widget_size.gx_rectangle_top + 1);
			ypos -= map->gx_pixelmap_width;
			ypos /= 2;
			ypos += checkbox->gx_widget_size.gx_rectangle_top;

			if (checkbox->state == CHECKBOX_STATE_NONE) {
				gx_canvas_pixelmap_draw(checkbox->gx_widget_size.gx_rectangle_right - map->gx_pixelmap_width -
											checkbox->cur_offset,
										ypos, map);
			} else {
				gx_canvas_pixelmap_draw(checkbox->gx_widget_size.gx_rectangle_left + checkbox->cur_offset, ypos, map);
			}
		}
	} else {
		gx_context_pixelmap_get(checkbox->gx_checkbox_unchecked_pixelmap_id, &map);

		if (map) {
			ypos = (checkbox->gx_widget_size.gx_rectangle_bottom - checkbox->gx_widget_size.gx_rectangle_top + 1);
			ypos -= map->gx_pixelmap_width;
			ypos /= 2;
			ypos += checkbox->gx_widget_size.gx_rectangle_top;

			if (checkbox->state == CHECKBOX_STATE_NONE) {
				gx_canvas_pixelmap_draw(checkbox->gx_widget_size.gx_rectangle_left + checkbox->cur_offset, ypos, map);
			} else {
				gx_canvas_pixelmap_draw(checkbox->gx_widget_size.gx_rectangle_right - map->gx_pixelmap_width -
											checkbox->cur_offset,
										ypos, map);
			}
		}
	}
}

/*************************************************************************************/
VOID custom_checkbox_select(GX_CHECKBOX *checkbox)
{
	if (!(checkbox->gx_widget_style & GX_STYLE_BUTTON_PUSHED)) {
		checkbox->gx_widget_style |= GX_STYLE_BUTTON_PUSHED;
	} else {
		checkbox->gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
	}
}

/*************************************************************************************/
extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);
UINT custom_checkbox_event_process(CUSTOM_CHECKBOX *checkbox, GX_EVENT *event_ptr)
{
	UINT status = 0;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN:
		if ((checkbox->gx_widget_style & GX_STYLE_ENABLED) && !(checkbox->flag_effect_from_external)) {
			_gx_system_input_capture((GX_WIDGET *)checkbox);
		}
		status = gx_widget_event_to_parent((GX_WIDGET *)checkbox, event_ptr);
		break;
	case GX_EVENT_PEN_UP:
		if (checkbox->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release((GX_WIDGET *)checkbox);
			if (gx_utility_rectangle_point_detect(&checkbox->gx_widget_size,
												  event_ptr->gx_event_payload.gx_event_pointdata)) {
				checkbox->gx_button_select_handler((GX_WIDGET *)checkbox);
				checkbox->state = CHECKBOX_STATE_SLIDING;

				gx_system_timer_start(checkbox, CUSTOM_CHECKBOX_TIMER, 1, 1);
			}
			gx_system_dirty_mark((GX_WIDGET *)checkbox);
		}
		// need check clicked event happended or not
		status = gx_widget_event_to_parent((GX_WIDGET *)checkbox, event_ptr);
		break;
	case GX_EVENT_INPUT_RELEASE:
		if (checkbox->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release((GX_WIDGET *)checkbox);
			checkbox->gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
			gx_system_dirty_mark((GX_WIDGET *)checkbox);
		}
		break;
	case GX_EVENT_TIMER:
		if (event_ptr->gx_event_payload.gx_event_timer_id == CUSTOM_CHECKBOX_TIMER) {
			checkbox->cur_offset += 6;

			if (checkbox->cur_offset >= checkbox->end_offset) {
				checkbox->cur_offset = checkbox->start_offset;
				gx_system_timer_stop(checkbox, CUSTOM_CHECKBOX_TIMER);
				checkbox->state = CHECKBOX_STATE_NONE;

				if (checkbox->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
					gx_widget_event_generate((GX_WIDGET *)checkbox, GX_EVENT_TOGGLE_ON, 0);
				} else {
					gx_widget_event_generate((GX_WIDGET *)checkbox, GX_EVENT_TOGGLE_OFF, 0);
				}
			}

			gx_system_dirty_mark(checkbox);
		}
		break;

	default:
		return gx_widget_event_process((GX_WIDGET *)checkbox, event_ptr);
	}

	return 0;
}

void custom_checkbox_status_change(CUSTOM_CHECKBOX *checkbox)
{
	if (checkbox->flag_effect_from_external) {
		checkbox->gx_button_select_handler((GX_WIDGET *)checkbox);
		checkbox->state = CHECKBOX_STATE_SLIDING;

		gx_system_timer_start(checkbox, CUSTOM_CHECKBOX_TIMER, 1, 1);
		gx_system_dirty_mark((GX_WIDGET *)checkbox);
	}
}

void custom_checkbox_event_from_external_enable(CUSTOM_CHECKBOX *checkbox, GX_BOOL flag)
{
	checkbox->flag_effect_from_external = flag;
}