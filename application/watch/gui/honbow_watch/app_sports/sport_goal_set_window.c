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


GX_WIDGET sport_goal_set_window;

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);


#define MAIN_WINDOW_TIMER_ID 1
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;


#define BUTTON_MINUS_ID 1
#define BUTTON_PLUS_ID 2
#define BUTTON_CONFIRM_ID 3

#define SETTING_CHILD   5
extern GX_WINDOW_ROOT *root;
typedef struct {
	GX_WIDGET widget;
	GX_PROMPT decs;
	GX_RESOURCE_ID icon;
	GX_RESOURCE_ID bg_color;
	GX_RESOURCE_ID nor_color;
}COMMON_BUTTON_T;

typedef struct {
	GX_WIDGET widget;	
	COMMON_BUTTON_T child[SETTING_CHILD];
	SPORT_CHILD_PAGE_T sport_type;
	GOAL_SETTING_TYPE_T goal_type;
	int32_t data;
}GOAL_SETTING_WIDGET_T;

GOAL_SETTING_WIDGET_T goal_setting_widget_momery;

static Custom_title_icon_t icon_title = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_APP_SPORT_BTN_BACK,
};

static GX_PROMPT setting_prompt;
static GX_PROMPT time_prompt;
static char time_prompt_buff[8];
static GX_PROMPT goal_prompt;
static char goal_buff[15];
static GX_PROMPT name_prompt;


static const GX_CHAR *lauguage_en_goal_units_element[14] = { "Free","Calorise", "km", "min", "meter"};

static const GX_CHAR **goal_string[] = {
	//[LANGUAGE_CHINESE] = lauguage_ch_setting_element,
	[LANGUAGE_ENGLISH] = lauguage_en_goal_units_element};
static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return goal_string[id];
}

void app_goal_units_string_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

void set_goal_string(GX_STRING *string, GOAL_SETTING_TYPE_T type, int32_t data)
{	
	switch (type) {
	case CALORISE_GOAL:
		snprintf(goal_buff, 15, "%d", data/100);
		string->gx_string_ptr = goal_buff;
		string->gx_string_length = strlen(string->gx_string_ptr);
		break;
	case DISTANCE_GOAL:
		snprintf(goal_buff, 15, "%d.%02d", data/100, data%100);
		string->gx_string_ptr = goal_buff;
		string->gx_string_length = strlen(string->gx_string_ptr);
		break;
	case TIME_GOAL:
		snprintf(goal_buff, 15, "%02d:%02d", ((data/100)/60)%100, (data/100) % 60);
		string->gx_string_ptr = goal_buff;
		string->gx_string_length = strlen(string->gx_string_ptr);
	default :
		break;
	}	
}

void set_gui_goal_data(GOAL_SETTING_TYPE_T type, int32_t data)
{
	switch (type) {
	case CALORISE_GOAL:
		gui_sport_cfg.goal_calorise = data/100;		
		break;
	case DISTANCE_GOAL:	
		gui_sport_cfg.goal_distance = data;	
		break;
	case TIME_GOAL:
		gui_sport_cfg.goal_time = data/100;		
	default :
		break;
	}	
}

