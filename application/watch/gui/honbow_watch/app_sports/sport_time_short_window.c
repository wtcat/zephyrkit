#include "app_outdoor_run_window.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "app_list_define.h"
#include "drivers_ext/sensor_priv.h"
#include "custom_window_title.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include "string.h"
#include "stdio.h"

#include "custom_rounded_button.h"
#include "zephyr.h"
#include "guix_language_resources_custom.h"
#include "app_sports_window.h"
void gui_set_sport_param(Sport_cfg_t *cfg);
GX_WIDGET sport_time_short_window;
extern GX_WIDGET sport_start_window;
extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);
void app_sport_time_short_windown_init(GX_WINDOW *window);
UINT confirm_button_window_event(GX_WIDGET *widget, GX_EVENT *event_ptr);
#define WIDGET_ID_2    2
#define WIDGET_ID_3    3
extern uint32_t sport_time_count;
extern struct timespec end_sport_ts;

UINT sport_time_short_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {	

	case GX_SIGNAL(WIDGET_ID_2, GX_EVENT_CLICKED):
		jump_sport_page(&sport_data_window, &sport_time_short_window, SCREEN_SLIDE_RIGHT);
		gui_sport_cfg.status = SPORT_START;
//		gui_set_sport_param(&gui_sport_cfg);
		return 0;
		break;
	case GX_SIGNAL(WIDGET_ID_3, GX_EVENT_CLICKED):
		jump_sport_page(&sport_main_page, &sport_time_short_window, SCREEN_SLIDE_LEFT);
		gui_sport_cfg.status = SPORT_END;
		gui_sport_cfg.work_long = sport_time_count;
//		gui_set_sport_param(&gui_sport_cfg);
		sport_time_count = 0;
		clear_sport_widow();
		return 0;
		break;
	default :
		break;
	}
	return gx_widget_event_process(window, event_ptr);	
}

