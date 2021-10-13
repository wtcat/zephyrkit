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

#define START_ICON  1
#define TIME_ICON   2
#define FREQ_ICON	3

extern APP_BREATH_WINDOW_CONTROL_BLOCK app_breath_window;
GX_WIDGET breath_main_page;
static GX_PROMPT title_prompt;
static GX_PROMPT time_prompt;
static char time_prompt_buff[8];
static breath_icon_Widget_T time_widget, freq_widget, start_icon;
static GX_PROMPT breath_time_prompt;
static char breath_time_prompt_buff[5];
static GX_PROMPT freq_prompt;
static char freq_prompt_buff[5];

extern GX_WINDOW_ROOT *root;

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
#define TIME_FRESH_TIMER_ID 2
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;

BREATH_SET_T breath_data = {
	.breath_time = 1,
	.breath_freq = 9,
};

UINT app_breath_main_page_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	GX_STRING string;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {		
		gx_system_timer_start(window, TIME_FRESH_TIMER_ID, 50, 50);	
		snprintf(freq_prompt_buff, 5, "%ds", breath_data.breath_freq%11);	
		string.gx_string_ptr = freq_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&freq_prompt, &string);
		snprintf(breath_time_prompt_buff, 5, "%dmin", breath_data.breath_time%6);	
		string.gx_string_ptr = breath_time_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&breath_time_prompt, &string);
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
	case GX_SIGNAL(TIME_ICON, GX_EVENT_CLICKED):
		jump_breath_page(&set_breath_time_window, &breath_main_page, SCREEN_SLIDE_LEFT);
	break;
	case GX_SIGNAL(FREQ_ICON, GX_EVENT_CLICKED):
		jump_breath_page(&set_breath_freq_window, &breath_main_page, SCREEN_SLIDE_LEFT);
	break;
	case GX_SIGNAL(START_ICON, GX_EVENT_CLICKED):
		jump_breath_page(&breath_attention_window, &breath_main_page, SCREEN_SLIDE_LEFT);
	break;
	case GX_EVENT_KEY_UP:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			windows_mgr_page_jump((GX_WINDOW *)&app_spo2_window, NULL, WINDOWS_OP_BACK);		
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);			
			return 0;
		}
		break;
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

static const GX_CHAR *get_language_breath_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_ENGLISH:
		return "Breath";
	default:
		return "Breath";
	}
}

UINT breath_child_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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
		status = gx_widget_event_process(widget, event_ptr);
	break;
	}	
	return status;
}

GX_WIDGET *app_breath_menu_screen_list[] = {
	&breath_main_page,
	GX_NULL,
	GX_NULL,
};

//SCREEN_SLIDE_LEFT
void jump_breath_page(GX_WIDGET *next, GX_WIDGET *prev, uint8_t dir)
{
	app_breath_menu_screen_list[0] = prev;
	app_breath_menu_screen_list[1] = next;
	custom_animation_start(app_breath_menu_screen_list, (GX_WIDGET *)&app_breath_window, dir);
}

