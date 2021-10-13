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

struct timespec end_sport_ts;
extern struct timespec start_sport_ts;

#define WIDGET_ID_0  1
GX_WIDGET sport_paused_window;
extern GX_WIDGET sport_data_window;

typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID bg_color;
	GX_RESOURCE_ID icon;
} sport_paused_icon_t;

typedef struct Sport_Paused_Row_Struct {
	GX_WIDGET widget;
	sport_paused_icon_t child0;
	sport_paused_icon_t child1;
	sport_paused_icon_t child2;
}SPORT_PAUSED_ROW_T;
SPORT_PAUSED_ROW_T sport_paused_momery;

static Custom_title_icon_t icon_title = {
	.bg_color = GX_COLOR_ID_HONBOW_BLACK,
	.icon = GX_PIXELMAP_ID_APP_SPORT_IC_GPS_DOWN,
};

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);
static GX_PROMPT time_prompt;
extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 1
static GX_PROMPT Paused_prompt;

static const GX_CHAR *lauguage_en_sport_data_element[1] = {"Paused"};
static const GX_CHAR **sport_data_string[] = {
	[LANGUAGE_ENGLISH] = lauguage_en_sport_data_element};
static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return sport_data_string[id];
}

void app_sport_paused_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

bool get_gps_connet_status(void);
UINT sport_paused_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{		
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: 
		gx_system_timer_start(window, TIME_FRESH_TIMER_ID, 50, 50);

	break;
	case GX_EVENT_HIDE: 
		gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);	
	break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}	
	case GX_EVENT_PEN_UP: {
	 	delta_x =  gx_point_x_first - event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
	 	if (delta_x >= 50) {
			 printf("&&&&&&&&&&&&&  add later!! sport_paused_window_event -> music ctrl window\n");
	 	}
		break;
	}	
	case GX_EVENT_TIMER: 
		if (event_ptr->gx_event_payload.gx_event_timer_id == TIME_FRESH_TIMER_ID) {
			struct timespec ts;
			GX_STRING string;
			if (get_gps_connet_status()) {
				icon_title.icon = GX_PIXELMAP_ID_APP_SPORT_IC_GPS_ON;
			}else {
				icon_title.icon = GX_PIXELMAP_ID_APP_SPORT_IC_GPS_DOWN;
			}
			clock_gettime(CLOCK_REALTIME, &ts);
			time_t now = ts.tv_sec;
			struct tm *tm_now = gmtime(&now);
			snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
			time_prompt_buff[7] = 0;
			string.gx_string_ptr = time_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);			
			gx_prompt_text_set_ext(&time_prompt, &string);	
		}		
		return 0;
	default :
		break;
	}
	return gx_widget_event_process(window, event_ptr);	
}

void set_sport_duration(uint32_t time);
 UINT sport_paused_widget_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
 {
	UINT status = 0;
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(3, GX_EVENT_CLICKED):
		jump_sport_page(&sport_data_window, &sport_paused_window, SCREEN_SLIDE_LEFT);
		gui_sport_cfg.status = SPORT_START;
//		gui_set_sport_param(&gui_sport_cfg);
	break;

	case GX_SIGNAL(2, GX_EVENT_CLICKED):
		clock_gettime(CLOCK_REALTIME, &end_sport_ts);
		if(sport_time_count > 60) {
			set_sport_duration(sport_time_count);
			app_sport_summary_window_init(NULL, true);				
			jump_sport_page(&sport_summary_window, &sport_paused_window, SCREEN_SLIDE_LEFT);		gui_sport_cfg.status = SPORT_END;
			gui_sport_cfg.work_long = sport_time_count;
			gui_sport_cfg.gui_start = start_sport_ts.tv_sec;
			gui_sport_cfg.gui_end = end_sport_ts.tv_sec;
//			gui_set_sport_param(&gui_sport_cfg);
			sport_time_count = 0;				
		} else {				
			jump_sport_page(&sport_time_short_window, &sport_paused_window, SCREEN_SLIDE_LEFT);
		}			
	break;
	default:
		gx_widget_event_process(widget, event_ptr);
		break;
	}
	return status;
 }

