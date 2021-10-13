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


GX_WIDGET gps_connecting_window;
GX_WIDGET gps_not_connectd_window;
GX_WIDGET gps_confirm_window;
GX_WIDGET confirm_child;
GX_WIDGET gps_child;
extern GX_WIDGET sport_start_window;
extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

static GX_PROMPT gps_title_prompt;
static GX_PROMPT time_prompt;
extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];
typedef struct GPS_DISCONNET_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	Custom_RoundedIconButton_TypeDef child;
} GPS_DISCONNET_LIST_ROW;

static GPS_DISCONNET_LIST_ROW gps_connecting_list_row_memory, confirm_window_row;
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 1
static GX_BOOL first_connect_gps = true;

void clear_gps_connect_status(void)
{
	first_connect_gps = true;
}

bool get_gps_connet_status(void)
{
	return true;	
}

void set_gps_connetting_titlle_prompt(INT index)
{
	GX_STRING string;
	app_sport_list_prompt_get(index, &string);
	gx_prompt_text_set_ext(&gps_title_prompt, &string);
}

UINT app_gps_connecting_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	static UINT count_timer = 0;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: 
		gx_system_timer_start(window, TIME_FRESH_TIMER_ID, 50, 50);
		count_timer = 0;
	break;
	case GX_EVENT_HIDE: 
		gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);
	
	break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}	
	case GX_EVENT_PEN_UP: {
	 	delta_x = GX_ABS(
			event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - 
				gx_point_x_first);
	 	if (delta_x >= 50) {
			jump_sport_page(&sport_main_page, 
				&gps_connecting_window, SCREEN_SLIDE_RIGHT);
	 	}
		break;
	}
	case GX_EVENT_TIMER: 
		if (event_ptr->gx_event_payload.gx_event_timer_id == TIME_FRESH_TIMER_ID) {
			struct timespec ts;
			GX_STRING string;
			clock_gettime(CLOCK_REALTIME, &ts);
			time_t now = ts.tv_sec;
			struct tm *tm_now = gmtime(&now);
			snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
			time_prompt_buff[7] = 0;
			string.gx_string_ptr = time_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&time_prompt, &string);
			count_timer++;		
			bool status = get_gps_connet_status();
			if(status) {
				gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);
				jump_sport_page(&sport_start_window, window, SCREEN_SLIDE_LEFT);				
			}else {
				if(count_timer == 10){
					if (first_connect_gps) {
						gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);
						jump_sport_page(&gps_confirm_window, &gps_connecting_window, SCREEN_SLIDE_LEFT);						
						first_connect_gps = false;
					}else {
						gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);
						jump_sport_page(&gps_not_connectd_window, &gps_connecting_window, SCREEN_SLIDE_LEFT);						
					}			
				}
			}			
			return 0;
		}		
	default :
		break;
	}
	return gx_widget_event_process(window, event_ptr);	
}

#define TIME_CONFIRM_WINDOW_ID 2
UINT app_gps_confirm_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}	
	case GX_EVENT_PEN_UP: {
	 	delta_x = GX_ABS(
			 event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - 
			 gx_point_x_first);
	 	if (delta_x >= 50) {
			;	
	 	}
		break;
	}	
	default :
		break;
	}
	return gx_widget_event_process(window, event_ptr);	
}

UINT gps_connecting_window_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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
		break;
	}
	return status;
}

