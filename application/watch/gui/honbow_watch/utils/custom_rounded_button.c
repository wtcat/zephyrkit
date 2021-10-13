#include "custom_rounded_button.h"
#include "zephyr.h"
#include "guix_simple_resources.h"
#include "gx_system.h"

extern GX_WINDOW_ROOT *root;
extern UINT _gx_utility_alphamap_create(INT width, INT height, GX_PIXELMAP *map);
void custom_rounded_button_draw(GX_WIDGET *widget)
{
	INT xpos;
	INT ypos;
	GX_RECTANGLE fillrect;
	GX_DRAW_CONTEXT *current_context;
	Custom_RoundedButton_TypeDef *element = CONTAINER_OF(widget, Custom_RoundedButton_TypeDef, widget);
	GX_COLOR color = 0;

	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);

	gx_system_draw_context_get(&current_context);
	gx_context_color_get(element->bg_color, &color);
	if (!element->disable_flag) {
		if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
			uint8_t red = ((color >> 11) >> 1) + ((color >> 11) >> 2);
			uint8_t green = (((color >> 5) & 0x3f) >> 1) + (((color >> 5) & 0x3f) >> 2);
			uint8_t blue = ((color & 0x1f) >> 1) + ((color & 0x1f) >> 2);
			color = (red << 11) + (green << 5) + blue;
		}
	}

	uint16_t width = abs(widget->gx_widget_size.gx_rectangle_bottom - widget->gx_widget_size.gx_rectangle_top) + 1;
	gx_brush_define(&current_context->gx_draw_context_brush, color, 0, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
	gx_context_brush_width_set(20);
	uint16_t half_line_width = 10;
	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + half_line_width,
						widget->gx_widget_size.gx_rectangle_top + half_line_width + 1,
						widget->gx_widget_size.gx_rectangle_right - half_line_width,
						widget->gx_widget_size.gx_rectangle_top + half_line_width + 1);

	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + half_line_width,
						widget->gx_widget_size.gx_rectangle_bottom - half_line_width,
						widget->gx_widget_size.gx_rectangle_right - half_line_width,
						widget->gx_widget_size.gx_rectangle_bottom - half_line_width);
	gx_brush_define(&current_context->gx_draw_context_brush, color, 0, 0);
	gx_context_brush_width_set(width - 20);
	uint16_t half_line_width_2 = ((width - 20) >> 1) + 1;
	gx_canvas_line_draw(
		widget->gx_widget_size.gx_rectangle_left, widget->gx_widget_size.gx_rectangle_top + 10 + half_line_width_2,
		widget->gx_widget_size.gx_rectangle_right, widget->gx_widget_size.gx_rectangle_top + 10 + half_line_width_2);
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
	gx_widget_children_draw(widget);

	if ((element->disable_flag) && (element->overlay_map != NULL)) {
		if (element->overlay_map->gx_pixelmap_data != NULL)
			gx_canvas_pixelmap_tile(&fillrect, element->overlay_map);
	}
}

void custom_rounded_button_icon_draw(GX_WIDGET *widget)
{
	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);

	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);

	INT xpos;
	INT ypos;
	Custom_RoundedIconButton_TypeDef *element = CONTAINER_OF(widget, Custom_RoundedIconButton_TypeDef, widget);

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

	uint16_t width = abs(widget->gx_widget_size.gx_rectangle_bottom - widget->gx_widget_size.gx_rectangle_top) + 1;

	gx_brush_define(&current_context->gx_draw_context_brush, color, 0, GX_BRUSH_ALIAS);
	gx_context_brush_width_set(width - 20);
	uint16_t half_line_width_2 = ((width - 20) >> 1) + 1;
	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + 1,
						widget->gx_widget_size.gx_rectangle_top + 10 + half_line_width_2,
						widget->gx_widget_size.gx_rectangle_right - 1,
						widget->gx_widget_size.gx_rectangle_top + 10 + half_line_width_2);

	gx_brush_define(&current_context->gx_draw_context_brush, color, 0, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);

#if 0
	gx_context_brush_width_set(width);
	uint16_t temp = (width >> 1) + 1;
	// draw bg color
	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + temp, widget->gx_widget_size.gx_rectangle_top + temp,
						widget->gx_widget_size.gx_rectangle_right - temp,
						widget->gx_widget_size.gx_rectangle_top + temp);