UINT sport_paused_child_widget_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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

void pause_child_widget_draw(GX_WIDGET *widget)
{
	GX_RECTANGLE fillrect;
	sport_paused_icon_t *icon = CONTAINER_OF(widget, sport_paused_icon_t, widget);
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
	gx_context_color_get(icon->bg_color, &color);

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
	gx_context_pixelmap_get(icon->icon, &map);
	if (map) {		
		xpos = widget->gx_widget_size.gx_rectangle_left;
		ypos = widget->gx_widget_size.gx_rectangle_top;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

void sport_paused_widget_create(GX_WIDGET *parent)
{
	GX_RECTANGLE size;
	GX_STRING string;
	SPORT_PAUSED_ROW_T *row = &sport_paused_momery;
	gx_utility_rectangle_define(&size, 0, 58, 319, 359);
	gx_widget_create(&row->widget, NULL, parent, GX_STYLE_ENABLED, 0, &size);
	gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&row->widget, sport_paused_widget_event);

	gx_utility_rectangle_define(&size, 110, 155, 211, 185);
	gx_widget_create(&row->child0.widget, NULL, &row->widget, GX_STYLE_ENABLED, 1, &size);
	gx_widget_fill_color_set(&row->child0.widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_prompt_create(&Paused_prompt, NULL, &row->child0.widget, 0, 
		GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE,	0, &size);
	gx_prompt_text_color_set(&Paused_prompt, GX_COLOR_ID_HONBOW_WHITE, 
		GX_COLOR_ID_HONBOW_WHITE,GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&Paused_prompt, GX_FONT_ID_SIZE_30);
	app_sport_paused_list_prompt_get(0, &string);
	gx_prompt_text_set_ext(&Paused_prompt, &string);
	
	row->child1.icon = GX_PIXELMAP_ID_APP_SPORT_BTN_S_STOP;
	row->child1.bg_color = GX_COLOR_ID_HONBOW_RED;
	row->child2.icon = GX_PIXELMAP_ID_APP_SPORT_BTN_S_CONFIRM;
	row->child2.bg_color = GX_COLOR_ID_HONBOW_GREEN;	
	gx_utility_rectangle_define(&size, 12, 283, 155, 348);
	gx_widget_create(&row->child1.widget, NULL, &row->widget, GX_STYLE_ENABLED, 2, &size);
	gx_widget_fill_color_set(&row->child1.widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&row->child1.widget, sport_paused_child_widget_event);
	gx_widget_draw_set(&row->child1.widget, pause_child_widget_draw);

	gx_utility_rectangle_define(&size, 165, 283, 308, 348);
	gx_widget_create(&row->child2.widget, NULL, &row->widget, GX_STYLE_ENABLED, 3, &size);
	gx_widget_fill_color_set(&row->child2.widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&row->child2.widget, sport_paused_child_widget_event);
	gx_widget_draw_set(&row->child2.widget, pause_child_widget_draw);
}

void app_sport_paused_window_init(GX_WINDOW *window)
{
	GX_RECTANGLE win_size;
	GX_STRING string;
	gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
	gx_widget_create(&sport_paused_window, "sport_paused_window", window, 
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
		0, &win_size);
	gx_widget_fill_color_set(&sport_paused_window, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
	gx_widget_event_process_set(&sport_paused_window, sport_paused_window_event);
	
	GX_WIDGET *parent = &sport_paused_window;

	if (get_gps_connet_status()) {
				icon_title.icon = GX_PIXELMAP_ID_APP_SPORT_IC_GPS_ON;
	}else {
		icon_title.icon = GX_PIXELMAP_ID_APP_SPORT_IC_GPS_DOWN;
	}
	custom_window_title_create(parent, &icon_title, NULL, &time_prompt);	
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	time_t now = ts.tv_sec;
	struct tm *tm_now = gmtime(&now);
	snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
	time_prompt_buff[7] = 0;
	string.gx_string_ptr = time_prompt_buff;
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&time_prompt, &string);	
	sport_paused_widget_create(parent);	
}