void app_sport_short_time_text_get(GX_STRING *prompt)
{	
	prompt->gx_string_ptr = "Workout duration is too short, do you end the exercise?";	
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

static CHAR short_buffer1[100];
static GX_WIDGET short_sport_widget0, short_sport_widget1, short_sport_widget2;
void sport_time_short_window1_draw(GX_WIDGET *widget)
{
	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);
	INT xpos;
	INT ypos;
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);
	GX_COLOR color ;
	gx_context_color_get(GX_COLOR_ID_HONBOW_DARK_BLACK, &color);

	if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		uint8_t red = ((color >> 11) >> 1);
		uint8_t green = (((color >> 5) & 0x3f) >> 1);
		uint8_t blue = ((color & 0x1f) >> 1);
		color = (red << 11) + (green << 5) + blue;
	}

	uint16_t width = abs(widget->gx_widget_size.gx_rectangle_bottom - 
		widget->gx_widget_size.gx_rectangle_top) + 1;

	
	gx_brush_define(&current_context->gx_draw_context_brush, 
		color, color, GX_BRUSH_ALIAS);
	gx_context_brush_width_set(width - 20);
	uint16_t half_line_width_2 = ((width - 20) >> 1) + 1;
	gx_canvas_line_draw(
		widget->gx_widget_size.gx_rectangle_left + 1,
		widget->gx_widget_size.gx_rectangle_top + 10 + half_line_width_2,
		widget->gx_widget_size.gx_rectangle_right - 1,
		widget->gx_widget_size.gx_rectangle_top + 10 + half_line_width_2);
	gx_brush_define(&current_context->gx_draw_context_brush, 
			color, color, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);

	gx_context_brush_width_set(20);
	uint16_t half_line_width = 11;
	gx_canvas_line_draw(
		widget->gx_widget_size.gx_rectangle_left + half_line_width,
		widget->gx_widget_size.gx_rectangle_top + half_line_width,
		widget->gx_widget_size.gx_rectangle_right - half_line_width,
		widget->gx_widget_size.gx_rectangle_top + half_line_width);
	gx_canvas_line_draw(
		widget->gx_widget_size.gx_rectangle_left + half_line_width,
		widget->gx_widget_size.gx_rectangle_bottom - half_line_width + 2,
		widget->gx_widget_size.gx_rectangle_right - half_line_width,
		widget->gx_widget_size.gx_rectangle_bottom - half_line_width + 2);

	GX_PIXELMAP *map = NULL;
	gx_context_pixelmap_get(GX_PIXELMAP_ID_APP_SPORT_BTN_S_CANCEL, &map);
	if (map) {		
		xpos = widget->gx_widget_size.gx_rectangle_left;
		ypos = widget->gx_widget_size.gx_rectangle_top;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

void sport_time_short_window2_draw(GX_WIDGET *widget)
{
	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);
	INT xpos;
	INT ypos;
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);
	GX_COLOR color ;
	gx_context_color_get(GX_COLOR_ID_HONBOW_CONFIRM_GREEN, &color);

	if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		uint8_t red = ((color >> 11) >> 1);
		uint8_t green = (((color >> 5) & 0x3f) >> 1);
		uint8_t blue = ((color & 0x1f) >> 1);
		color = (red << 11) + (green << 5) + blue;
	}

	uint16_t width = abs(
				widget->gx_widget_size.gx_rectangle_bottom - 
				widget->gx_widget_size.gx_rectangle_top) + 1;	
	gx_brush_define(&current_context->gx_draw_context_brush, 
								color, color, GX_BRUSH_ALIAS);
	gx_context_brush_width_set(width - 20);
	uint16_t half_line_width_2 = ((width - 20) >> 1) + 1;
	gx_canvas_line_draw(
		widget->gx_widget_size.gx_rectangle_left + 1,
		widget->gx_widget_size.gx_rectangle_top + 10 + half_line_width_2,
		widget->gx_widget_size.gx_rectangle_right - 1,
		widget->gx_widget_size.gx_rectangle_top + 10 + half_line_width_2);
	gx_brush_define(&current_context->gx_draw_context_brush, 
				color, color, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
	gx_context_brush_width_set(20);
	uint16_t half_line_width = 11;
	gx_canvas_line_draw(
		widget->gx_widget_size.gx_rectangle_left + half_line_width,
		widget->gx_widget_size.gx_rectangle_top + half_line_width,
		widget->gx_widget_size.gx_rectangle_right - half_line_width,
		widget->gx_widget_size.gx_rectangle_top + half_line_width);
	gx_canvas_line_draw(
		widget->gx_widget_size.gx_rectangle_left + half_line_width,
		widget->gx_widget_size.gx_rectangle_bottom - half_line_width + 2,
		widget->gx_widget_size.gx_rectangle_right - half_line_width,
		widget->gx_widget_size.gx_rectangle_bottom - half_line_width + 2);
	GX_PIXELMAP *map = NULL;
	gx_context_pixelmap_get(GX_PIXELMAP_ID_APP_SPORT_BTN_S_CONFIRM, &map);
	if (map) {		
		xpos = widget->gx_widget_size.gx_rectangle_left;
		ypos = widget->gx_widget_size.gx_rectangle_top;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

static GX_MULTI_LINE_TEXT_INPUT short_multi_text;
void app_sport_time_short_windown_init(GX_WINDOW *window)
{
	GX_RECTANGLE win_size;
	GX_STRING string;	
	UINT status;
	
	gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
	gx_widget_create(&sport_time_short_window, "sport_time_short", window,
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
		0, &win_size);
	gx_widget_fill_color_set(&sport_time_short_window, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK);
	GX_WIDGET *parent = &sport_time_short_window;
	
	gx_utility_rectangle_define(&win_size, 12, 120, 308, 220);
	gx_widget_create(&short_sport_widget0, "sport_time_short_widget0", parent, 
				GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
				0, &win_size);
	gx_widget_fill_color_set(&short_sport_widget0, GX_COLOR_ID_HONBOW_BLACK,
				GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);		
	status = gx_multi_line_text_input_create(&short_multi_text, "multi123", 
				&short_sport_widget0, short_buffer1, 100, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE|GX_STYLE_TEXT_CENTER,
				1, &win_size);
	app_sport_short_time_text_get(&string);
	if (status == GX_SUCCESS) {
		gx_multi_line_text_view_font_set((GX_MULTI_LINE_TEXT_VIEW *)&short_multi_text, 
				GX_FONT_ID_SIZE_30);//GX_FONT_ID_TEXT_INPUT
		gx_multi_line_text_input_fill_color_set(&short_multi_text, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		gx_multi_line_text_input_text_color_set(&short_multi_text, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
		gx_multi_line_text_view_whitespace_set((GX_MULTI_LINE_TEXT_VIEW *)&short_multi_text, 0);
		gx_multi_line_text_view_line_space_set((GX_MULTI_LINE_TEXT_VIEW *)&short_multi_text, 0);
		gx_multi_line_text_input_text_set_ext(&short_multi_text, &string);
	}

	gx_utility_rectangle_define(&win_size, 12, 283, 155, 348);
	gx_widget_create(&short_sport_widget1, "sport_short_time_widget1", parent, 
				GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
				WIDGET_ID_2, &win_size);
	gx_widget_fill_color_set(&short_sport_widget1, GX_COLOR_ID_HONBOW_BLACK,
				GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);	
	gx_widget_event_process_set(&short_sport_widget1, confirm_button_window_event);
	gx_widget_draw_set(&short_sport_widget1, sport_time_short_window1_draw);

	gx_utility_rectangle_define(&win_size, 165, 283, 308, 348);
	gx_widget_create(&short_sport_widget2, "sport_short_time_widget2", parent, 
				GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
				WIDGET_ID_3, &win_size);
	gx_widget_fill_color_set(&short_sport_widget2, GX_COLOR_ID_HONBOW_BLACK,
				GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);	
	gx_widget_event_process_set(&short_sport_widget2, confirm_button_window_event);
	gx_widget_draw_set(&short_sport_widget2, sport_time_short_window2_draw);

	gx_widget_event_process_set(&sport_time_short_window, 
				sport_time_short_window_event);
}



