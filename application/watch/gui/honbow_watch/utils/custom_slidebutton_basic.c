#include "custom_slidebutton_basic.h"
#include "gx_system.h"

#define CUSTOM_SLIDEBUTTON_TIMER 1
#define CONTAINER_OF(ptr, type, field) ((type *)(((char *)(ptr)) - offsetof(type, field)))

static VOID custom_slide_button_update(Custom_SlideButton_TypeDef *button)
{
	GX_PIXELMAP *map = NULL;
	gx_widget_pixelmap_get((GX_WIDGET *)button, button->slide_icon, &map);
	if (map) {
		if (button->icon_width != map->gx_pixelmap_width) {
			button->icon_width = map->gx_pixelmap_width;
		}
	}
}

UINT custom_slide_button_event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	GX_WIDGET **stackptr;
	GX_EVENT input_release_event;

	Custom_SlideButton_TypeDef *button = CONTAINER_OF(widget, Custom_SlideButton_TypeDef, widget);
	GX_RECTANGLE *rect = &widget->gx_widget_size;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		status = gx_widget_event_process(widget, event_ptr);
		custom_slide_button_update(button);
	} break;
	case GX_EVENT_PEN_DOWN:
		if (widget->gx_widget_style & GX_STYLE_ENABLED) {
			_gx_system_input_capture((GX_WIDGET *)widget);
			button->move_start = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		} else {
			status = gx_widget_event_to_parent((GX_WIDGET *)widget, event_ptr);
		}
		break;
	case GX_EVENT_PEN_UP:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release((GX_WIDGET *)widget);
			gx_system_dirty_mark((GX_WIDGET *)widget);
			if (button->icon_width != 0) {
				if (rect->gx_rectangle_left + button->cur_offset >=
					rect->gx_rectangle_right - button->icon_width - button->pos_offset) {
					gx_widget_event_generate(widget, GX_EVENT_CLICKED, 0);
				} else {
					gx_system_timer_start(widget, CUSTOM_SLIDEBUTTON_TIMER, 1, 1);
				}
			}
		} else {
			status = gx_widget_event_to_parent((GX_WIDGET *)widget, event_ptr);
		}
		break;
	case GX_EVENT_PEN_DRAG: {
		GX_VALUE delta = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - button->move_start;
		GX_RECTANGLE *rect = &widget->gx_widget_size;
		if ((widget->gx_widget_status & GX_STATUS_OWNS_INPUT) && (delta != 0)) {
			if (button->icon_width != 0) {
				button->cur_offset += delta;
				if (button->cur_offset < button->pos_offset) {
					button->cur_offset = button->pos_offset;
				}
				if (rect->gx_rectangle_left + button->cur_offset >=
					rect->gx_rectangle_right - button->icon_width - button->pos_offset) {
					button->cur_offset =
						rect->gx_rectangle_right - button->icon_width - button->pos_offset - rect->gx_rectangle_left;
				}
			}

			button->move_start = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;

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
			gx_system_dirty_mark(widget);
		}
		break;
	}
	case GX_EVENT_INPUT_RELEASE:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release((GX_WIDGET *)widget);
			widget->gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
			gx_system_dirty_mark((GX_WIDGET *)widget);
		}
		break;
	case GX_EVENT_TIMER:
		if (event_ptr->gx_event_payload.gx_event_timer_id == CUSTOM_SLIDEBUTTON_TIMER) {
			GX_VALUE steps = (rect->gx_rectangle_right - rect->gx_rectangle_left + 1) >> 2;
			if (button->cur_offset > button->pos_offset) {
				button->cur_offset -= steps;
				if (button->cur_offset < button->pos_offset) {
					button->cur_offset = button->pos_offset;
					gx_system_timer_stop(widget, CUSTOM_SLIDEBUTTON_TIMER);
				}
			}
			gx_system_dirty_mark(widget);
		} else {
			return gx_widget_event_process(widget, event_ptr);
		}
		break;
	default:
		status = gx_widget_event_process(widget, event_ptr);
		break;
	}
	return status;
}

void custom_slide_button_draw(GX_WIDGET *widget)
{
	Custom_SlideButton_TypeDef *button = CONTAINER_OF(widget, Custom_SlideButton_TypeDef, widget);
	GX_RECTANGLE *rect = &widget->gx_widget_size;
	GX_RESOURCE_ID bg_color = button->bg_default_color;

	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);

	if (button->cur_offset < button->pos_offset) {
		button->cur_offset = button->pos_offset;
	}
	if (rect->gx_rectangle_left + button->cur_offset >=
		rect->gx_rectangle_right - button->icon_width - button->pos_offset) {
		button->cur_offset =
			rect->gx_rectangle_right - button->icon_width - button->pos_offset - rect->gx_rectangle_left;
		bg_color = button->bg_active_color;
	}

	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);

	gx_context_brush_define(bg_color, bg_color, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
	uint16_t width = abs(rect->gx_rectangle_bottom - rect->gx_rectangle_top) + 1;
	gx_context_brush_width_set(width);
	uint16_t temp = (width >> 1) + 1;
	// draw bg color
	gx_canvas_line_draw(rect->gx_rectangle_left + temp, rect->gx_rectangle_top + temp, rect->gx_rectangle_right - temp,
						rect->gx_rectangle_top + temp);

	gx_prompt_draw(&button->desc);

	GX_PIXELMAP *map = NULL;
	gx_context_pixelmap_get(button->slide_icon, &map);
	INT xpos = 0;
	INT ypos = 0;
	if (map) {
		xpos = rect->gx_rectangle_left + button->cur_offset;
		ypos = rect->gx_rectangle_bottom - rect->gx_rectangle_top - map->gx_pixelmap_height + 1;
		ypos /= 2;
		ypos += rect->gx_rectangle_top;

		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
}
void custom_slide_button_create(Custom_SlideButton_TypeDef *button, USHORT widget_id, GX_WIDGET *parent,
								GX_RECTANGLE *size, GX_RESOURCE_ID color_bg_default, GX_RESOURCE_ID color_bg_active,
								GX_RESOURCE_ID icon_id, GX_STRING *desc, GX_RESOURCE_ID font_id,
								GX_RESOURCE_ID font_color, INT offset)
{
	button->bg_default_color = color_bg_default;
	button->bg_active_color = color_bg_active;
	button->slide_icon = icon_id;
	button->pos_offset = offset;
	GX_WIDGET *widget = (GX_WIDGET *)button;

	gx_widget_create(widget, NULL, parent, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_TRANSPARENT, widget_id,
					 size);

	gx_widget_event_process_set(widget, custom_slide_button_event_processing_function);

	gx_widget_draw_set(widget, custom_slide_button_draw);

	gx_prompt_create(&button->desc, NULL, &button->widget, 0,
					 GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 0, size);
	gx_prompt_text_color_set(&button->desc, font_color, font_color, font_color);
	gx_prompt_font_set(&button->desc, font_id);
	gx_prompt_text_set_ext(&button->desc, desc);

	return;
}