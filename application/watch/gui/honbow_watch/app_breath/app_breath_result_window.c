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
#include "app_breath_window.h"


GX_WIDGET breath_result_window;
// static uint8_t hr = 0;
static GX_PROMPT title_prompt;
static GX_PROMPT time_prompt;
static breath_icon_Widget_T greate_icon, heart_icon, confirm_widget;
static char time_prompt_buff[8];

static GX_PROMPT hr_result_promt;
static char hr_result_promt_buf[4];

static GX_PROMPT hr_result_util_promt;

#define TIME_FRESH_TIMER_ID 2
static GX_VALUE gx_point_x_first;
extern GX_WINDOW_ROOT *root;

UINT app_breath_result_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
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
		// Gui_close_spo2();
		jump_breath_page(&breath_main_page, &breath_result_window, SCREEN_SLIDE_RIGHT);
		gx_widget_delete(&breath_result_window);
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

static VOID breath_result_child_widget_create(GX_WIDGET *widget, uint8_t hr_data)
{
	GX_RECTANGLE childsize;
	GX_STRING string;	
	// uint8_t len;

	gx_utility_rectangle_define(&childsize, 120, 80, 200, 160);
	gx_widget_create(&greate_icon.widget, "greate_icon.widget", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_fill_color_set(&greate_icon.widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	greate_icon.icon = GX_PIXELMAP_ID_APP_BREATH_IC_FINISH;
	_gxe_widget_event_process_set(&greate_icon.widget, breath_child_widget_common_event);
	gx_widget_draw_set(&greate_icon.widget, breath_simple_icon_widget_draw);

	gx_utility_rectangle_define(&childsize, 65, 178, 105, 218);
	gx_widget_create(&heart_icon.widget, "greate_icon.widget", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_fill_color_set(&heart_icon.widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	heart_icon.icon = GX_PIXELMAP_ID_APP_BREATH_IC_HEART_S_MEASURE;
	_gxe_widget_event_process_set(&heart_icon.widget, breath_child_widget_common_event);
	gx_widget_draw_set(&heart_icon.widget, breath_simple_icon_widget_draw);

	if (hr_data) {
		snprintf(hr_result_promt_buf, 4, "%d", hr_data);	
	} else {
		sprintf(hr_result_promt_buf, "%s", "--");
	}
	
	gx_utility_rectangle_define(&childsize, 105, 160, 105 + 34*strlen(hr_result_promt_buf), 220);
	gx_prompt_create(&hr_result_promt, NULL, widget, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
					0, &childsize);
	gx_prompt_text_color_set(&hr_result_promt, GX_COLOR_ID_HONBOW_WHITE, 
		GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
	gx_prompt_font_set(&hr_result_promt, GX_FONT_ID_SIZE_60);
	string.gx_string_ptr = hr_result_promt_buf;
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&hr_result_promt, &string);

	gx_utility_rectangle_define(&childsize, 105 + 34*strlen(hr_result_promt_buf), 183, 105 + 34*strlen(hr_result_promt_buf) + 47, 213);
	gx_prompt_create(&hr_result_util_promt, NULL, widget, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
	gx_prompt_text_color_set(&hr_result_util_promt, GX_COLOR_ID_HONBOW_DARK_BLACK, 
		GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
	gx_prompt_font_set(&hr_result_util_promt, GX_FONT_ID_SIZE_30);
	string.gx_string_ptr = "BPI";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&hr_result_util_promt, &string);	

	gx_utility_rectangle_define(&childsize, 12, 283, 308, 348);
	gx_widget_create(&confirm_widget.widget, "confirm_widget", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 2, &childsize);
	gx_widget_fill_color_set(&confirm_widget.widget, GX_COLOR_ID_HONBOW_BLACK,
		GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&confirm_widget.widget, breath_child_widget_common_event);
	gx_widget_draw_set(&confirm_widget.widget, breath_confirm_rectangle_button_draw);
	confirm_widget.icon = GX_PIXELMAP_ID_APP_BREATH_BTN_L_OK;	
	confirm_widget.bg_color = GX_COLOR_ID_HONBOW_BLUE;
}

void app_breath_result_window_init(GX_WINDOW *window, uint8_t hr_data)
{
	GX_RECTANGLE childsize;
	GX_BOOL result;
	gx_widget_created_test(&breath_result_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 319, 359);
		gx_widget_create(&breath_result_window, "breath_result_window", window, 
						GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
						&childsize);
		gx_widget_event_process_set(&breath_result_window, app_breath_result_window_event);
		GX_WIDGET *parent = &breath_result_window;
		custom_window_title_create(parent, NULL, &title_prompt, &time_prompt);
		GX_STRING string;
		string.gx_string_ptr = get_language_result_title();
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&title_prompt, &string);
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
		time_prompt_buff[7] = 0;
		string.gx_string_ptr = time_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&time_prompt, &string);

		breath_result_child_widget_create(parent, hr_data);
	}
	
}