void app_gps_connecting_list_prompt_get(GX_STRING *prompt)
{	
	prompt->gx_string_ptr = "GPS connecting";
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

void gps_disconnet_window_draw(GX_WIDGET *widget)
{
	GX_RECTANGLE fillrect;
	//GX_STRING string;
	gx_widget_client_get(widget, 0, &fillrect);

	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);
	INT xpos;
	INT ypos;
	Custom_RoundedIconButton_TypeDef *element = CONTAINER_OF(widget, 
		Custom_RoundedIconButton_TypeDef, widget);
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_DARK_BLACK, 
		GX_COLOR_ID_HONBOW_DARK_BLACK, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
	gx_context_brush_width_set(1);
	gx_canvas_circle_draw(
		((fillrect.gx_rectangle_right - fillrect.gx_rectangle_left + 1) >> 1) + 
		fillrect.gx_rectangle_left+1,
		((fillrect.gx_rectangle_bottom - fillrect.gx_rectangle_top + 1) >> 1) + 
		(fillrect.gx_rectangle_top >> 1)+1, 75);
	GX_PIXELMAP *map = NULL;
	gx_context_pixelmap_get(element->icon0, &map);

	if (map) {
		xpos = widget->gx_widget_size.gx_rectangle_right - 
			widget->gx_widget_size.gx_rectangle_left -
			map->gx_pixelmap_width + 1;
		xpos /= 2;
		xpos += widget->gx_widget_size.gx_rectangle_left;

		ypos = widget->gx_widget_size.gx_rectangle_bottom - 
			widget->gx_widget_size.gx_rectangle_top -
			map->gx_pixelmap_height + 1;		
		ypos += widget->gx_widget_size.gx_rectangle_top;
		ypos /= 2;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_context_pixelmap_get(element->icon1, &map);

	if (map) {	
		xpos = 24;		
		ypos = 289;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

GX_WIDGET prompt_widget;
static VOID gps_disconnet_run_list_row_create(GX_WIDGET *widget)
{
	GX_RECTANGLE childsize;
	GPS_DISCONNET_LIST_ROW *row = &gps_connecting_list_row_memory;
	GX_STRING string;	
	
	gx_utility_rectangle_define(&childsize, 0, 58, 318, 359);
	gx_widget_create(&row->widget, NULL, widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&row->widget, gps_connecting_window_event);	
	
	gx_utility_rectangle_define(&childsize, 0, 58, 318, 359);
	gx_widget_create(&row->child.widget, NULL, &row->widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_fill_color_set(&row->child.widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
	row->child.icon0 = GX_PIXELMAP_ID_APP_SPORT_BTN_START_NO;
	row->child.icon1 = GX_PIXELMAP_ID_APP_SPORT_IC_GPS_DOWN;			
	GX_RECTANGLE child_size = row->widget.gx_widget_size;
	ULONG style = 0;	
	app_gps_connecting_list_prompt_get(&string);		
	child_size.gx_rectangle_left = 50;
	child_size.gx_rectangle_top  = 289;	
	child_size.gx_rectangle_right = 290;
	child_size.gx_rectangle_bottom = 319;
	style = GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE;
	gx_widget_create(&prompt_widget, "promt_widget", &row->child.widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &child_size);
	gx_widget_fill_color_set(&prompt_widget,GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_prompt_create(&row->child.desc, NULL, &row->child.widget, 
		0, style, 0, &child_size);
	gx_prompt_text_color_set(&row->child.desc, GX_COLOR_ID_HONBOW_WHITE, 
		GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&row->child.desc, GX_FONT_ID_SIZE_30);
	gx_prompt_text_set_ext(&row->child.desc, &string);
	gx_widget_event_process_set(&row->child.widget, gps_connecting_window_event);
	gx_widget_draw_set(&row->child.widget, gps_disconnet_window_draw);		
}

GX_WIDGET gps_child;
void app_gps_connecting_window_init(GX_WINDOW *window, SPORT_CHILD_PAGE_T sport_type)
{
	GX_BOOL result;
	GX_RECTANGLE win_size;
	GX_STRING string;
	gx_widget_created_test(&gps_connecting_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
		gx_widget_create(&gps_connecting_window, "gps_connecting_window", window, 
			GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
			0, &win_size);
		gx_widget_fill_color_set(&gps_connecting_window, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);		
		GX_WIDGET *parent = &gps_connecting_window;
		custom_window_title_create(parent, NULL, &gps_title_prompt, &time_prompt);	
		app_sport_list_prompt_get(sport_type, &string);
		gx_prompt_text_set_ext(&gps_title_prompt, &string);		
		

		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
		time_prompt_buff[7] = 0;
		string.gx_string_ptr = time_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&time_prompt, &string);
		gx_widget_created_test(&gps_child, &result);
		if (!result) {
			gx_utility_rectangle_define(&win_size, 0, 58, 318, 359);
			gx_widget_create(&gps_child, NULL, parent, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &win_size);
			gx_widget_fill_color_set(&gps_child, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			gps_disconnet_run_list_row_create(&gps_child);	
		}
		gx_widget_event_process_set(&gps_connecting_window, app_gps_connecting_window_event);		
	}	
}

void app_gps_confirm_text_get(GX_STRING *prompt)
{	
	prompt->gx_string_ptr = "Turn on the app to connect the band and allow the app to access the location.";	
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

UINT gps_confirm_window_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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
		break;
	}
	return status;
}

UINT confirm_button_window_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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
		break;
	}
	return status;
}

UINT confirm_window_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {	
	case GX_SIGNAL(2, GX_EVENT_CLICKED):
		jump_sport_page(&gps_connecting_window, &gps_confirm_window, SCREEN_SLIDE_LEFT);
	break;
	default:
		gx_widget_event_process(widget, event_ptr);
		break;
	}
	return 0;
}

void gps_confirm_window_draw(GX_WIDGET *widget)
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
	gx_context_pixelmap_get(GX_PIXELMAP_ID_APP_SPORT_BTN_L_CONFIRM, &map);
	if (map) {		
		xpos = widget->gx_widget_size.gx_rectangle_left;
		ypos = widget->gx_widget_size.gx_rectangle_top;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

GX_MULTI_LINE_TEXT_INPUT text_input;
#define CONFIRM_TEXT_INPUT_ID 1
static CHAR buffer[100];
static GX_WIDGET confirm_widget;
static VOID gps_confirm_row_create(GX_WIDGET *widget)
{
	GX_RECTANGLE childsize;
	GPS_DISCONNET_LIST_ROW *row = &confirm_window_row;
	GX_STRING string;	

	gx_utility_rectangle_define(&childsize, 0, 60, 318, 359);
	gx_widget_create(&row->widget, "confirm_text", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&row->widget, confirm_window_event);	

	gx_utility_rectangle_define(&childsize, 12, 60, 308, 250);
	gx_widget_create(&row->child.widget, NULL, &row->widget,
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_fill_color_set(&row->child.widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	row->child.icon0 = GX_PIXELMAP_ID_APP_SPORT_BTN_S_CONFIRM;
	GX_RECTANGLE child_size = row->child.widget.gx_widget_size;
	app_gps_confirm_text_get(&string);		
	UINT status;	
	status = gx_multi_line_text_input_create(&text_input, "multi_text", 
				&row->child.widget, buffer, 100, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE|GX_STYLE_TEXT_LEFT,
				CONFIRM_TEXT_INPUT_ID, &child_size);
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

	gx_utility_rectangle_define(&childsize, 12, 283, 309, 349);
	gx_widget_create(&confirm_widget, "confirm_widget", &row->widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 2, &childsize);
	gx_widget_fill_color_set(&confirm_widget, GX_COLOR_ID_HONBOW_GREEN,
		GX_COLOR_ID_HONBOW_GREEN,GX_COLOR_ID_HONBOW_GREEN);
	gx_widget_event_process_set(&confirm_widget, confirm_button_window_event);
	gx_widget_event_process_set(&row->child.widget, gps_confirm_window_event);
	gx_widget_draw_set(&confirm_widget, gps_confirm_window_draw);	
	
}

void app_gps_confirm_window_init(GX_WINDOW *window)
{
	GX_BOOL result;
	GX_RECTANGLE win_size;
	gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
	gx_widget_create(&gps_confirm_window, "gps_confirm_window", window, 
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 0, &win_size);
	gx_widget_fill_color_set(&gps_confirm_window, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
	GX_WIDGET *parent = &gps_confirm_window;
	gx_widget_created_test(&confirm_child, &result);

	if (!result) {
		gx_utility_rectangle_define(&win_size, 0, 60, 318, 359);
		gx_widget_create(&confirm_child, NULL, parent, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &win_size);
		gx_widget_fill_color_set(&confirm_child, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
		gps_confirm_row_create(&confirm_child);	
	}
	gx_widget_event_process_set(&gps_confirm_window, app_gps_confirm_window_event);
}

#define WIDGET_ID_2    2
#define WIDGET_ID_3    3

UINT gps_not_connectd_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	GX_BOOL result;
	switch (event_ptr->gx_event_type) {	

	case GX_SIGNAL(WIDGET_ID_2, GX_EVENT_CLICKED):
		jump_sport_page(&gps_connecting_window, &gps_not_connectd_window, SCREEN_SLIDE_RIGHT);
		return 0;
		break;
	case GX_SIGNAL(WIDGET_ID_3, GX_EVENT_CLICKED):		
		gx_widget_created_test(&count_down_window, &result);
		if (!result) {
			app_count_down_window_init(NULL);
		}
		jump_sport_page(&count_down_window, &gps_not_connectd_window, SCREEN_SLIDE_LEFT);
		return 0;
		break;
	default :
		break;
	}
	return gx_widget_event_process(window, event_ptr);	
}

void app_gps_start_work_text_get(GX_STRING *prompt)
{	
	prompt->gx_string_ptr = "GPS is not connectd Start workout?";	
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

static CHAR buffer1[100];
GX_WIDGET widget0, widget1, widget2;
void gps_not_connectd_window1_draw(GX_WIDGET *widget)
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

void gps_not_connectd_window2_draw(GX_WIDGET *widget)
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

GX_MULTI_LINE_TEXT_INPUT multi_text;
void app_gps_not_connectd_windown_init(GX_WINDOW *window)
{
	GX_RECTANGLE win_size;
	GX_STRING string;	
	UINT status;
	
	gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
	gx_widget_create(&gps_not_connectd_window, "gps_not_connectd_window", window,
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 0, &win_size);
	gx_widget_fill_color_set(&gps_not_connectd_window, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

	GX_WIDGET *parent = &gps_not_connectd_window;	
	gx_utility_rectangle_define(&win_size, 12, 120, 308, 194);
	gx_widget_create(&widget0, "start_work_widget0", parent, 
				GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
				0, &win_size);
	gx_widget_fill_color_set(&widget0, GX_COLOR_ID_HONBOW_BLACK,
				GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);		
	status = gx_multi_line_text_input_create(&multi_text, "multi_text1", 
				&widget0, buffer1, 100, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE|GX_STYLE_TEXT_CENTER,
				CONFIRM_TEXT_INPUT_ID, &win_size);
	app_gps_start_work_text_get(&string);
	if (status == GX_SUCCESS) {
		gx_multi_line_text_view_font_set((GX_MULTI_LINE_TEXT_VIEW *)&multi_text, 
				GX_FONT_ID_SIZE_30);//GX_FONT_ID_TEXT_INPUT
		gx_multi_line_text_input_fill_color_set(&multi_text, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		gx_multi_line_text_input_text_color_set(&multi_text, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
		gx_multi_line_text_view_whitespace_set((GX_MULTI_LINE_TEXT_VIEW *)&multi_text, 0);
		gx_multi_line_text_view_line_space_set((GX_MULTI_LINE_TEXT_VIEW *)&multi_text, 0);
		gx_multi_line_text_input_text_set_ext(&multi_text, &string);
	}

	gx_utility_rectangle_define(&win_size, 12, 283, 155, 348);
	gx_widget_create(&widget1, "start_work_widget1", parent, 
				GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
				WIDGET_ID_2, &win_size);
	gx_widget_fill_color_set(&widget1, GX_COLOR_ID_HONBOW_BLACK,
				GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);	
	gx_widget_event_process_set(&widget1, confirm_button_window_event);
	gx_widget_draw_set(&widget1, gps_not_connectd_window1_draw);

	gx_utility_rectangle_define(&win_size, 165, 283, 308, 348);
	gx_widget_create(&widget2, "start_work_widget1", parent, 
				GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
				WIDGET_ID_3, &win_size);
	gx_widget_fill_color_set(&widget2, GX_COLOR_ID_HONBOW_BLACK,
				GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);	
	gx_widget_event_process_set(&widget2, confirm_button_window_event);
	gx_widget_draw_set(&widget2, gps_not_connectd_window2_draw);

	gx_widget_event_process_set(&gps_not_connectd_window, 
				gps_not_connectd_window_event);
}