UINT sport_goal_set_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: 
		gx_system_timer_start(window, MAIN_WINDOW_TIMER_ID, 50, 50);
	break;
	case GX_EVENT_HIDE: 
		gx_system_timer_stop(window, MAIN_WINDOW_TIMER_ID);	
	break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}	
	case GX_EVENT_PEN_UP: {
	 	delta_x = (
			 event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - 
			 gx_point_x_first);
	 	if (delta_x >= 50) {
			 jump_sport_page(&outdoor_run_window, &sport_goal_set_window, SCREEN_SLIDE_RIGHT);
			 gx_widget_delete(&sport_goal_set_window);
	 	}
		break;
	}	
	case GX_EVENT_TIMER: 
		if (event_ptr->gx_event_payload.gx_event_timer_id == MAIN_WINDOW_TIMER_ID) {
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

void set_goal_data(GOAL_SETTING_WIDGET_T *row, GX_BOOL pulse_flag)
{
	if (false == pulse_flag) {
		switch (row->goal_type) {
		case CALORISE_GOAL:				
			row->data -= 2500; 
			if(row->data < 0){
				row->data = 0;
			}
		break;
		case DISTANCE_GOAL:
			row->data -= 10;
			if(row->data < 0){
				row->data = 0;
			}
		break;
		case TIME_GOAL:
			row->data -= 500;
			if(row->data < 0){
				row->data = 0;
			}

		break;
		default:		
		break;
		}
		
	} else {
		switch (row->goal_type) {
		case CALORISE_GOAL:				
			row->data += 2500; 	
			if(row->data >= 1000000){
				row->data = 999999;
			}		
		break;
		case DISTANCE_GOAL:
			row->data += 10;	
			if(row->data >= 1000000){
				row->data = 999999;
			}	
		break;
		case TIME_GOAL:
			row->data += 500;
			if(row->data >= 599900){
				row->data = 599900;
			}	
		break;
		default:		
		break;
		}		
	}
	if (row->data > 0) {
		row->child[4].icon = GX_PIXELMAP_ID_APP_SPORT_BTN_L_START;
		row->child[4].nor_color = GX_COLOR_ID_HONBOW_GREEN;
	} else {
		row->child[4].icon = GX_PIXELMAP_ID_APP_SPORT_BTN_L_START_NO;
		row->child[4].nor_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
	}	
}
#define LONG_PRESS_TIMER_ID 2
#define LONG_PRESSED_PLUS  1
#define LONG_PRESSED_MINUS 2

void goal_set_child_widget_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static UINT long_press_time = 0;
	UINT status = 0;
	static int long_press_status = 0;
	GX_STRING string;
	GOAL_SETTING_WIDGET_T *row = CONTAINER_OF(widget, GOAL_SETTING_WIDGET_T, widget);
	switch (event_ptr->gx_event_type) {
	
	case GX_SIGNAL(BUTTON_MINUS_ID, GX_EVENT_CLICKED):
		set_goal_data(row, false);
		set_goal_string(&string, row->goal_type, row->data);
		gx_prompt_text_set_ext(&goal_prompt, &string);
		break;
	case GX_SIGNAL(BUTTON_PLUS_ID, GX_EVENT_CLICKED):
		set_goal_data(row, true);
		set_goal_string(&string, row->goal_type, row->data);
		gx_prompt_text_set_ext(&goal_prompt, &string);
		break;
	case GX_SIGNAL(BUTTON_CONFIRM_ID, GX_EVENT_CLICKED):
		jump_sport_page(&gps_connecting_window, &sport_goal_set_window, SCREEN_SLIDE_LEFT);
		set_gui_goal_data(row->goal_type, row->data);
		gx_widget_delete(&sport_goal_set_window);
		gx_widget_delete(&outdoor_run_window);
		break;

	case GX_EVENT_PEN_DOWN:
		if (widget->gx_widget_style & GX_STYLE_ENABLED) {
			_gx_system_input_capture(widget);
			widget->gx_widget_style |= GX_STYLE_BUTTON_PUSHED;
			gx_system_dirty_mark(widget);
		}		
		if (gx_utility_rectangle_point_detect(&row->child[0].widget.gx_widget_size,
								event_ptr->gx_event_payload.gx_event_pointdata)) {
			gx_system_timer_start(widget, LONG_PRESS_TIMER_ID, 15, 15);
			long_press_status = LONG_PRESSED_MINUS;
		}else if (gx_utility_rectangle_point_detect(&row->child[2].widget.gx_widget_size,
								event_ptr->gx_event_payload.gx_event_pointdata)) {
			gx_system_timer_start(widget, LONG_PRESS_TIMER_ID, 15, 15);	
			long_press_status = LONG_PRESSED_PLUS;					
		}else {
			long_press_status = 0;
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
			gx_system_timer_stop(widget, LONG_PRESS_TIMER_ID);
			long_press_time = 0;
		}
		// need check clicked event happended or not
		status = gx_widget_event_to_parent(widget, event_ptr);
		break;
	case GX_EVENT_INPUT_RELEASE:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release(widget);
			widget->gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
			gx_system_dirty_mark(widget);
		}
		break;
	case GX_EVENT_TIMER: 
		long_press_time++;
		switch (long_press_time) {
			case 1:
				gx_system_timer_start(widget, LONG_PRESS_TIMER_ID, 13, 13);
			break;
			case 5:
				gx_system_timer_start(widget, LONG_PRESS_TIMER_ID, 6, 6);
			break;
			case 18:
				gx_system_timer_start(widget, LONG_PRESS_TIMER_ID, 2, 2);
			break;
			default:
			break;
		}
		// if (long_press_time == 1){
		// 	gx_system_timer_start(widget, LONG_PRESS_TIMER_ID, 13, 13);
		// }  if ((long_press_time == 6)){
		// 	gx_system_timer_start(widget, LONG_PRESS_TIMER_ID, 6, 6);
		// } else if ((long_press_time == 20)){
		// 	gx_system_timer_start(widget, LONG_PRESS_TIMER_ID, 2, 2);
		// }else {;}	
	
		if (event_ptr->gx_event_payload.gx_event_timer_id == LONG_PRESS_TIMER_ID) {
			if (LONG_PRESSED_PLUS == long_press_status)	{
				set_goal_data(row, true);
				set_goal_string(&string, row->goal_type, row->data);
				gx_prompt_text_set_ext(&goal_prompt, &string);
			} else if (LONG_PRESSED_MINUS){
				set_goal_data(row, false);
				set_goal_string(&string, row->goal_type, row->data);
				gx_prompt_text_set_ext(&goal_prompt, &string);
			}else {
				;
			}
		}
		goto end;
		break;
	default:
		gx_widget_event_process(widget, event_ptr);
	break;
	}	
