#include "gx_api.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "home_window.h"
#include "zephyr.h"
#include "string.h"
#include "stdio.h"

static GX_WIDGET curr_widget;
static GX_PROMPT degree_info;
static GX_CHAR degree_info_buff[10];
static uint16_t angree;
static uint8_t enter_silent_mode = 0;
#define COMPASS_TIME_ID 1
static void compass_widget_draw(GX_WIDGET *widget)
{
	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);
	GX_PIXELMAP *map = NULL;
	gx_context_pixelmap_get(GX_PIXELMAP_ID_APP_COMPASS_MAIN, &map);
	if (map != NULL) {
		if (enter_silent_mode) {
			gx_canvas_pixelmap_rotate(widget->gx_widget_size.gx_rectangle_left + 50,
									  widget->gx_widget_size.gx_rectangle_top + 82, map, 0, -1, -1);
		} else {
			gx_canvas_pixelmap_rotate(widget->gx_widget_size.gx_rectangle_left + 50,
									  widget->gx_widget_size.gx_rectangle_top + 82, map, -(angree + 10), -1, -1);
		}
	}
	gx_context_pixelmap_get(GX_PIXELMAP_ID_APP_COMPASS_ARROW, &map);
	if (map != NULL) {
		gx_canvas_pixelmap_draw(widget->gx_widget_size.gx_rectangle_left + 151,
								widget->gx_widget_size.gx_rectangle_top + 70, map);
	}
	gx_widget_children_draw(widget);
}

static void degree_format(uint16_t degree, GX_CHAR *buff, uint8_t buff_cnt)
{
	if (enter_silent_mode) {
		snprintf(buff, buff_cnt, "NA");
	} else {
		if ((degree > 0) && (degree < 90)) {
			snprintf(buff, buff_cnt, "NE %d°", degree);
		} else if (degree == 0) {
			snprintf(buff, buff_cnt, "N %d°", degree);
		} else if (degree == 90) {
			snprintf(buff, buff_cnt, "E %d°", degree);
		} else if ((degree > 90) && (degree < 180)) {
			snprintf(buff, buff_cnt, "SE %d°", degree);
		} else if (degree == 180) {
			snprintf(buff, buff_cnt, "S %d°", degree);
		} else if ((degree > 180) && (degree < 270)) {
			snprintf(buff, buff_cnt, "SW %d°", degree);
		} else if (degree == 270) {
			snprintf(buff, buff_cnt, "W %d°", degree);
		} else if ((degree > 270) && (degree < 360)) {
			snprintf(buff, buff_cnt, "NW %d°", degree);
		} else if (degree == 360) {
			snprintf(buff, buff_cnt, "N 0°");
		}
	}
}

static UINT compass_widget_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		gx_system_timer_start(widget, COMPASS_TIME_ID, 1, 1);
		break;
	case GX_EVENT_HIDE:
		gx_system_timer_stop(widget, COMPASS_TIME_ID);
		angree = 0;
		break;
	case GX_EVENT_TIMER: {
		static uint8_t silent_mode_cnt = 0;
		UINT timer_id = event_ptr->gx_event_payload.gx_event_timer_id;
		if ((event_ptr->gx_event_target == widget) && (timer_id == COMPASS_TIME_ID)) {
			if ((widget->gx_widget_size.gx_rectangle_left == 0) && (!home_window_is_pen_down())) {
				if (!enter_silent_mode) {
					angree += 10;
					if (angree >= 60) {
						angree = 0;
					}
				} else {
					silent_mode_cnt++;
					if (silent_mode_cnt >= 20) {
						silent_mode_cnt = 0;
						enter_silent_mode = 0;
					}
				}
			} else {
				angree = 350;
				enter_silent_mode = 1;
				silent_mode_cnt = 0;
			}
			static uint16_t angree_tmp;
			if (angree_tmp != angree) {
				degree_format(angree, degree_info_buff, sizeof(degree_info_buff));
				GX_STRING string;
				string.gx_string_ptr = degree_info_buff;
				string.gx_string_length = strlen(string.gx_string_ptr);
				gx_prompt_text_set_ext(&degree_info, &string);
				gx_system_dirty_mark(widget);
			}
			return 0;
		}
		break;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

void widget_compass_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_widget, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	home_widget_type_reg(&curr_widget, HOME_WIDGET_TYPE_COMPASS);
	gx_widget_draw_set(&curr_widget, compass_widget_draw);
	gx_widget_event_process_set(&curr_widget, compass_widget_event);

	gx_utility_rectangle_define(&childSize, 12, 25, 319, 25 + 26 - 1);
	gx_prompt_create(&degree_info, NULL, &curr_widget, 0,
					 GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 0, &childSize);
	gx_prompt_text_color_set(&degree_info, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&degree_info, GX_FONT_ID_SIZE_26);

	GX_STRING init_string;
	init_string.gx_string_ptr = "NE 320°";
	init_string.gx_string_length = strlen(init_string.gx_string_ptr);
	gx_prompt_text_set_ext(&degree_info, &init_string);

	enter_silent_mode = 0;
}