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


GX_WIDGET breath_inhale_window;
static GX_PROMPT inhale_prompt;

#define TIME_FRESH_TIMER_ID 12
static uint16_t max_count = 0;
static uint8_t R = 15;
static uint8_t R_MIN = 20;

int inhale_child_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static uint32_t count = 0;
	static uint32_t sum_count = 0;
	static uint32_t hr_count = 0, hr_sum = 0;;
	int hr = 0;
	// uint32_t data = 0;	
	GX_STRING string;
	static uint8_t inhale_flage = 1;
	static uint8_t inhale_count = 0;
	switch (event_ptr->gx_event_type) {	
	case GX_EVENT_SHOW: {		
		gx_system_timer_start(widget, TIME_FRESH_TIMER_ID, 2, 2);
		inhale_flage = 1;
		count = 0;
		sum_count = 0;
		hr_count = 0;
		hr_sum = 0;
		inhale_count = 0;
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		printk("open time : %02d:%02d:%02d\n", tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
		Gui_open_hr();		
	} 
	break;
	case GX_EVENT_HIDE: {
		gx_system_timer_stop(widget, TIME_FRESH_TIMER_ID);	
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		printk("close time : %02d:%02d:%02d, sum_count = %d\n", tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec, sum_count);
		Gui_close_hr();
	} 
	break;
	case GX_EVENT_TIMER: {
		count++;
		sum_count++;		
		if(0 == sum_count % 100) {
			hr = sport_get_hr();
			if (hr) {
				hr_sum += hr;
				hr_count++;
			}
		}
		// printf("count : %d\n", count);
		if (event_ptr->gx_event_payload.gx_event_timer_id == TIME_FRESH_TIMER_ID) {	
			if (inhale_flage) {
				if (count >=  max_count) {
					count = 0;
					inhale_flage = false;
					string.gx_string_ptr = "exhale";
					string.gx_string_length = strlen(string.gx_string_ptr);
					gx_prompt_text_set_ext(&inhale_prompt, &string);
					gx_system_timer_start(widget, TIME_FRESH_TIMER_ID, 5, 2);
					return 0;
				}	
				R = (107 * count)/max_count + 25;
				R_MIN = (93 * count)/max_count + 32;
			} else {
				if (count >=  max_count) {
					count = 0;
					inhale_flage = true;
					inhale_count++;
					string.gx_string_ptr = "inhale";
					string.gx_string_length = strlen(string.gx_string_ptr);
					gx_prompt_text_set_ext(&inhale_prompt, &string);
					gx_system_timer_start(widget, TIME_FRESH_TIMER_ID, 5, 2);
					if(inhale_count == breath_data.breath_time * breath_data.breath_freq){
						if (hr_count) {							 
							app_breath_result_window_init(NULL, hr_sum/hr_count);
						}
						else {
							app_breath_result_window_init(NULL, 86);
						}						
						jump_breath_page(&breath_result_window, &breath_inhale_window, SCREEN_SLIDE_RIGHT);
						gx_widget_delete(&breath_inhale_window);
					}
					return 0;
				}	
				R = (107 * (max_count - count))/max_count + 25;
				R_MIN = (93 * (max_count - count))/max_count+ 32;
			}			
			gx_system_dirty_mark(widget);
			// gx_system_timer_start(widget, TIME_FRESH_TIMER_ID, 2, 2);
			return 0;
		}
	} 
	break;
	default:		
	break;
	}	
	gx_widget_event_process(widget, event_ptr);
	return 0;
}

