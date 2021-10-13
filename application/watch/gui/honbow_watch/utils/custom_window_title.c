#include "custom_window_title.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "stdbool.h"
#include "sys/util.h"

#define TITLE_START_POS_X 12
#define TITLE_START_POS_Y 16

#define TIME_TIPS_POS_X 230
#define TIME_TIPS_POS_Y 16

void custom_title_icon_draw_func(GX_WIDGET *widget)
{
	INT xpos;
	INT ypos;

	Custom_title_icon_t *icon_tips = CONTAINER_OF(widget, Custom_title_icon_t, widget);

	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);

	gx_context_brush_define(icon_tips->bg_color, icon_tips->bg_color, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
	gx_context_brush_width_set(1);
	gx_canvas_circle_draw(
		((fillrect.gx_rectangle_right - fillrect.gx_rectangle_left + 1) >> 1) + fillrect.gx_rectangle_left,
		((fillrect.gx_rectangle_bottom - fillrect.gx_rectangle_top + 1) >> 1) + fillrect.gx_rectangle_top, 12);

	GX_PIXELMAP *map;
	gx_context_pixelmap_get(icon_tips->icon, &map);

	xpos = widget->gx_widget_size.gx_rectangle_right - widget->gx_widget_size.gx_rectangle_left -
		   map->gx_pixelmap_width + 1;

	xpos /= 2;
	xpos += widget->gx_widget_size.gx_rectangle_left;

	ypos = widget->gx_widget_size.gx_rectangle_bottom - widget->gx_widget_size.gx_rectangle_top -
		   map->gx_pixelmap_height + 1;
	ypos /= 2;
	ypos += widget->gx_widget_size.gx_rectangle_top;

	gx_canvas_pixelmap_draw(xpos, ypos, map);
}
void custom_window_title_create(GX_WIDGET *parent, Custom_title_icon_t *icon_tips, GX_PROMPT *info_tips,
								GX_PROMPT *time_tips)
{
	bool have_icon = false;
	if (icon_tips) {
#if 0
		gx_icon_create(icon_tips, NULL, parent, GX_NULL, GX_STYLE_TRANSPARENT, 0, TITLE_START_POS_X, TITLE_START_POS_Y);
		gx_widget_fill_color_set(icon_tips, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								 GX_COLOR_ID_HONBOW_BLACK);
#else
		GX_RECTANGLE size;
		gx_utility_rectangle_define(&size, TITLE_START_POS_X, TITLE_START_POS_Y, TITLE_START_POS_X + 26 - 1,
									TITLE_START_POS_Y + 26 - 1);
		gx_widget_create(&icon_tips->widget, NULL, parent, GX_STYLE_ENABLED, GX_ID_NONE, &size);
		gx_widget_draw_set(&icon_tips->widget, custom_title_icon_draw_func);
#endif
		have_icon = true;
	}

	GX_RECTANGLE childsize;

	if (info_tips) {
		if (true == have_icon) {
			gx_utility_rectangle_define(&childsize, TITLE_START_POS_X + 26 + 8, TITLE_START_POS_Y,
										TITLE_START_POS_X + 26 + 8 + 8 * 26 - 1, TITLE_START_POS_Y + 26 - 1);
		} else {
			gx_utility_rectangle_define(&childsize, TITLE_START_POS_X, TITLE_START_POS_Y,
										TITLE_START_POS_X + 8 * 26 - 1, TITLE_START_POS_Y + 26 - 1);
		}

		gx_prompt_create(info_tips, NULL, parent, 0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE,
						 0, &childsize);
		gx_prompt_text_color_set(info_tips, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
								 GX_COLOR_ID_HONBOW_WHITE);
		gx_prompt_font_set(info_tips, GX_FONT_ID_SIZE_26);
	}

	if (time_tips) {
		gx_utility_rectangle_define(&childsize, TIME_TIPS_POS_X, TIME_TIPS_POS_Y, TIME_TIPS_POS_X + 100,
									TIME_TIPS_POS_Y + 26 - 1);

		gx_prompt_create(time_tips, NULL, parent, 0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE,
						 0, &childsize);
		gx_prompt_text_color_set(time_tips, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
								 GX_COLOR_ID_HONBOW_WHITE);
		gx_prompt_font_set(time_tips, GX_FONT_ID_SIZE_26);
	}
}