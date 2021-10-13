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
#include "app_spo2_window.h"


GX_WIDGET spo2_result_window;
static uint8_t spo2 = 0;
static GX_PROMPT spo2_prompt;
static GX_PROMPT time_prompt;
static icon_Widget_T icon_widget, confirm_widget;
extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];

static GX_PROMPT result_promt;
static char result_promt_buf[4];

static GX_PROMPT result_util_promt;

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
#define TIME_FRESH_TIMER_ID 2
static GX_VALUE gx_point_x_first;

UINT app_spo2_result_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	GX_STRING string;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {		
		gx_system_timer_start(window, TIME_FRESH_TIMER_ID, 50, 50);	
	} 
	break;
	case GX_EVENT_HIDE: {
		gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);	
	} 
	break;
	case GX_SIGNAL(2, GX_EVENT_CLICKED):
		Gui_close_spo2();
		jump_spo2_page(&spo2_main_page, &spo2_result_window, SCREEN_SLIDE_LEFT);
		gx_widget_delete(&spo2_result_window);
	break;
	// case GX_EVENT_KEY_UP:
	// 	if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
	// 		windows_mgr_page_jump((GX_WINDOW *)&app_spo2_window, NULL, WINDOWS_OP_BACK);		
	// 		// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);			
	// 		return 0;
	// 	}
	break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}		
	case GX_EVENT_TIMER: {
		if (event_ptr->gx_event_payload.gx_event_timer_id == TIME_FRESH_TIMER_ID) {			
			struct timespec ts;
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
	} 
	break;
	default:
		break;
	}
	return gx_widget_event_process(window, event_ptr);
}

static const GX_CHAR *get_language_result_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_ENGLISH:
		return "Result";
	default:
		return "Result";
	}
}

void spo2_child_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr);
void wear_confirm_rectangle_button_draw(GX_WIDGET *widget);
void spo2_icon_widget_draw(GX_WIDGET *widget);

static VOID spo2_result_child_widget_create(GX_WIDGET *widget, uint8_t spo2_data)
{
	GX_RECTANGLE childsize;
	GX_STRING string;	

	gx_utility_rectangle_define(&childsize, 131, 88, 182, 153);
	gx_widget_create(&icon_widget.widget, "icon_widget", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_fill_color_set(&icon_widget.widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	icon_widget.icon = GX_PIXELMAP_ID_APP_SPO2_BLOOD;
	gx_widget_draw_set(&icon_widget.widget, spo2_icon_widget_draw);

	gx_utility_rectangle_define(&childsize, 8, 160, 185, 220);
	gx_prompt_create(&result_promt, NULL, widget, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_RIGHT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
	gx_prompt_text_color_set(&result_promt, GX_COLOR_ID_HONBOW_WHITE, 
		GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
	gx_prompt_font_set(&result_promt, GX_FONT_ID_SIZE_60);
	snprintf(result_promt_buf, 4, "%d", spo2_data%101);		
	string.gx_string_ptr = result_promt_buf;
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&result_promt, &string);

	gx_utility_rectangle_define(&childsize, 189, 183, 212, 213);
	gx_prompt_create(&result_util_promt, NULL, widget, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_RIGHT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
	gx_prompt_text_color_set(&result_util_promt, GX_COLOR_ID_HONBOW_DARK_BLACK, 
		GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
	gx_prompt_font_set(&result_util_promt, GX_FONT_ID_SIZE_30);
	string.gx_string_ptr = "%%";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&result_util_promt, &string);

	gx_utility_rectangle_define(&childsize, 12, 282, 308, 347);
	gx_widget_create(&confirm_widget.widget, "confirm_widget", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 2, &childsize);
	gx_widget_fill_color_set(&confirm_widget.widget, GX_COLOR_ID_HONBOW_BLACK,
		GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&confirm_widget.widget, spo2_child_widget_common_event);
	gx_widget_draw_set(&confirm_widget.widget, wear_confirm_rectangle_button_draw);
	confirm_widget.icon = GX_PIXELMAP_ID_APP_SPO2_OK;		
}

void app_spo2_result_window_init(GX_WINDOW *window, uint8_t spo2_data)
{
	GX_RECTANGLE childsize;
	GX_BOOL result;
	gx_widget_created_test(&spo2_result_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 319, 359);
		gx_widget_create(&spo2_result_window, "spo2_result_window", window, 
						GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
						&childsize);
		gx_widget_event_process_set(&spo2_result_window, app_spo2_result_window_event);
		GX_WIDGET *parent = &spo2_result_window;
		custom_window_title_create(parent, NULL, &spo2_prompt, &time_prompt);
		GX_STRING string;
		string.gx_string_ptr = get_language_result_title();
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&spo2_prompt, &string);
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
		time_prompt_buff[7] = 0;
		string.gx_string_ptr = time_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&time_prompt, &string);

		spo2_result_child_widget_create(parent, spo2_data);
	}
	
}

