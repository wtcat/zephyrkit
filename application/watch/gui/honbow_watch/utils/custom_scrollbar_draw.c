#include "gx_api.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"

#define SCROLLBAR_HEIGHT 100
VOID custom_scrollbar_vertical_draw(GX_SCROLLBAR *scrollbar)
{
	GX_RECTANGLE main_size;
	GX_RECTANGLE thumb_size;
	GX_COLOR fill_color;

	if (scrollbar->gx_widget_style & GX_STYLE_ENABLED) {
		if (scrollbar->gx_widget_style & GX_STYLE_DRAW_SELECTED) {
			fill_color = scrollbar->gx_widget_selected_fill_color;
		} else {
			fill_color = scrollbar->gx_widget_normal_fill_color;
		}
	} else {
		fill_color = scrollbar->gx_widget_disabled_fill_color;
	}

	gx_widget_client_get(scrollbar, 0, &main_size);

	gx_context_brush_define(GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&main_size);

#if 1
	gx_context_brush_define(scrollbar->gx_widget_normal_fill_color, scrollbar->gx_widget_normal_fill_color,
							GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
#else
	gx_context_brush_define(GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
							GX_BRUSH_ALIAS | GX_BRUSH_ROUND);

#endif
	uint16_t width = abs(scrollbar->gx_widget_size.gx_rectangle_right - scrollbar->gx_widget_size.gx_rectangle_left);
	gx_context_brush_width_set(width);
	uint16_t temp = (width >> 1) + 1;
	// draw bg color
	gx_canvas_line_draw(scrollbar->gx_widget_size.gx_rectangle_left + temp,
						scrollbar->gx_widget_size.gx_rectangle_top + temp,
						scrollbar->gx_widget_size.gx_rectangle_left + temp,
						scrollbar->gx_widget_size.gx_rectangle_top + SCROLLBAR_HEIGHT - temp);

	thumb_size = scrollbar->gx_scrollbar_thumb.gx_widget_size;
	uint16_t height_old = thumb_size.gx_rectangle_bottom - thumb_size.gx_rectangle_top + 1;
	uint16_t height_new =
		SCROLLBAR_HEIGHT * height_old / (main_size.gx_rectangle_bottom - main_size.gx_rectangle_top + 1);

	GX_VALUE top = main_size.gx_rectangle_top +
				   (thumb_size.gx_rectangle_top - main_size.gx_rectangle_top) * height_new / height_old;
	GX_VALUE bottom = top + height_new;

	GX_VALUE left = thumb_size.gx_rectangle_left;
	GX_VALUE right = thumb_size.gx_rectangle_right;

	gx_context_brush_define(scrollbar->gx_scrollbar_thumb.gx_scroll_thumb_border_color,
							scrollbar->gx_scrollbar_thumb.gx_widget_normal_fill_color, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);

	width = abs(right - left);
	gx_context_brush_width_set(width);
	temp = (width >> 1) + 1;
	// draw bg color
	gx_canvas_line_draw(thumb_size.gx_rectangle_left + temp, top + temp, thumb_size.gx_rectangle_left + temp,
						bottom - temp);
}
