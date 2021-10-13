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
#include "app_today_window.h"
#include "custom_animation.h"
#include "windows_manager.h"

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

GX_WIDGET no_sport_record_window;
static GX_PROMPT sport_prompt;
static GX_PROMPT time_prompt;
extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];
static SLEEP_NO_RECORD_WIDGET_T no_record_widget_memory;

static Custom_title_icon_t icon = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_BTN_BACK,
};

extern APP_TODAY_WINDOW_CONTROL_BLOCK app_today_window;
extern GX_WINDOW_ROOT *root;

VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 1
UINT no_sport_record_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {		
		gx_system_timer_start(window, TIME_FRESH_TIMER_ID, 50, 50);		
	} 
	break;
	case GX_EVENT_HIDE: {
		gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);
	} 
	break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}	
	case GX_EVENT_PEN_UP: {
		delta_x = (event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x <= -50) {
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);
			windows_mgr_page_jump((GX_WINDOW *)&app_today_window, NULL, WINDOWS_OP_BACK);
			gx_widget_delete(&no_sport_record_window);
			gx_widget_delete(&today_main_page); 
	 	}
		if (delta_x >= 50) {
			jump_today_page(&today_main_page, &no_sport_record_window, SCREEN_SLIDE_RIGHT);
			gx_widget_delete(&no_sport_record_window);
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
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);
			windows_mgr_page_jump((GX_WINDOW *)&app_today_window, NULL, WINDOWS_OP_BACK);
			gx_widget_delete(&no_sport_record_window);
			gx_widget_delete(&today_main_page); 
			return 0;
		}
		break;
	case GX_EVENT_TIMER: {
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
			return 0;
		}
	} break;
	default:
		break;
	}
	return gx_widget_event_process(window, event_ptr);
}


static const GX_CHAR *get_language_sport_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
#if 0
	case LANGUAGE_CHINESE:
		return "设置";
#endif
	case LANGUAGE_ENGLISH:
		return "Sport";
	default:
		return "Sport";
	}
}

void record_icon_widget_draw(GX_WIDGET *widget);
void app_no_sport_record_widget(GX_WIDGET *widget)
{
	GX_RECTANGLE childsize;
	GX_STRING string;
	SLEEP_NO_RECORD_WIDGET_T *row = &no_record_widget_memory;
	gx_utility_rectangle_define(&childsize, 0, 58, 319, 359);
	gx_widget_create(&row->widget, NULL, widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
	gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
							GX_COLOR_ID_HONBOW_BLACK);
	
	gx_utility_rectangle_define(&childsize, 120, 120, 200, 200);
	gx_widget_create(&row->child, NULL, &row->widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_event_process_set(&row->child, today_child_widget_common_event);
	gx_widget_fill_color_set(&row->child, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
							GX_COLOR_ID_HONBOW_BLACK);
	row->icon = GX_PIXELMAP_ID_IC_SPORT_NO_GRAY;
	gx_widget_draw_set(&row->child,  record_icon_widget_draw);

	gx_utility_rectangle_define(&childsize, 12, 206, 308, 236);
	gx_prompt_create(&row->decs, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
						0, &childsize);
	gx_prompt_text_color_set(&row->decs, 
				GX_COLOR_ID_HONBOW_DARK_GRAY, 
				GX_COLOR_ID_HONBOW_DARK_GRAY,
				GX_COLOR_ID_HONBOW_DARK_GRAY);
	gx_prompt_font_set(&row->decs, GX_FONT_ID_SIZE_30);
	string.gx_string_ptr = "No record";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&row->decs, &string);
}

void app_no_sport_record_window_init(GX_WINDOW *window)
{
	GX_RECTANGLE win_size;
	GX_BOOL result;
	gx_widget_created_test(&no_sport_record_window, &result);
	if (!result)
	{
		gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
		gx_widget_create(&no_sport_record_window, "no_sport_widget", window, 
			GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
			0, &win_size);
		gx_widget_fill_color_set(&no_sport_record_window, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
		gx_widget_event_process_set(&no_sport_record_window, no_sport_record_window_event);
		
		GX_WIDGET *parent = &no_sport_record_window;
		custom_window_title_create(parent, &icon, &sport_prompt, &time_prompt);

		GX_STRING string;
		string.gx_string_ptr = get_language_sport_title();
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&sport_prompt, &string);
	
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
		time_prompt_buff[7] = 0;
		string.gx_string_ptr = time_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&time_prompt, &string);		
		app_no_sport_record_widget(parent);
	}	
}