void breath_confirm_rectangle_button_draw(GX_WIDGET *widget)
{
	breath_icon_Widget_T *row = CONTAINER_OF(widget, breath_icon_Widget_T, widget);
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

void breath_param_set_rectangle_button_draw(GX_WIDGET *widget)
{
	breath_icon_Widget_T *row = CONTAINER_OF(widget, breath_icon_Widget_T, widget);
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
		xpos = widget->gx_widget_size.gx_rectangle_left + 105;
		ypos = widget->gx_widget_size.gx_rectangle_top + 21;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

void breath_simple_icon_widget_draw(GX_WIDGET *widget)
{
	breath_icon_Widget_T *row = CONTAINER_OF(widget, breath_icon_Widget_T, widget);
	GX_RECTANGLE fillrect;
	INT xpos;
	INT ypos;
	if (row->icon) {
		gx_widget_client_get(widget, 0, &fillrect);
		gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
		gx_canvas_rectangle_draw(&fillrect);

		GX_PIXELMAP *map = NULL;
		gx_context_pixelmap_get(row->icon, &map);
		if (map) {		
			xpos = widget->gx_widget_size.gx_rectangle_left;
			ypos = widget->gx_widget_size.gx_rectangle_top;
			gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
		}
	}	
}

void breath_circle_icon_widget_draw(GX_WIDGET *widget)
{
	breath_icon_Widget_T *row = CONTAINER_OF(widget, breath_icon_Widget_T, widget);
	GX_RECTANGLE fillrect;
	INT xpos;
	INT ypos;
	if (row->icon) {
		gx_widget_client_get(widget, 0, &fillrect);
		gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
		gx_canvas_rectangle_draw(&fillrect);

		GX_COLOR color;
		gx_context_color_get(row->bg_color, &color);
		if (row->widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
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
			fillrect.gx_rectangle_top+1, 74);
		GX_PIXELMAP *map = NULL;
		gx_context_pixelmap_get(row->icon, &map);
		if (map) {
			xpos = fillrect.gx_rectangle_left;
			ypos = fillrect.gx_rectangle_top; 		
			gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
		}	
		gx_widget_children_draw(widget);		
	}	
}

static VOID breath_child_widget_create(GX_WIDGET *widget)
{
	GX_RECTANGLE childsize;
	GX_STRING string;	

	gx_utility_rectangle_define(&childsize, 85, 86, 235, 236);
	gx_widget_create(&start_icon.widget, "start_icon", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, START_ICON, &childsize);
	gx_widget_fill_color_set(&start_icon.widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	start_icon.icon = GX_PIXELMAP_ID_APP_BREATH_START;
	start_icon.bg_color = GX_COLOR_ID_HONBOW_BLUE;
	gx_widget_event_process_set(&start_icon.widget, breath_child_widget_common_event);
	gx_widget_draw_set(&start_icon.widget, breath_circle_icon_widget_draw);
	

	gx_utility_rectangle_define(&childsize, 12, 283, 155, 348);
	gx_widget_create(&time_widget, NULL, widget,
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, TIME_ICON, &childsize);
	gx_widget_fill_color_set(&time_widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&time_widget, breath_child_widget_common_event);
	time_widget.icon = GX_PIXELMAP_ID_APP_BREATH_BTN_NEXT_ARROW;
	time_widget.bg_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
	gx_widget_draw_set(&time_widget, breath_param_set_rectangle_button_draw);

	gx_utility_rectangle_define(&childsize, 26, 301, 110, 331);
	gx_prompt_create(&breath_time_prompt, NULL, &time_widget, 0,	GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 0, &childsize);
	gx_prompt_text_color_set(&breath_time_prompt, GX_COLOR_ID_HONBOW_WHITE,GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&breath_time_prompt, GX_FONT_ID_SIZE_30);	
	snprintf(breath_time_prompt_buff, 5, "%dmin", breath_data.breath_time%6);	
	string.gx_string_ptr = breath_time_prompt_buff;
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&breath_time_prompt, &string);

	gx_utility_rectangle_define(&childsize, 165, 283, 308, 348);
	gx_widget_create(&freq_widget, NULL, widget,
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, FREQ_ICON, &childsize);
	gx_widget_fill_color_set(&freq_widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&freq_widget, breath_child_widget_common_event);
	freq_widget.icon = GX_PIXELMAP_ID_APP_BREATH_BTN_NEXT_ARROW;
	freq_widget.bg_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
	gx_widget_draw_set(&freq_widget, breath_param_set_rectangle_button_draw);

	gx_utility_rectangle_define(&childsize, 179, 301, 260, 331);
	gx_prompt_create(&freq_prompt, NULL, &freq_widget, 0,	GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 0, &childsize);
	gx_prompt_text_color_set(&freq_prompt, GX_COLOR_ID_HONBOW_WHITE,GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&freq_prompt, GX_FONT_ID_SIZE_30);	
	snprintf(freq_prompt_buff, 5, "%ds", breath_data.breath_freq%11);	
	string.gx_string_ptr = freq_prompt_buff;
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&freq_prompt, &string);
}

void app_breath_window_init(GX_WINDOW *window)
{
	GX_BOOL result;
	gx_widget_created_test(&breath_main_page, &result);
	if (!result) {
		gx_widget_create(&breath_main_page, "breath_main_page", window, 
					GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
					&window->gx_widget_size);
		GX_WIDGET *parent = &breath_main_page;
		custom_window_title_create(parent, NULL, &title_prompt, &time_prompt);
		GX_STRING string;
		string.gx_string_ptr = get_language_breath_title();
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

		breath_child_widget_create(parent);	
		gx_widget_event_process_set(&breath_main_page, app_breath_main_page_event);
	}
	app_breath_attention_window_init(NULL);
	// app_breath_inhale_window_init(NULL);
	app_set_breath_time_window_init(NULL);
	app_set_breath_freq_window_init(NULL);	
}

GUI_APP_DEFINE_WITH_INIT(today, APP_ID_05_ALARM_BREATHE, (GX_WIDGET *)&app_breath_window,
	GX_PIXELMAP_ID_APP_LIST_ICON_BREATHE_90_90, GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY,
	app_breath_window_init);