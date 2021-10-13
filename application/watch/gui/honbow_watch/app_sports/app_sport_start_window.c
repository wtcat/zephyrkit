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
// #include "cywee_motion.h"

// Sport_cfg_t param_cfg = {0};

struct timespec start_sport_ts;

#define WIDGET_ID_0  1
GX_WIDGET sport_start_window;
GX_WIDGET count_down_window;

static Custom_title_icon_t icon_title = {
	.bg_color = GX_COLOR_ID_HONBOW_BLACK,
	.icon = GX_PIXELMAP_ID_APP_SPORT_IC_GPS_ON,
};

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

static GX_PROMPT time_prompt;
extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];

typedef struct {
	GX_WIDGET widget0;
	GX_WIDGET widget1;
	GX_RESOURCE_ID icon0;
	GX_RESOURCE_ID icon1;
	GX_PROMPT desc;
} Sport_StartChildWindow_TypeDef;

typedef struct SPORT_STARAT_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	Sport_StartChildWindow_TypeDef child;
} SPORT_START_LIST_ROW;

SPORT_START_LIST_ROW sport_start_list_momery;

static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 1

void app_sport_sport_list_prompt_get(GX_STRING *prompt)
{	
	prompt->gx_string_ptr = "Start";
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

UINT sport_start_icon_start_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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

UINT sport_start_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
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
	 	delta_x = GX_ABS(
			 event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - 
			 gx_point_x_first);
	 	if (delta_x >= 50) {
			;
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
		}		
		return 0;
	default :
		break;
	}
	return gx_widget_event_process(window, event_ptr);	
}

UINT sport_start_widget_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	//static UINT count_timer = 0;
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(WIDGET_ID_0, GX_EVENT_CLICKED):
		app_count_down_window_init(NULL);
		jump_sport_page(&count_down_window, &sport_start_window, SCREEN_SLIDE_LEFT);
		clock_gettime(CLOCK_REALTIME, &start_sport_ts);		
		start_sport_ts.tv_sec += 4;		
		break;	
	default :
		break;
	}
	return gx_widget_event_process(window, event_ptr);	
}


void sport_start_widget_draw(GX_WIDGET *widget)
{
	GX_RECTANGLE fillrect;
	SPORT_START_LIST_ROW *element = CONTAINER_OF(widget, SPORT_START_LIST_ROW, widget);
	//GX_STRING string;
	gx_widget_client_get(&element->child.widget0, 0, &fillrect);

	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);
	INT xpos;
	INT ypos;
	//GX_RESOURCE_ID color = GX_COLOR_ID_HONBOW_CONFIRM_GREEN;
	GX_COLOR color;
	gx_context_color_get(GX_COLOR_ID_HONBOW_CONFIRM_GREEN, &color);

	if (element->child.widget0.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		uint8_t red = ((color >> 11) >> 1);
		uint8_t green = (((color >> 5) & 0x3f) >> 1);
		uint8_t blue = ((color & 0x1f) >> 1);
		color = (red << 11) + (green << 5) + blue;
	}

	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);

	gx_brush_define(&current_context->gx_draw_context_brush, color, color, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);

	gx_context_brush_width_set(1);
	gx_canvas_circle_draw(
		((fillrect.gx_rectangle_right - fillrect.gx_rectangle_left + 1) >> 1) + 
		fillrect.gx_rectangle_left+1,
		((fillrect.gx_rectangle_bottom - fillrect.gx_rectangle_top + 1) >> 1) + 
		fillrect.gx_rectangle_top+1, 75);
	GX_PIXELMAP *map = NULL;
	gx_context_pixelmap_get(element->child.icon0, &map);

	if (map) {
		xpos = fillrect.gx_rectangle_left;
		ypos = fillrect.gx_rectangle_top; 		
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}	
	gx_widget_children_draw(widget);
}