#else
	gx_context_brush_width_set(20);
	uint16_t half_line_width = 11;
	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + half_line_width,
						widget->gx_widget_size.gx_rectangle_top + half_line_width,
						widget->gx_widget_size.gx_rectangle_right - half_line_width,
						widget->gx_widget_size.gx_rectangle_top + half_line_width);

	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + half_line_width,
						widget->gx_widget_size.gx_rectangle_bottom - half_line_width + 2,
						widget->gx_widget_size.gx_rectangle_right - half_line_width,
						widget->gx_widget_size.gx_rectangle_bottom - half_line_width + 2);

#endif
	GX_PIXELMAP *map = NULL;
	gx_context_pixelmap_get(element->icon0, &map);
	if (map) {
		xpos = widget->gx_widget_size.gx_rectangle_right - widget->gx_widget_size.gx_rectangle_left -
			   map->gx_pixelmap_width + 1;
		xpos = widget->gx_widget_size.gx_rectangle_left + 10;
		ypos = widget->gx_widget_size.gx_rectangle_top + 10;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);
UINT custom_rounded_button_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	Custom_RoundedButton_TypeDef *element = CONTAINER_OF(widget, Custom_RoundedButton_TypeDef, widget);

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		if ((_gx_system_memory_allocator != NULL) && (element->disable_alpha_value) && (element->overlay_map == NULL)) {
			element->overlay_map = _gx_system_memory_allocator(sizeof(GX_PIXELMAP));
			int width = ROUND_BUTTON_OVERLAY_PIXELMAP_WIDTH;
			int height = ROUND_BUTTON_OVERLAY_PIXELMAP_HEIGHT;
			_gx_utility_alphamap_create(width, height, element->overlay_map);
			GX_UBYTE *data = (GX_UBYTE *)element->overlay_map->gx_pixelmap_data;
			memset(data, element->disable_alpha_value, width * height);
		}
		gx_widget_event_process(widget, event_ptr);
		break;
	case GX_EVENT_HIDE:
		if (element->overlay_map != NULL) {
			_gx_system_memory_free((GX_UBYTE *)element->overlay_map->gx_pixelmap_data);
			element->overlay_map->gx_pixelmap_data = NULL;
			_gx_system_memory_free(element->overlay_map);
			element->overlay_map = NULL;
		}
		gx_widget_event_process(widget, event_ptr);
		break;
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
void custom_rounded_button_create(Custom_RoundedButton_TypeDef *element, USHORT widget_id, GX_WIDGET *parent,
								  GX_RECTANGLE *size, GX_RESOURCE_ID color_id, GX_RESOURCE_ID icon_id, GX_STRING *desc,
								  Custom_RounderButton_DispType disp_type)
{
	gx_widget_create(&element->widget, GX_NULL, parent, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, widget_id, size);

	if (disp_type != CUSTOM_ROUNDED_BUTTON_ICON_ONLY) {
		if (desc != NULL) {
			GX_RECTANGLE child_size = *size;
			ULONG style = 0;
			if (disp_type == CUSTOM_ROUNDED_BUTTON_TEXT_LEFT) {
				child_size.gx_rectangle_left += 20;
				style = GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE;
			} else {
				style = GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE;
			}
			gx_prompt_create(&element->desc, NULL, &element->widget, 0, style, 0, &child_size);
			gx_prompt_text_color_set(&element->desc, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
									 GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&element->desc, GX_FONT_ID_SIZE_26);
			gx_prompt_text_set_ext(&element->desc, desc);
		}
	}
	gx_widget_draw_set(&element->widget, custom_rounded_button_draw);
	gx_widget_event_process_set(&element->widget, custom_rounded_button_event);
	element->bg_color = color_id;
	element->icon = icon_id;
	element->disp_type = disp_type;
	element->overlay_map = NULL;
}

