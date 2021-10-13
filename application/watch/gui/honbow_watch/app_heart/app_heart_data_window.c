#include "app_today_window.h"
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
#include "guix_language_resources_custom.h"
#include "common_draw_until.h"
#include "custom_animation.h"
#include "app_heart_window.h"

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

GX_WIDGET please_wear_window;
VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
void heart_child_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr);



static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
INT please_wear_window_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {	
	case GX_SIGNAL(2, GX_EVENT_CLICKED):
		jump_heart_page(&heart_main_page, &please_wear_window, SCREEN_SLIDE_RIGHT);
	break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}	
	case GX_EVENT_PEN_UP: {
	 	delta_x = (event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
	 	if (delta_x >= 50) {
			jump_heart_page(&heart_main_page, &please_wear_window, SCREEN_SLIDE_RIGHT);
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);			
	 	}
		break;
	}
	case GX_EVENT_KEY_DOWN:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			return 0;
		}
		break;
	case GX_EVENT_KEY_UP:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			jump_heart_page(&heart_main_page, &please_wear_window, SCREEN_SLIDE_RIGHT);
			windows_mgr_page_jump((GX_WINDOW *)&please_wear_window, NULL, WINDOWS_OP_BACK);	
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);			
			return 0;
		}
		break;
	default:
		gx_widget_event_process(widget, event_ptr);
		break;
	}
	return 0;
}

void please_wear_text_get(GX_STRING *prompt)
{	
	prompt->gx_string_ptr = "Please Wear it Correctly";	
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

void please_wear_rectangle_button_draw(GX_WIDGET *widget)
{
	MaxMinHr_Widget_T *row = CONTAINER_OF(widget, MaxMinHr_Widget_T, hr_widget);
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
	gx_context_color_get(GX_COLOR_ID_HONBOW_RED, &color);
	
	if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		uint8_t red = ((color >> 11) >> 2);
		uint8_t green = (((color >> 5) & 0x3f) >> 2);
		uint8_t blue = ((color & 0x1f) >> 2);
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
	gx_context_pixelmap_get(row->icon, &map);
	if (map) {		
		xpos = widget->gx_widget_size.gx_rectangle_left;
		ypos = widget->gx_widget_size.gx_rectangle_top;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

MaxMinHr_Widget_T icon_widget, retry_widget;
GX_WIDGET multi_line_widget;
static GX_MULTI_LINE_TEXT_INPUT text_input;
static CHAR buffer[100];

static VOID please_wear_widget_create(GX_WIDGET *widget)
{
	GX_RECTANGLE childsize;
	GX_STRING string;	

	gx_utility_rectangle_define(&childsize, 62, 71, 308, 174);
	gx_widget_create(&icon_widget.hr_widget, "icon_widget", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_fill_color_set(&icon_widget.hr_widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&icon_widget.hr_widget, heart_child_widget_common_event);
	icon_widget.icon = GX_PIXELMAP_ID_APP_HEART_IMG_HAND;
	gx_widget_draw_set(&icon_widget.hr_widget, heart_icon_widget_draw);

	gx_utility_rectangle_define(&childsize, 9, 174, 308, 250);
	gx_widget_create(&multi_line_widget, NULL, widget,
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_fill_color_set(&multi_line_widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	please_wear_text_get(&string);		
	UINT status;	
	status = gx_multi_line_text_input_create(&text_input, "multi_text", 
				&multi_line_widget, buffer, 100, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE|GX_STYLE_TEXT_CENTER,
				GX_ID_NONE, &childsize);
	if (status == GX_SUCCESS) {
		gx_multi_line_text_view_font_set(
				(GX_MULTI_LINE_TEXT_VIEW *)&text_input, GX_FONT_ID_SIZE_30);
		gx_multi_line_text_input_fill_color_set(
			&text_input, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		gx_multi_line_text_input_text_color_set(
			&text_input, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
		gx_multi_line_text_view_whitespace_set(
			(GX_MULTI_LINE_TEXT_VIEW *)&text_input, 0);
		gx_multi_line_text_view_line_space_set(
			(GX_MULTI_LINE_TEXT_VIEW *)&text_input, 0);
		gx_multi_line_text_input_text_set_ext(
			&text_input, &string);
	}

	gx_utility_rectangle_define(&childsize, 12, 282, 308, 347);
	gx_widget_create(&retry_widget.hr_widget, "retry_widget", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 2, &childsize);
	gx_widget_fill_color_set(&retry_widget.hr_widget, GX_COLOR_ID_HONBOW_GREEN,
		GX_COLOR_ID_HONBOW_GREEN,GX_COLOR_ID_HONBOW_GREEN);
	gx_widget_event_process_set(&retry_widget.hr_widget, heart_child_widget_common_event);
	gx_widget_draw_set(&retry_widget.hr_widget, please_wear_rectangle_button_draw);
	retry_widget.icon = GX_PIXELMAP_ID_APP_HEART_BTN_L_REFRESH;	
	
}

void app_please_wear_window_init(GX_WINDOW *window)
{
	GX_RECTANGLE childsize;
	gx_utility_rectangle_define(&childsize, 0, 0, 319, 359);
	gx_widget_create(&please_wear_window, "please_wear_window", window, 
					GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
					&childsize);
	gx_widget_event_process_set(&please_wear_window, please_wear_window_event);
	GX_WIDGET *parent = &please_wear_window;
	please_wear_widget_create(parent);
}