void inhale_circle_icon_widget_draw(GX_WIDGET *widget)
{
	// breath_icon_Widget_T *row = CONTAINER_OF(widget, breath_icon_Widget_T, widget);
	GX_RECTANGLE fillrect;
	// INT xpos;
	// INT ypos;
	
	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
	gx_canvas_rectangle_draw(&fillrect);
	GX_COLOR color;
	
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);

	gx_context_color_get(GX_COLOR_ID_HONBOW_BLUE_35_PERCENT, &color);	
	gx_brush_define(&current_context->gx_draw_context_brush, color, color, GX_BRUSH_ALIAS|GX_BRUSH_ROUND);
	gx_context_brush_width_set(35);
	gx_canvas_circle_draw(
		((fillrect.gx_rectangle_right - fillrect.gx_rectangle_left + 1) >> 1) + 
		fillrect.gx_rectangle_left+1,
		((fillrect.gx_rectangle_bottom -50 - fillrect.gx_rectangle_top + 1) >> 1) + 
		fillrect.gx_rectangle_top+1, R);


	gx_context_color_get(GX_COLOR_ID_HONBOW_BLUE, &color);
	gx_brush_define(&current_context->gx_draw_context_brush, color, color, GX_BRUSH_ALIAS|GX_BRUSH_ROUND);
	gx_context_brush_width_set(15);
	gx_canvas_circle_draw(
		((fillrect.gx_rectangle_right - fillrect.gx_rectangle_left + 1) >> 1) + 
		fillrect.gx_rectangle_left+1,
		((fillrect.gx_rectangle_bottom -50 - fillrect.gx_rectangle_top + 1) >> 1) + 
		fillrect.gx_rectangle_top+1, R_MIN);		
	gx_widget_children_draw(widget);		
	
}


static uint32_t delta_x;
static uint32_t gx_point_x_first;
UINT app_breath_inhale_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	// GX_STRING string;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {			
	} 
	break;
	// case GX_EVENT_HIDE: {
	// 	gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);	
	// } 
	// break;
	// case GX_EVENT_PEN_DOWN: {
	// 	gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
	// 	break;
	// }	
	case GX_EVENT_PEN_UP: {
	 	delta_x = (event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
	 	if (delta_x >= 50) {
					
	 	}
		break;
	}	
	// case GX_SIGNAL(START_ICON, GX_EVENT_CLICKED):
	// 	jump_breath_page(&breath_attention_window, &breath_main_page, SCREEN_SLIDE_RIGHT);
	// 	break;
	case GX_EVENT_KEY_UP:
		// if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
		// 	windows_mgr_page_jump((GX_WINDOW *)&app_spo2_window, NULL, WINDOWS_OP_BACK);		
		// 	// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);			
		// 	return 0;
		// }
		break;
	case GX_EVENT_TIMER: {
		// if (event_ptr->gx_event_payload.gx_event_timer_id == TIME_FRESH_TIMER_ID) {			
		// 	struct timespec ts;
		// 	clock_gettime(CLOCK_REALTIME, &ts);
		// 	time_t now = ts.tv_sec;
		// 	struct tm *tm_now = gmtime(&now);
		// 	snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
		// 	time_prompt_buff[7] = 0;
		// 	string.gx_string_ptr = time_prompt_buff;
		// 	string.gx_string_length = strlen(string.gx_string_ptr);
		// 	gx_prompt_text_set_ext(&time_prompt, &string);	
		// 	return 0;
		// }
	} 
	break;
	default:
		break;
	}
	return gx_widget_event_process(window, event_ptr);
}

void app_breath_inhale_window_init(GX_WINDOW *window)
{
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&breath_inhale_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 319, 359);
		gx_widget_create(&breath_inhale_window, "breath_inhale_window", window, 
					GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childsize);
		gx_widget_fill_color_set(&breath_inhale_window, GX_COLOR_ID_HONBOW_BLUE, 
			GX_COLOR_ID_HONBOW_BLUE, GX_COLOR_ID_HONBOW_BLUE);
		gx_widget_draw_set(&breath_inhale_window, inhale_circle_icon_widget_draw);
		gx_widget_event_process_set(&breath_inhale_window, inhale_child_widget_common_event);			
		GX_WIDGET *parent = &breath_inhale_window;
		gx_utility_rectangle_define(&childsize, 12, 310, 308, 348);
		gx_prompt_create(&inhale_prompt, NULL, parent, 0,GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 0, &childsize);
		gx_prompt_text_color_set(&inhale_prompt, GX_COLOR_ID_HONBOW_WHITE,GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
		gx_prompt_font_set(&inhale_prompt, GX_FONT_ID_SIZE_30);		
		string.gx_string_ptr = "inhale";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&inhale_prompt, &string);
		max_count = (30000/46.3)/breath_data.breath_freq + 0.5;
		
		printk("max_count = %d\n", max_count);
	}
	
}

