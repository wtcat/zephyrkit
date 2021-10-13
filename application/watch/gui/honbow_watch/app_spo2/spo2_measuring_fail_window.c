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


GX_WIDGET measuring_fail_window;

static GX_PROMPT spo2_prompt;
static GX_PROMPT time_prompt;

static icon_Widget_T retry_widget, exit_widget;
static GX_WIDGET result_multi_line_widget;
static GX_MULTI_LINE_TEXT_INPUT text_input;
static CHAR buffer[100];
extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

#define TIME_FRESH_TIMER_ID 3
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;

#define RETRY_BUTTON  1
#define EXIT_BUTTON 2

UINT app_measuring_fail_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
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
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}	
	case GX_EVENT_PEN_UP: {
	 	delta_x = (event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
	 	if (delta_x >= 50) {
					
	 	}
		break;
	}	
	case GX_SIGNAL(RETRY_BUTTON, GX_EVENT_CLICKED):
		app_spo2_measuring_window_init(NULL);
		jump_spo2_page(&spo2_measuring_window, &measuring_fail_window, SCREEN_SLIDE_RIGHT);
		break;
	case GX_SIGNAL(EXIT_BUTTON, GX_EVENT_CLICKED):
		Gui_close_spo2();
		jump_spo2_page(&spo2_main_page, &measuring_fail_window, SCREEN_SLIDE_RIGHT);
		break;
	// case GX_EVENT_KEY_UP:
	// 	if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
	// 		windows_mgr_page_jump((GX_WINDOW *)&app_spo2_window, NULL, WINDOWS_OP_BACK);		
	// 		// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);			
	// 		return 0;
	// 	}
	// 	break;
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

static const GX_CHAR *get_language_spo2_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_ENGLISH:
		return "SPO2";
	default:
		return "SPO2";
	}
}

void failed_child_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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

void failed_info_text_get(GX_STRING *prompt)
{	
	prompt->gx_string_ptr = "Failed stay still 15s Wear your watch clung to skin and keep still, so that you can measur it";
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

void measuring_failed_rectangle_button_draw(GX_WIDGET *widget)
{
	icon_Widget_T *row = CONTAINER_OF(widget, icon_Widget_T, widget);
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
	gx_context_color_get(row->bg_color, &color);
	
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

static VOID measuring_failed_child_widget_create(GX_WIDGET *widget)
{
	GX_RECTANGLE childsize;
	GX_STRING string;

	gx_utility_rectangle_define(&childsize, 12, 61, 308, 242);
	gx_widget_create(&result_multi_line_widget, NULL, widget,
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_fill_color_set(&result_multi_line_widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&result_multi_line_widget, failed_child_widget_common_event);
	failed_info_text_get(&string);		
	UINT status;	
	status = gx_multi_line_text_input_create(&text_input, "multi_text", 
				&result_multi_line_widget, buffer, 100, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE|GX_STYLE_TEXT_LEFT,
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

	gx_utility_rectangle_define(&childsize, 12, 283, 155, 348);
	gx_widget_create(&retry_widget.widget, "retry_widget", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 1, &childsize);
	gx_widget_fill_color_set(&retry_widget.widget, GX_COLOR_ID_HONBOW_BLACK,
		GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&retry_widget.widget, failed_child_widget_common_event);
	gx_widget_draw_set(&retry_widget.widget, measuring_failed_rectangle_button_draw);
	retry_widget.icon = GX_PIXELMAP_ID_APP_SPO2_BTN_S_REFRESH;	
	retry_widget.bg_color = GX_COLOR_ID_HONBOW_DARK_GRAY;

	gx_utility_rectangle_define(&childsize, 165, 283, 308, 348);
	gx_widget_create(&exit_widget.widget, "exit_widget", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 2, &childsize);
	gx_widget_fill_color_set(&exit_widget.widget, GX_COLOR_ID_HONBOW_BLACK,
		GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&exit_widget.widget, failed_child_widget_common_event);
	gx_widget_draw_set(&exit_widget.widget, measuring_failed_rectangle_button_draw);
	exit_widget.icon = GX_PIXELMAP_ID_APP_SPO2_BTN_S_CANCEL;	
	exit_widget.bg_color = GX_COLOR_ID_HONBOW_RED;
}

void app_spo2_measuring_failed_window_init(GX_WINDOW *window)
{
	GX_RECTANGLE childsize;
	gx_utility_rectangle_define(&childsize, 0, 0, 319, 359);
	gx_widget_create(&measuring_fail_window, "measuring_fail_window", window, 
					GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
					&childsize);
	gx_widget_event_process_set(&measuring_fail_window, app_measuring_fail_window_event);
	GX_WIDGET *parent = &measuring_fail_window;

	custom_window_title_create(parent, NULL, &spo2_prompt, &time_prompt);
	GX_STRING string;
	string.gx_string_ptr = get_language_spo2_title();
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
	measuring_failed_child_widget_create(parent);
	//app_spo2_get_ready_widget_create((GX_WINDOW *)parent);
}

