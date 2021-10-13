#include "custom_icon_utils.h"
#include "zephyr.h"
#include "guix_simple_resources.h"

extern GX_WINDOW_ROOT *root;
void custom_icon_draw(GX_WIDGET *widget)
{
	gx_canvas_drawing_initiate(root->gx_window_root_canvas, widget, &widget->gx_widget_size);
	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);

	gx_context_brush_define(GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);

	INT xpos;
	INT ypos;
	Custom_Icon_TypeDef *element = CONTAINER_OF(widget, Custom_Icon_TypeDef, widget);

	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);

	GX_COLOR color = 0;

	gx_context_color_get(element->bg_color, &color);

	if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		uint8_t red = ((color >> 11) >> 1);
		uint8_t green = (((color >> 5) & 0x3f) >> 1);
		uint8_t blue = ((color & 0x1f) >> 1);
		color = (red << 11) + (green << 5) + blue;
	}
	gx_brush_define(&current_context->gx_draw_context_brush, color, 0, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);

	uint16_t width = abs(widget->gx_widget_size.gx_rectangle_bottom - widget->gx_widget_size.gx_rectangle_top) + 1;
	gx_context_brush_width_set(width);

	uint16_t temp = (width >> 1);
	// draw bg color
	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + temp, widget->gx_widget_size.gx_rectangle_top + temp,
						widget->gx_widget_size.gx_rectangle_right - temp,
						widget->gx_widget_size.gx_rectangle_top + temp);

	// draw icon
	GX_PIXELMAP *map = NULL;
	gx_context_pixelmap_get(element->icon, &map);
	if (map) {
		xpos = widget->gx_widget_size.gx_rectangle_right - widget->gx_widget_size.gx_rectangle_left -
			   map->gx_pixelmap_width + 1;

		xpos /= 2;
		xpos += widget->gx_widget_size.gx_rectangle_left;

		ypos = widget->gx_widget_size.gx_rectangle_bottom - widget->gx_widget_size.gx_rectangle_top -
			   map->gx_pixelmap_height + 1;

		ypos /= 2;
		ypos += widget->gx_widget_size.gx_rectangle_top;

		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_canvas_drawing_complete(root->gx_window_root_canvas, GX_FALSE);
}

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);
UINT control_center_icon_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN:
		if (widget->gx_widget_style & GX_STYLE_ENABLED) {
			_gx_system_input_capture(widget);
			widget->gx_widget_style |= GX_STYLE_BUTTON_PUSHED;
			gx_system_dirty_mark(widget);
		}
		status = gx_widget_event_to_parent(widget, event_ptr);
		break;
	case GX_EVENT_PEN_UP:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release(widget);
			widget->gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
			gx_system_dirty_mark(widget);
			if (gx_utility_rectangle_point_detect(&widget->gx_widget_size,
												  event_ptr->gx_event_payload.gx_event_pointdata)) {
				gx_widget_event_generate(widget, GX_EVENT_CLICKED, 0);
			}
		}
		// need check clicked event happended or not
		status = gx_widget_event_to_parent(widget, event_ptr);
		break;
	case GX_EVENT_INPUT_RELEASE:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release(widget);
			widget->gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
			gx_system_dirty_mark(widget);
		}
		break;
	default:
		gx_widget_event_process(widget, event_ptr);
	}
	return status;
}
void custom_icon_create(Custom_Icon_TypeDef *element, USHORT widget_id, GX_WIDGET *parent, GX_RECTANGLE *size,
						GX_RESOURCE_ID color_id, GX_RESOURCE_ID icon_id)
{
	gx_widget_create(&element->widget, GX_NULL, parent, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, widget_id, size);
	gx_widget_draw_set(&element->widget, custom_icon_draw);
	gx_widget_event_process_set(&element->widget, control_center_icon_event);
	element->bg_color = color_id;
	element->icon = icon_id;
}

void custom_icon_change_bg_color(Custom_Icon_TypeDef *element, GX_RESOURCE_ID curr_color)
{
	element->bg_color = curr_color;
	gx_system_dirty_mark(&element->widget);
}

void custom_icon_change_pixelmap(Custom_Icon_TypeDef *element, GX_RESOURCE_ID map_id)
{
	element->icon = map_id;
	gx_system_dirty_mark(&element->widget);
}