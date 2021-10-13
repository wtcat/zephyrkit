#include "custom_radiobox.h"
#include "sys/util.h"
void custom_radiobox_draw(GX_WIDGET *widget)
{
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);

	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);

	CUSTOM_RADIOBOX *radiobox = CONTAINER_OF(widget, CUSTOM_RADIOBOX, widget);
	UINT r = (fillrect.gx_rectangle_right - fillrect.gx_rectangle_left + 1) >> 1;
	UINT r_real = 0;

	gx_widget_background_draw(widget);

	if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		gx_context_brush_define(radiobox->color_id_select, radiobox->color_id_select,
								GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
		gx_context_brush_width_set(1);
		r_real = r - 1;

	} else {
		gx_context_brush_define(radiobox->color_id_deselect, radiobox->color_id_deselect, GX_BRUSH_ALIAS);
		gx_context_brush_width_set(3);
		r_real = r - 3;
	}

	gx_canvas_circle_draw(
		((fillrect.gx_rectangle_right - fillrect.gx_rectangle_left + 1) >> 1) + fillrect.gx_rectangle_left,
		((fillrect.gx_rectangle_bottom - fillrect.gx_rectangle_top + 1) >> 1) + fillrect.gx_rectangle_top, r_real);

	if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		gx_context_brush_define(radiobox->color_id_background, radiobox->color_id_background, GX_BRUSH_ALIAS);
		gx_context_brush_width_set(5);
		gx_canvas_circle_draw(
			((fillrect.gx_rectangle_right - fillrect.gx_rectangle_left + 1) >> 1) + fillrect.gx_rectangle_left,
			((fillrect.gx_rectangle_bottom - fillrect.gx_rectangle_top + 1) >> 1) + fillrect.gx_rectangle_top,
			r_real - 5);
	}
}

void custom_radiobox_create(CUSTOM_RADIOBOX *checkbox, GX_WIDGET *parent, GX_RECTANGLE *size)
{
	gx_widget_create(&checkbox->widget, NULL, parent, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT,
					 GX_ID_NONE, size);
	gx_widget_draw_set(&checkbox->widget, custom_radiobox_draw);
}

void custom_radiobox_status_change(CUSTOM_RADIOBOX *radiobox, GX_BOOL flag)
{
	if (flag) {
		radiobox->widget.gx_widget_style |= GX_STYLE_BUTTON_PUSHED;
	} else {
		radiobox->widget.gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
	}
	gx_system_dirty_mark(&radiobox->widget);
}