end:;

}

void goal_child_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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
}

void confirm_button_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	COMMON_BUTTON_T *row = CONTAINER_OF(widget, COMMON_BUTTON_T, widget);

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN:
		if (row->nor_color == GX_COLOR_ID_HONBOW_GREEN) {
			if (widget->gx_widget_style & GX_STYLE_ENABLED) {
				_gx_system_input_capture(widget);
				widget->gx_widget_style |= GX_STYLE_BUTTON_PUSHED;
				gx_system_dirty_mark(widget);
			}
		}
		
		status = gx_widget_event_to_parent(widget, event_ptr);
		break;
	case GX_EVENT_PEN_UP:
		if (row->nor_color == GX_COLOR_ID_HONBOW_GREEN) {
			if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
				_gx_system_input_release(widget);
				widget->gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
				gx_system_dirty_mark(widget);
				if (gx_utility_rectangle_point_detect(&widget->gx_widget_size,
								event_ptr->gx_event_payload.gx_event_pointdata)) {
					gx_widget_event_generate(widget, GX_EVENT_CLICKED, 0);
				}
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
}

void goal_minus_button_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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
			GX_RECTANGLE size;
			size.gx_rectangle_left = widget->gx_widget_size.gx_rectangle_left - 10;
			size.gx_rectangle_right = widget->gx_widget_size.gx_rectangle_right + 20;
			size.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_top - 20;
			size.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_bottom + 20;	
			if (gx_utility_rectangle_point_detect(&size,
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
}

void goal_plus_button_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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
			GX_RECTANGLE size;
			size.gx_rectangle_left = widget->gx_widget_size.gx_rectangle_left - 20;
			size.gx_rectangle_right = widget->gx_widget_size.gx_rectangle_right + 10;
			size.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_top - 20;
			size.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_bottom + 20;	
			if (gx_utility_rectangle_point_detect(&size,
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
}

void common_rectangle_button_draw(GX_WIDGET *widget)
{
	COMMON_BUTTON_T *row = CONTAINER_OF(widget, COMMON_BUTTON_T, widget);
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
	gx_context_color_get(row->nor_color, &color);
	
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
	gx_context_pixelmap_get(row->icon, &map);
	if (map) {		
		xpos = widget->gx_widget_size.gx_rectangle_left;
		ypos = widget->gx_widget_size.gx_rectangle_top;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

void common_circle_button_draw(GX_WIDGET *widget)
{
	COMMON_BUTTON_T *row = CONTAINER_OF(widget, COMMON_BUTTON_T, widget);
	INT xpos;
	INT ypos;
	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);
	
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);
	GX_COLOR color;
	gx_context_color_get(row->nor_color, &color);
	if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		uint8_t red = ((color >> 11) >> 4);
		uint8_t green = (((color >> 5) & 0x3f) >> 4);
		uint8_t blue = ((color & 0x1f) >> 4);
		color = (red << 11) + (green << 5) + blue;
	}
	gx_brush_define(&current_context->gx_draw_context_brush, color, color, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
	gx_context_brush_width_set(1);
	INT R = ((fillrect.gx_rectangle_right - fillrect.gx_rectangle_left + 1) >> 1) - 1;
	gx_canvas_circle_draw(
		((fillrect.gx_rectangle_right - fillrect.gx_rectangle_left + 1) >> 1) + fillrect.gx_rectangle_left+1,
		((fillrect.gx_rectangle_bottom - fillrect.gx_rectangle_top + 1) >> 1) + fillrect.gx_rectangle_top+1, R);
	GX_PIXELMAP *map;
	gx_context_pixelmap_get(row->icon, &map);
	xpos = widget->gx_widget_size.gx_rectangle_left;
	ypos = widget->gx_widget_size.gx_rectangle_top;
	gx_canvas_pixelmap_draw(xpos, ypos, map);
}

void app_sport_goal_set_widget_create(GX_WIDGET *parent, SPORT_CHILD_PAGE_T sport_type, GOAL_SETTING_TYPE_T goal_type)
{
	GX_RECTANGLE childsize;
	GX_STRING string;
	GOAL_SETTING_WIDGET_T *row = &goal_setting_widget_momery;
	row->sport_type = sport_type;
	row->data = 0;
	row->goal_type = goal_type;
	gx_utility_rectangle_define(&childsize, 0, 58, 319, 359);
	gx_widget_create(&row->widget, "goal_set_child", parent,
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
		0, &childsize);
	gx_widget_fill_color_set(&row->widget, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&row->widget, &goal_set_child_widget_event);
	
	//child 0
	gx_utility_rectangle_define(&childsize, 12, 118, 92, 198);
	gx_widget_create(&row->child[0].widget, "goal_setchild.widget0", &row->widget,
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
		BUTTON_MINUS_ID, &childsize);
	row->child[0].icon = GX_PIXELMAP_ID_APP_SPORT_BTN_MINUS;
	row->child[0].nor_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
	gx_widget_fill_color_set(&row->child[0].widget, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_draw_set(&row->child[0].widget, common_circle_button_draw);
	gx_widget_event_process_set(&row->child[0].widget, goal_minus_button_event);	

	//child 1
	gx_utility_rectangle_define(&childsize, 94, 135, 226, 181);
	gx_widget_create(&row->child[1].widget, "goal_setchild.widget1", &row->widget,
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
		0, &childsize);
	gx_widget_fill_color_set(&row->child[1].widget, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&row->child[1].widget, goal_child_widget_common_event);

	gx_prompt_create(&goal_prompt, NULL, &row->child[1].widget, 0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE,
						 0, &childsize);
	gx_prompt_text_color_set(&goal_prompt, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
								GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&goal_prompt, GX_FONT_ID_SIZE_46);
	set_goal_string(&string, goal_type, 0);
	gx_prompt_text_set_ext(&goal_prompt, &string);

	//child2
	gx_utility_rectangle_define(&childsize, 228, 118, 308, 198);
	gx_widget_create(&row->child[2].widget, "goal_setchild.widget2", &row->widget,
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
		BUTTON_PLUS_ID, &childsize);
	gx_widget_fill_color_set(&row->child[2].widget, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK);
	row->child[2].icon = GX_PIXELMAP_ID_APP_SPORT_BTN_PLUS;
	row->child[2].nor_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
	gx_widget_draw_set(&row->child[2].widget, common_circle_button_draw);
	gx_widget_event_process_set(&row->child[2].widget, goal_plus_button_event);	

	//child3
	gx_utility_rectangle_define(&childsize, 12, 202, 308, 228);
	gx_widget_create(&row->child[3].widget, "goal_setchild.widget3", &row->widget,
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
		0, &childsize);
	gx_widget_fill_color_set(&row->child[3].widget, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&row->child[3].widget, goal_child_widget_common_event);

	gx_prompt_create(&name_prompt, NULL, &row->child[3].widget, 0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE,
						 0, &childsize);
	gx_prompt_text_color_set(&name_prompt, GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK,
								GX_COLOR_ID_HONBOW_DARK_BLACK);
	gx_prompt_font_set(&name_prompt, GX_FONT_ID_SIZE_26);
	app_goal_units_string_get(goal_type, &string);
	gx_prompt_text_set_ext(&name_prompt, &string);

	//child4
	gx_utility_rectangle_define(&childsize, 12, 283, 308, 348);
	gx_widget_create(&row->child[4].widget, "goal_setchild.widget4", &row->widget,
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
		BUTTON_CONFIRM_ID, &childsize);
	gx_widget_fill_color_set(&row->child[4].widget, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK);
	row->child[4].icon = GX_PIXELMAP_ID_APP_SPORT_BTN_L_START_NO;
	row->child[4].nor_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
	gx_widget_event_process_set(&row->child[4].widget, goal_child_widget_common_event);
	gx_widget_draw_set(&row->child[4].widget, common_rectangle_button_draw);

}

void app_sport_goal_set_window_init(GX_WINDOW *window, SPORT_CHILD_PAGE_T sport_type, GOAL_SETTING_TYPE_T goal_type)
{
	GX_RECTANGLE win_size;
	GX_STRING string;	
	GX_BOOL result;
	gx_widget_created_test(&sport_goal_set_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
		gx_widget_create(&sport_goal_set_window, "sport_goal_set_window", window,
			GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
			0, &win_size);
		gx_widget_fill_color_set(&sport_goal_set_window, 
					GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
					GX_COLOR_ID_HONBOW_BLACK);
		GX_WIDGET *parent = &sport_goal_set_window;

		custom_window_title_create(parent, &icon_title, &setting_prompt, &time_prompt);
		app_sport_list_prompt_get(sport_type, &string);
		gx_prompt_text_set_ext(&setting_prompt, &string);

		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
		time_prompt_buff[7] = 0;
		string.gx_string_ptr = time_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&time_prompt, &string);

		app_sport_goal_set_widget_create(parent, sport_type, goal_type);

		gx_widget_event_process_set(&sport_goal_set_window, 
					sport_goal_set_window_event);	
	}
	
}



