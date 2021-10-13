/* This is a small demo of the high-performance GUIX graphics framework. */

#include "gx_api.h"

/*************************************************************************************/
/* Define a custom pixemap button draw function. */
/* This function draw a pixelmap button with an alphamap, */
/* If the button is pushed, draw the alphamap with selected fill color,
 * otherwise,   */
/*  draw it with normal fill color. */
/*************************************************************************************/
VOID custom_circle_pixelmap_button_draw(GX_PIXELMAP_BUTTON *button)
{
	GX_PIXELMAP *map;
	GX_RESOURCE_ID fill_color;
	INT xpos;
	INT ypos;
	GX_COLOR color;
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);

	gx_context_pixelmap_get(button->gx_pixelmap_button_normal_id, &map);
	gx_context_color_get(button->gx_widget_normal_fill_color, &color);
	if (button->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		uint8_t red = ((color >> 11) >> 1);
		uint8_t green = (((color >> 5) & 0x3f) >> 1);
		uint8_t blue = ((color & 0x1f) >> 1);
		color = (red << 11) + (green << 5) + blue;
	}

	gx_brush_define(&current_context->gx_draw_context_brush, color, color, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);

	gx_context_brush_width_set(1);
	INT r = ((button->gx_widget_size.gx_rectangle_right - button->gx_widget_size.gx_rectangle_left + 1) >> 1) - 1;
	INT x = button->gx_widget_size.gx_rectangle_left + r;
	INT y = button->gx_widget_size.gx_rectangle_top + r;
	gx_canvas_circle_draw(x, y, r);

	if (map) {
		switch (button->gx_widget_style & GX_PIXELMAP_HALIGN_MASK) {
		case GX_STYLE_HALIGN_CENTER:
			xpos = button->gx_widget_size.gx_rectangle_right - button->gx_widget_size.gx_rectangle_left -
				   map->gx_pixelmap_width + 1;

			xpos /= 2;
			xpos += button->gx_widget_size.gx_rectangle_left;
			break;

		case GX_STYLE_HALIGN_RIGHT:
			xpos = button->gx_widget_size.gx_rectangle_right - map->gx_pixelmap_width + 1;
			break;

		default:
			xpos = button->gx_widget_size.gx_rectangle_left;
			break;
		}

		switch (button->gx_widget_style & GX_PIXELMAP_VALIGN_MASK) {
		case GX_STYLE_VALIGN_CENTER:
			ypos = button->gx_widget_size.gx_rectangle_bottom - button->gx_widget_size.gx_rectangle_top -
				   map->gx_pixelmap_height + 1;

			ypos /= 2;
			ypos += button->gx_widget_size.gx_rectangle_top;
			break;

		case GX_STYLE_VALIGN_BOTTOM:
			ypos = button->gx_widget_size.gx_rectangle_bottom - map->gx_pixelmap_height + 1;
			break;

		default:
			ypos = button->gx_widget_size.gx_rectangle_top;
			break;
		}

		gx_context_fill_color_set(fill_color);
		gx_canvas_pixelmap_draw(xpos, ypos, map);
	}

	gx_widget_children_draw((GX_WIDGET *)button);
}