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

#define TIME_FRESH_TIMER_ID 1

GX_WIDGET breath_attention_window;
static GX_WIDGET breath_multi_line_widget;
static GX_MULTI_LINE_TEXT_INPUT text_input;
static CHAR buffer[100];

UINT breath_attention_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	static uint32_t count = 0;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {		
		gx_system_timer_start(window, TIME_FRESH_TIMER_ID, 50, 50);	
		count = 0;		
	} 
	break;
	case GX_EVENT_HIDE: {
		gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);	
	} 
	break;
	// case GX_EVENT_PEN_DOWN: {
	// 	gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
	// 	break;
	// }	
	// case GX_EVENT_PEN_UP: {
	//  	delta_x = (event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
	//  	if (delta_x >= 50) {
					
	//  	}
	// 	break;
	// }	
	// case GX_EVENT_KEY_UP:
	// 	if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
	// 		windows_mgr_page_jump((GX_WINDOW *)&app_spo2_window, NULL, WINDOWS_OP_BACK);		
	// 		// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);			
	// 		return 0;
	// 	}
	// 	break;
	case GX_EVENT_TIMER: {
		if (event_ptr->gx_event_payload.gx_event_timer_id == TIME_FRESH_TIMER_ID) {			
			count++;
			if (count == 2) {
				app_breath_inhale_window_init(NULL);
				jump_breath_page(&breath_inhale_window, &breath_attention_window, SCREEN_SLIDE_LEFT);
			}
			return 0;
		}
	} 
	break;
	default:
		break;
	}
	return gx_widget_event_process(window, event_ptr);
}

void breath_attention_text_get(GX_STRING *prompt)
{	
	prompt->gx_string_ptr = "Pay attention to breathing";	
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

void app_breath_attention_window_init(GX_WINDOW *window)
{
	GX_RECTANGLE childsize;
	GX_BOOL result;
	GX_STRING string;
	gx_widget_created_test(&breath_attention_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 319, 359);
		gx_widget_create(&breath_attention_window, "breath_attention_window", window, 
						GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
						&childsize);		
		GX_WIDGET *parent = &breath_attention_window;

		gx_utility_rectangle_define(&childsize, 43, 142, 278, 218);
		gx_widget_create(&breath_multi_line_widget, NULL, parent, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_fill_color_set(&breath_multi_line_widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_event_process_set(&breath_multi_line_widget, breath_child_widget_common_event);
		breath_attention_text_get(&string);		
		UINT status;	
		status = gx_multi_line_text_input_create(&text_input, "multi_text", &breath_multi_line_widget, buffer, 100, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE|GX_STYLE_TEXT_CENTER, GX_ID_NONE, &childsize);
		if (status == GX_SUCCESS) {
			gx_multi_line_text_view_font_set( (GX_MULTI_LINE_TEXT_VIEW *)&text_input, GX_FONT_ID_SIZE_30);
			gx_multi_line_text_input_fill_color_set(&text_input, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			gx_multi_line_text_input_text_color_set(&text_input, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_multi_line_text_view_whitespace_set((GX_MULTI_LINE_TEXT_VIEW *)&text_input, 0);
			gx_multi_line_text_view_line_space_set((GX_MULTI_LINE_TEXT_VIEW *)&text_input, 0);
			gx_multi_line_text_input_text_set_ext(&text_input, &string);
		}	
		
		gx_widget_event_process_set(&breath_attention_window, breath_attention_window_event);
	}
	

}