void custom_rounded_button_create2(Custom_RoundedButton_TypeDef *element, USHORT widget_id, GX_WIDGET *parent,
								   GX_RECTANGLE *size, GX_RESOURCE_ID color_id, GX_RESOURCE_ID icon_id, GX_STRING *desc,
								   GX_RESOURCE_ID font_id, Custom_RounderButton_DispType disp_type)
{
	gx_widget_create(&element->widget, GX_NULL, parent, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, widget_id, size);

	if (disp_type != CUSTOM_ROUNDED_BUTTON_ICON_ONLY) {
		if (desc != NULL) {
			GX_RECTANGLE child_size = *size;
			ULONG style = 0;
			if (disp_type == CUSTOM_ROUNDED_BUTTON_TEXT_LEFT) {
				child_size.gx_rectangle_left += 20;
				style = GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE;
			} else {
				style = GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE;
			}
			gx_prompt_create(&element->desc, NULL, &element->widget, 0, style, 0, &child_size);
			gx_prompt_text_color_set(&element->desc, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
									 GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&element->desc, font_id);
			gx_prompt_text_set_ext(&element->desc, desc);
		}
	}
	gx_widget_draw_set(&element->widget, custom_rounded_button_draw);
	gx_widget_event_process_set(&element->widget, custom_rounded_button_event);
	element->bg_color = color_id;
	element->icon = icon_id;
	element->disp_type = disp_type;
	element->overlay_map = NULL;
}

void custom_rounded_button_create_3(Custom_RoundedButton_TypeDef *element, USHORT widget_id, GX_WIDGET *parent,
									GX_RECTANGLE *size, GX_RESOURCE_ID color_id, GX_RESOURCE_ID icon_id,
									GX_STRING *desc, GX_RESOURCE_ID font_id, Custom_RounderButton_DispType disp_type,
									GX_UBYTE text_only_mode_offset_value, GX_UBYTE disabled_alpha_value)
{
	gx_widget_create(&element->widget, GX_NULL, parent, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, widget_id, size);

	if (disp_type != CUSTOM_ROUNDED_BUTTON_ICON_ONLY) {
		if (desc != NULL) {
			GX_RECTANGLE child_size = *size;
			ULONG style = 0;
			if (disp_type == CUSTOM_ROUNDED_BUTTON_TEXT_LEFT) {
				child_size.gx_rectangle_left += text_only_mode_offset_value;
				style = GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE;
			} else {
				style = GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE;
			}
			gx_prompt_create(&element->desc, NULL, &element->widget, 0, style, 0, &child_size);
			gx_prompt_text_color_set(&element->desc, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
									 GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&element->desc, font_id);
			gx_prompt_text_set_ext(&element->desc, desc);
		}
	}
	gx_widget_draw_set(&element->widget, custom_rounded_button_draw);
	gx_widget_event_process_set(&element->widget, custom_rounded_button_event);
	element->bg_color = color_id;
	element->icon = icon_id;
	element->disp_type = disp_type;
	element->disable_alpha_value = disabled_alpha_value;
	element->disable_flag = 0;
	element->overlay_map = NULL;
}

void custom_rounded_button_align_create(Custom_RoundedIconButton_TypeDef *element, USHORT widget_id, GX_WIDGET *parent,
										GX_RECTANGLE *size, GX_RESOURCE_ID color_id, GX_RESOURCE_ID icon_id0,
										GX_RESOURCE_ID icon_id1, GX_STRING *desc, INT x_offset, INT y_offset)
{
	gx_widget_create(&element->widget, GX_NULL, parent, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, widget_id, size);

	if (desc != NULL) {
		GX_RECTANGLE child_size = *size;
		ULONG style = 0;
		if ((x_offset < child_size.gx_rectangle_right) && (y_offset < child_size.gx_rectangle_bottom)) {
			child_size.gx_rectangle_left += 10;
			child_size.gx_rectangle_top += 78;
		}
		style = GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE;

		gx_prompt_create(&element->desc, NULL, &element->widget, 0, style, 0, &child_size);
		gx_prompt_text_color_set(&element->desc, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
								 GX_COLOR_ID_HONBOW_WHITE);
		gx_prompt_font_set(&element->desc, GX_FONT_ID_SIZE_26);
		gx_prompt_text_set_ext(&element->desc, desc);
	}

	gx_widget_draw_set(&element->widget, custom_rounded_button_icon_draw);
	gx_widget_event_process_set(&element->widget, custom_rounded_button_event);
	element->bg_color = color_id;
	element->icon0 = icon_id0;
	element->icon1 = icon_id1;
}