static VOID app_sport_start_row_create(GX_WIDGET *widget)
{
	SPORT_START_LIST_ROW *row = &sport_start_list_momery;
	GX_RECTANGLE win_size;
	GX_STRING string;
	gx_utility_rectangle_define(&win_size, 0, 0, 319, 359);
	gx_widget_create(&row->widget, "sport_start", widget, 
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
		0, &win_size);
	gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	
	GX_WIDGET *parent = &row->widget;
	gx_utility_rectangle_define(&win_size, 85, 88, 235, 238);
	gx_widget_create(&row->child.widget0, "sport_start", parent, 
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
		WIDGET_ID_0, &win_size);
	gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	row->child.icon0 = GX_PIXELMAP_ID_APP_SPORT_BTN_START;
	gx_widget_event_process_set(&row->child.widget0, sport_start_icon_start_event);

	gx_utility_rectangle_define(&win_size, 127, 288, 193, 323);
	gx_widget_create(&row->child.widget1, "start_string", parent, 
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
		0, &win_size);
	gx_widget_fill_color_set(&row->child.widget1, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

	gx_prompt_create(&row->child.desc, NULL, &row->child.widget1, 
			0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
			0, &win_size);
	gx_prompt_text_color_set(&row->child.desc, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&row->child.desc, GX_FONT_ID_SIZE_30);
	app_sport_sport_list_prompt_get(&string);
	gx_prompt_text_set_ext(&row->child.desc, &string);

	gx_widget_draw_set(&row->widget, sport_start_widget_draw);
	gx_widget_event_process_set(&row->widget, sport_start_widget_event);
}

void app_sport_start_window_init(GX_WINDOW *window)
{
	GX_RECTANGLE win_size;
	GX_STRING string;
	GX_BOOL result;
	gx_widget_created_test(&sport_start_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
		gx_widget_create(&sport_start_window, "sport_start_window", window, 
			GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
			0, &win_size);
		gx_widget_fill_color_set(&sport_start_window, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		
		GX_WIDGET *parent = &sport_start_window;
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

		app_sport_start_row_create(parent);
		gx_widget_event_process_set(&sport_start_window, sport_start_window_event);
	}
}

GX_RESOURCE_ID count_down_icon = GX_PIXELMAP_ID_COUNT_DOWN_3;

void count_down_window_draw(GX_WIDGET *widget)
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

	GX_PIXELMAP *map = NULL;
	if (count_down_icon){
		gx_context_pixelmap_get(count_down_icon, &map);
	}	

	if (map) {
		xpos = fillrect.gx_rectangle_left;
		ypos = fillrect.gx_rectangle_top; 		
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

extern GX_WIDGET sport_data_window;
UINT count_down_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
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
	case GX_EVENT_TIMER: 
		count_timer++;
		switch (count_timer) {		
		case 1:
			count_down_icon = GX_PIXELMAP_ID_COUNT_DOWN_2;
			gx_system_dirty_mark(&count_down_window);
			return 0;
			break;
		case 2:
			count_down_icon = GX_PIXELMAP_ID_COUNT_DOWN_1;
			gx_system_dirty_mark(&count_down_window);
			return 0;
		break;
		case 3:
			count_down_icon = GX_PIXELMAP_ID_COUNT_DOWN_GO;
			gx_system_dirty_mark(&count_down_window);
			return 0;
		break;		
		default:
			gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);			
			//app_sport_data_window_init(NULL);
			jump_sport_page(&sport_data_window, &count_down_window, SCREEN_SLIDE_LEFT);	
			gui_sport_cfg.status = SPORT_START;
			//gui_set_sport_param(&gui_sport_cfg);
			gx_widget_delete(&count_down_window);
			count_down_icon = GX_PIXELMAP_ID_COUNT_DOWN_3;		
			return 0;
			break;
		}
		return 0;
	default :
		break;
	}
	return gx_widget_event_process(window, event_ptr);	
}

GX_WIDGET count_down_window_child;
void app_count_down_window_init(GX_WINDOW *window)
{
	GX_RECTANGLE win_size;
	GX_BOOL result;
	gx_widget_created_test(&count_down_window, &result);
	if (!result) {
		count_down_icon = GX_PIXELMAP_ID_COUNT_DOWN_3;
		gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
		gx_widget_create(&count_down_window, "count_down_window", window, 
			GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
			0, &win_size);
		gx_widget_fill_color_set(&count_down_window, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

		gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
		gx_widget_create(&count_down_window_child, NULL, &count_down_window, 
			GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
			0, &win_size);
		gx_widget_fill_color_set(&count_down_window_child, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_draw_set(&count_down_window_child, count_down_window_draw);
		gx_widget_event_process_set(&count_down_window_child, count_down_window_event);	
	}
}

