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
#include "windows_manager.h"

extern uint32_t time_count;
typedef struct {
	uint32_t sport_type;
	uint32_t Duration;
	uint32_t AvgHertRate;
	uint32_t AvgStride;
	uint32_t TotalSteps;
	float Distance;
	uint32_t Calorise;	
	float AvgPace;	
	uint8_t hr_percent[5];
	struct timespec start_ts;
	struct timespec end_ts;
}SPORT_SUMMARY_T;

static SPORT_SUMMARY_T sport_summary = {
	.sport_type = 1,
	.Duration = 123,
	.AvgHertRate = 123,
	.AvgStride = 154,	
	.TotalSteps = 15580,
	.Calorise = 12,
	.Distance = 12.04,
	.AvgPace = 9.4
};

#define WIDGET_ID_0  1
GX_WIDGET sport_summary_window;

extern struct timespec end_sport_ts;
extern struct timespec start_sport_ts;

typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID bg_color;
	GX_RESOURCE_ID icon;
	GX_WIDGET child[3];
} sport_summary_bar_icon_t;

static sport_summary_bar_icon_t summary_icon = {
	.bg_color = GX_COLOR_ID_HONBOW_BLACK,
	.icon = GX_PIXELMAP_ID_IC_SPORT_TODAY,
};

void get_sport_summary_data(void)
{
	sport_summary.AvgHertRate = 123;
	sport_summary.AvgPace = 10.2;
	sport_summary.AvgStride = 154;
	sport_summary.Calorise = 453;
	sport_summary.Distance = 10.6;
	sport_summary.hr_percent[0] = 12;
	sport_summary.hr_percent[1] = 40;
	sport_summary.hr_percent[2] = 60;
	sport_summary.hr_percent[3] = 80;
	sport_summary.hr_percent[4] = 100;
	sport_summary.sport_type = 1;
	sport_summary.TotalSteps = 1556;
	sport_summary.start_ts.tv_sec = start_sport_ts.tv_sec;
	sport_summary.start_ts.tv_nsec = start_sport_ts.tv_nsec;
	sport_summary.end_ts.tv_sec = end_sport_ts.tv_sec;
	sport_summary.end_ts.tv_nsec = end_sport_ts.tv_nsec;
}

void set_sport_duration(uint32_t time)
{
	sport_summary.Duration = time;
}

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

static GX_PROMPT duration_prompt;
static char duration_prompt_buff[15];
static GX_PROMPT distance_prompt;
static char distance_prompt_buff[10];
static GX_PROMPT calorise_burned_prompt;
static char calorise_burned_prompt_buff[10];
static GX_PROMPT avg_hertrate_prompt;
static char avg_hertrate_prompt_buff[10];
static GX_PROMPT avg_pace_prompt;
static char avg_pace_prompt_buff[10];
static GX_PROMPT avg_stride_prompt;
static char avg_stride_prompt_buff[10];
static GX_PROMPT total_steps_prompt;
static char total_steps_prompt_buff[10];

static GX_PROMPT time_prompt;
static GX_PROMPT summary_prompt;

extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];

typedef struct {
	GX_WIDGET widget0;
	GX_WIDGET widget1;
	GX_RESOURCE_ID icon0;
	GX_RESOURCE_ID icon1;
	GX_PROMPT desc;
} Sport_DataChildWindow_TypeDef;

typedef struct SPORT_STARAT_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	Sport_DataChildWindow_TypeDef child;
} SPORT_DATA_LIST_ROW;

SPORT_DATA_LIST_ROW sport_summary_list_momery;

static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 1
extern GX_WINDOW_ROOT *root;


static const GX_CHAR *lauguage_en_sport_summary_element[9] = { "summary",
					"Duration", "Distance", "Calories Burned", 
					"Avg Hert Rate", "Avg Pace","Avg Stride", 
					"Total steps", "Heart Rate Zone"};

static const GX_CHAR *lauguage_en_sport_summary_unit[9] = { "summary",
					"time", "km", "Kcal", 
					"bpm", "/km","/min", 
					"steps", "percent"};

static const GX_CHAR **sport_summary_until_string[] = {
	//[LANGUAGE_CHINESE] = lauguage_ch_setting_element,
	[LANGUAGE_ENGLISH] = lauguage_en_sport_summary_unit
};

static const GX_CHAR **sport_summary_string[] = {
	//[LANGUAGE_CHINESE] = lauguage_ch_setting_element,
	[LANGUAGE_ENGLISH] = lauguage_en_sport_summary_element
};

static const GX_CHAR **get_units_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return sport_summary_until_string[id];
}

static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return sport_summary_string[id];
}

static int8_t max_child = 11;
static void get_summary_title_string(GX_STRING *string)
{
	if(11 == max_child ) {
		string->gx_string_ptr = "Summary";
	}else {
		string->gx_string_ptr = "Sport";
	}	
	string->gx_string_length = strlen(string->gx_string_ptr);
}

void app_sport_summary_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

void sport_summary_units_prompt_get(GX_STRING *prompt, INT index)
{
	const GX_CHAR **units_string = get_units_string();
	prompt->gx_string_ptr = units_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

UINT sport_list_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
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
		delta_x = (event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x <= -50) {
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);
			windows_mgr_page_jump((GX_WINDOW *)&app_sport_window, NULL, WINDOWS_OP_BACK);
			gx_widget_delete(&sport_summary_window);
			gx_widget_delete(&today_main_page); 
	 	}
		if (delta_x >= 50) {
			jump_today_page(&today_main_page, &sport_summary_window, SCREEN_SLIDE_RIGHT);
			gx_widget_delete(&sport_summary_window);
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
			gx_system_dirty_mark(window);
		}		
		return 0;
	default :
		break;
	}
	return gx_widget_event_process(window, event_ptr);	
}

UINT sport_summary_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
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
			// jump_sport_page(&sport_paused_window, &sport_summary_window, SCREEN_SLIDE_LEFT);
			 //printf("sport_summary_window_event -> pause window\n");
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
			gx_system_dirty_mark(window);
		}		
		return 0;
	default :
		break;
	}
	return gx_widget_event_process(window, event_ptr);	
}

UINT sport_summary_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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

#define TITLE_START_POS_X 12
#define TITLE_START_POS_Y 60

void sport_summary_icon_draw_func(GX_WIDGET *widget)
{
	INT xpos;
	INT ypos;	

	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
	GX_PIXELMAP *map;
	gx_context_pixelmap_get(summary_icon.icon, &map);
	xpos = widget->gx_widget_size.gx_rectangle_left;	
	ypos = widget->gx_widget_size.gx_rectangle_top;
	gx_canvas_pixelmap_draw(xpos, ypos, map);
}

static char temp_buff[16];
static char date_buff[11];
static GX_PROMPT end_date_prompt;
static GX_PROMPT start_to_end_prompt;

void sport_summary_title_create(GX_WIDGET *parent, sport_summary_bar_icon_t *icon_tips)
{
		
	bool have_icon = false;
	if (icon_tips != NULL) {		
		GX_STRING string;
		GX_STRING date_string;
		GX_RECTANGLE size;
		gx_utility_rectangle_define(&size, 0, 58, 319, 212);
		gx_widget_create(&icon_tips->widget, NULL, parent, GX_STYLE_ENABLED, GX_ID_NONE, &size);
		gx_widget_fill_color_set(&icon_tips->widget, icon_tips->bg_color, icon_tips->bg_color, icon_tips->bg_color);
		gx_widget_event_process_set(&icon_tips->widget, sport_summary_common_event);
		have_icon = true;

		gx_utility_rectangle_define(&size, 135, 58, 185, 108);
		gx_widget_create(&icon_tips->child[0], NULL, &icon_tips->widget, GX_STYLE_ENABLED, GX_ID_NONE, &size);
		gx_widget_fill_color_set(&icon_tips->child[0], icon_tips->bg_color, icon_tips->bg_color, icon_tips->bg_color);
		gx_widget_draw_set(&icon_tips->child[0], sport_summary_icon_draw_func);
		gx_widget_event_process_set(&icon_tips->widget, sport_summary_common_event);

		gx_utility_rectangle_define(&size, 50, 120, 270, 150);
		gx_widget_create(&icon_tips->child[1], NULL, &icon_tips->widget, GX_STYLE_ENABLED, GX_ID_NONE, &size);
		gx_widget_fill_color_set(&icon_tips->child[1], icon_tips->bg_color, icon_tips->bg_color, icon_tips->bg_color);

		gx_prompt_create(&end_date_prompt, NULL, &icon_tips->child[1], 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
					0, &size);
		gx_prompt_text_color_set(&end_date_prompt, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
									GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&end_date_prompt, GX_FONT_ID_SIZE_30);
		time_t end_ts = sport_summary.end_ts.tv_sec;
		struct tm *tm_end = gmtime(&end_ts);
	
		snprintf(date_buff, 11, "%04d/%02d/%02d", tm_end->tm_year+100, tm_end->tm_mon+1, tm_end->tm_wday);
		date_string.gx_string_ptr = date_buff;
		date_string.gx_string_length = strlen(date_string.gx_string_ptr);
		gx_prompt_text_set_ext(&end_date_prompt, &date_string); 

		gx_utility_rectangle_define(&size, 20, 156, 300, 186);
		gx_widget_create(&icon_tips->child[2], NULL, &icon_tips->widget, GX_STYLE_ENABLED, GX_ID_NONE, &size);
		gx_widget_fill_color_set(&icon_tips->child[2], icon_tips->bg_color, icon_tips->bg_color, icon_tips->bg_color);

		gx_prompt_create(&start_to_end_prompt, NULL, &icon_tips->child[2], 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
					0, &size);
		gx_prompt_text_color_set(&start_to_end_prompt, GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK,
									GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&start_to_end_prompt, GX_FONT_ID_SIZE_30);

		time_t start_ts = sport_summary.start_ts.tv_sec;
		struct tm *tm_start = gmtime(&start_ts);

		snprintf(temp_buff, 16, "%02d:%02d%s-%02d:%02d%s", 
			(tm_start->tm_hour > 12) ? (tm_start->tm_hour - 12) : (tm_start->tm_hour),
			tm_start->tm_min, 
			(tm_start->tm_hour > 12) ? "PM" : "AM",
			(tm_end->tm_hour > 12) ? (tm_end->tm_hour - 12) : (tm_end->tm_hour),
			tm_end->tm_min, 
			(tm_end->tm_hour > 12) ? "PM" : "AM"
			);
		string.gx_string_ptr = temp_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&start_to_end_prompt, &string); 		
	}	
}

static GX_VERTICAL_LIST summary_data_list;

typedef struct {
	GX_WIDGET widget0;
	GX_WIDGET widget1;	
	GX_WIDGET widget2;	
	GX_PROMPT desc0;
	GX_PROMPT desc1;
	UCHAR id;
} SUMMARY_DataIconButton_TypeDef;

typedef struct SUMMARY_DATA_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	SUMMARY_DataIconButton_TypeDef child;
} SUMMARY_DATA_LIST_ROW;

#define SUMMARY_DATA_LIST_VISIBLE_ROWS 2
static SUMMARY_DATA_LIST_ROW summary_data_list_row_memory[11];

typedef struct HertRateZone_Struct{
	GX_WIDGET widget[5];
	GX_RESOURCE_ID bg_color[5];
	GX_PROMPT prompt[5];
	uint8_t percent[5];
}HERTRATEZONE_T;

char prompt_buf[5][5];
HERTRATEZONE_T HertRateZone;
HERTRATEZONE_T HertPromptZone;

static VOID summary_bar_create(GX_WIDGET *parent)
{
	
	GX_RECTANGLE childsize;
	int i = 0;
	for(i = 0; i < 5; i++) {
		gx_utility_rectangle_define(&childsize, 21 + i*62, 1038, 51 + i*62, 1158);
		gx_widget_create(&HertRateZone.widget[i], NULL, parent, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);		
		gx_widget_fill_color_set(&HertRateZone.widget[i], GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								GX_COLOR_ID_HONBOW_BLACK);
		HertRateZone.percent[i] = sport_summary.hr_percent[i];
		HertRateZone.percent[i] = i * 20;		
	}	
	
	HertRateZone.bg_color[0] = GX_COLOR_ID_HONBOW_BLUE;
	HertRateZone.bg_color[1] = GX_COLOR_ID_HONBOW_GREEN;
	HertRateZone.bg_color[2] = GX_COLOR_ID_HONBOW_YELLOW;
	HertRateZone.bg_color[3] = GX_COLOR_ID_HONBOW_ORANGE;
	HertRateZone.bg_color[4] = GX_COLOR_ID_HONBOW_RED;
}

static VOID summary_prompt_create(GX_WIDGET *parent)
{
	GX_STRING string;
	GX_RECTANGLE childsize;
	int i = 0;
	for(i = 0; i < 5; i++) {
		gx_utility_rectangle_define(&childsize, 12 + i*62, 1168, 74 + i*62, 1198);
		gx_widget_create(&HertPromptZone.widget[i], NULL, parent, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);		
		gx_widget_fill_color_set(&HertPromptZone.widget[i], GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								GX_COLOR_ID_HONBOW_BLACK);

		gx_prompt_create(&HertPromptZone.prompt[i], NULL, &HertPromptZone.widget[i], 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&HertPromptZone.prompt[i], GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
									GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&HertPromptZone.prompt[i], GX_FONT_ID_SIZE_26);
	
		snprintf(prompt_buf[i], 5, "%d%%", HertRateZone.percent[i] % 101);
		string.gx_string_ptr = prompt_buf[i];
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&HertPromptZone.prompt[i], &string); 					
	}		
}
	
void heart_bar_draw(GX_WIDGET *widget)
{
	int i = 0;
	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);

	gx_canvas_rectangle_draw(&fillrect);
	
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);
	GX_COLOR color[5];
	GX_COLOR bg_color = 0;
	gx_context_color_get(GX_COLOR_ID_HONBOW_DARK_BLACK, &bg_color);
	int half_line_width = 15;
	for (i = 0;i < 5; i++) {
		gx_context_color_get(HertRateZone.bg_color[i], &color[i]);
		gx_widget_client_get(&HertRateZone.widget[i], 0, &fillrect);
		gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);

		gx_brush_define(&current_context->gx_draw_context_brush, 
					bg_color, bg_color, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
		gx_context_brush_width_set(30);
		gx_canvas_line_draw(
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_right - half_line_width,
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_top + half_line_width,
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_right - half_line_width,
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_bottom - half_line_width);
	
		if (HertRateZone.percent[i] <= 25 ){
			
			gx_brush_define(&current_context->gx_draw_context_brush, 
					color[i], color[i], GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
			gx_canvas_line_draw(
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_right - half_line_width,
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_bottom - half_line_width,
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_right - half_line_width,
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_bottom - half_line_width);			
		} else {
			gx_brush_define(&current_context->gx_draw_context_brush, 
					color[i], color[i], GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
			gx_canvas_line_draw(
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_right - half_line_width,
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_bottom - half_line_width,
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_right - half_line_width,
				HertRateZone.widget[i].gx_widget_size.gx_rectangle_bottom - half_line_width - 
				1.2 * (HertRateZone.percent[i]-25));
		}	
	}
}

static VOID get_summary_info_string(GX_STRING *string)
{
	string->gx_string_ptr = "you can check the exercise results in the \"Today\" section or view the details in the App";
	string->gx_string_length = strlen(string->gx_string_ptr);
}

UINT summary_confirm_button_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {	
	case GX_SIGNAL(2, GX_EVENT_CLICKED):	
		jump_sport_page(&sport_main_page, &sport_summary_window, SCREEN_SLIDE_LEFT);
		clear_sport_widow();
	break;
	default:
		gx_widget_event_process(widget, event_ptr);
		break;
	}
	return 0;
}

static GX_MULTI_LINE_TEXT_INPUT summary_text_input;
static CHAR summary_text_buffer[100];
void gps_confirm_window_draw(GX_WIDGET *widget);

static VOID sport_summary_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	SUMMARY_DATA_LIST_ROW *row = (SUMMARY_DATA_LIST_ROW *)widget;
	GX_BOOL result;
	int i = index;
	gx_widget_created_test(&row->widget, &result);

	if (!result) {			
		GX_STRING string;
		app_sport_summary_list_prompt_get(index, &string);
		if (index == 0) {
			gx_utility_rectangle_define(&childsize, 12, 58, 308, 212);
			gx_widget_create(&row->widget, NULL, list, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_event_process_set(&row->widget, sport_summary_common_event);
			gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			gx_utility_rectangle_define(&childsize, 135, 58, 185, 108);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			gx_widget_draw_set(&row->child.widget0, sport_summary_icon_draw_func);

			gx_utility_rectangle_define(&childsize, 50, 120, 270, 150);
			gx_widget_create(&row->child.widget1, NULL, &row->widget, GX_STYLE_ENABLED, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget1, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			gx_prompt_create(&end_date_prompt, NULL, &row->child.widget1, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
						0, &childsize);
			gx_prompt_text_color_set(&end_date_prompt, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
			gx_prompt_font_set(&end_date_prompt, GX_FONT_ID_SIZE_30);
			time_t end_ts = sport_summary.end_ts.tv_sec;
			struct tm *tm_end_t = gmtime(&end_ts);
			struct tm tm_end;
			memcpy(&tm_end, tm_end_t, sizeof(tm_end));		
			snprintf(date_buff, 11, "%04d/%02d/%02d", tm_end.tm_year+1900, tm_end.tm_mon+1, tm_end.tm_wday);
			GX_STRING date_string;
			date_string.gx_string_ptr = date_buff;
			date_string.gx_string_length = strlen(date_string.gx_string_ptr);
			gx_prompt_text_set_ext(&end_date_prompt, &date_string); 

			gx_utility_rectangle_define(&childsize, 20, 156, 300, 186);
			gx_widget_create(&row->child.widget2, NULL, &row->widget, GX_STYLE_ENABLED, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget2, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			gx_prompt_create(&start_to_end_prompt, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
						0, &childsize);
			gx_prompt_text_color_set(&start_to_end_prompt, GX_COLOR_ID_HONBOW_DARK_BLACK, 
				GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
			gx_prompt_font_set(&start_to_end_prompt, GX_FONT_ID_SIZE_30);
			time_t start_ts = sport_summary.start_ts.tv_sec;
			struct tm *tm_start = gmtime(&start_ts);
			snprintf(temp_buff, 16, "%02d:%02d%s-%02d:%02d%s", 
				(tm_start->tm_hour > 12) ? (tm_start->tm_hour - 12) : (tm_start->tm_hour),
				tm_start->tm_min, 
				(tm_start->tm_hour > 12) ? "PM" : "AM",
				(tm_end.tm_hour > 12) ? (tm_end.tm_hour - 12) : (tm_end.tm_hour),
				tm_end.tm_min, 
				(tm_end.tm_hour > 12) ? "PM" : "AM"	);
			string.gx_string_ptr = temp_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&start_to_end_prompt, &string); 
		}else if (index == 1){
			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 324 + (i-1)*112);
			gx_widget_create(&row->widget, NULL, list, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_event_process_set(&row->widget, sport_summary_common_event);
			gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112 , 308, 242 + (i-1)*112);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_summary_common_event);
			gx_prompt_create(&row->child.desc0, NULL, &row->child.widget0, 0, 
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc0, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&row->child.desc0, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&row->child.desc0, &string);

			gx_utility_rectangle_define(&childsize, 12, 252 + (i-1)*112, 308, 298 + (i-1)*112);
			gx_widget_create(&row->child.widget1, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget1, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);				
			gx_prompt_create(&duration_prompt, NULL, &row->child.widget1, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&duration_prompt, GX_COLOR_ID_HONBOW_GREEN, 
				GX_COLOR_ID_HONBOW_GREEN, GX_COLOR_ID_HONBOW_GREEN);	
			gx_prompt_font_set(&duration_prompt, GX_FONT_ID_SIZE_46);
			snprintf(duration_prompt_buff, 15, "%02d:%02d:%02d", 
								sport_summary.Duration/3600, 
								(sport_summary.Duration/60)%60, 
								sport_summary.Duration%60);
			string.gx_string_ptr = duration_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&duration_prompt, &string);
		}else if (index == 2) {			
			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 324 + (i-1)*112);
			gx_widget_create(&row->widget, NULL, list, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_event_process_set(&row->widget, sport_summary_common_event);
			gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	

			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 242 + (i-1)*112);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_summary_common_event);
			gx_prompt_create(&row->child.desc0, NULL, &row->child.widget0, 0, 
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc0, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&row->child.desc0, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&row->child.desc0, &string);

			gx_utility_rectangle_define(&childsize, 12, 252 + (i-1)*112, 130, 298 + (i-1)*112);
			gx_widget_create(&row->child.widget1, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget1, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			gx_prompt_create(&distance_prompt, NULL, &row->child.widget1, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&distance_prompt, GX_COLOR_ID_HONBOW_GREEN, 
				GX_COLOR_ID_HONBOW_GREEN, GX_COLOR_ID_HONBOW_GREEN);	
			gx_prompt_font_set(&distance_prompt, GX_FONT_ID_SIZE_46);
			snprintf(distance_prompt_buff, 10, "%02d.%02d", (int)sport_summary.Distance, 
				(int)(sport_summary.Distance*100)%100);			
			string.gx_string_ptr = distance_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&distance_prompt, &string);
			gx_utility_rectangle_define(&childsize, 132, 266 + (i-1)*112, 174, 296 + (i-1)*112);
			gx_widget_create(&row->child.widget2, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget2, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			gx_prompt_create(&row->child.desc1, NULL, &row->child.widget2, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc1, GX_COLOR_ID_HONBOW_DARK_BLACK,
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
			gx_prompt_font_set(&row->child.desc1, GX_FONT_ID_SIZE_30);
			sport_summary_units_prompt_get(&string, index);
			gx_prompt_text_set_ext(&row->child.desc1, &string);
		}else if (index == 3) {  //calorise
			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 324 + (i-1)*112);
			gx_widget_create(&row->widget, NULL, list, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_event_process_set(&row->widget, sport_summary_common_event);
			gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	

			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 242 + (i-1)*112);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_summary_common_event);
			gx_prompt_create(&row->child.desc0, NULL, &row->child.widget0, 0, 
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc0, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&row->child.desc0, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&row->child.desc0, &string);
			snprintf(calorise_burned_prompt_buff, 10, "%d", sport_summary.Calorise);
			int len = strlen(calorise_burned_prompt_buff);
			gx_utility_rectangle_define(&childsize, 12, 252 + (i-1)*112, 12+len*27, 298 + (i-1)*112);
			gx_widget_create(&row->child.widget1, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget1, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

			gx_prompt_create(&calorise_burned_prompt, NULL, &row->child.widget1, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&calorise_burned_prompt, GX_COLOR_ID_HONBOW_RED, 
				GX_COLOR_ID_HONBOW_RED,GX_COLOR_ID_HONBOW_RED);	
			gx_prompt_font_set(&calorise_burned_prompt, GX_FONT_ID_SIZE_46);		
			string.gx_string_ptr = calorise_burned_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&calorise_burned_prompt, &string);	
			gx_utility_rectangle_define(&childsize, 12+len*27 + 2, 
				266 + (i-1)*112, 12+len*27 + 2 + 80, 296 + (i-1)*112);
			gx_widget_create(&row->child.widget2, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget2, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

			gx_prompt_create(&row->child.desc1, NULL, &row->child.widget2, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc1, GX_COLOR_ID_HONBOW_DARK_BLACK, 
				GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
			gx_prompt_font_set(&row->child.desc1, GX_FONT_ID_SIZE_30);
			sport_summary_units_prompt_get(&string, index);
			gx_prompt_text_set_ext(&row->child.desc1, &string);
		}else if (index == 4) {  //AVG hr
			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 324 + (i-1)*112);
			gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
				GX_ID_NONE, &childsize);
			gx_widget_event_process_set(&row->widget, sport_summary_common_event);
			gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	

			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 242 + (i-1)*112);
			gx_widget_create(&row->child.widget0, NULL, &row->widget,
				 GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_summary_common_event);
			gx_prompt_create(&row->child.desc0, NULL, &row->child.widget0, 0, 
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc0, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&row->child.desc0, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&row->child.desc0, &string);

			snprintf(avg_hertrate_prompt_buff, 10, "%d", sport_summary.AvgHertRate);
			int len = strlen(avg_hertrate_prompt_buff);
			gx_utility_rectangle_define(&childsize, 12, 252 + (i-1)*112, 12+len*27, 298 + (i-1)*112);
			gx_widget_create(&row->child.widget1, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget1, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

			gx_prompt_create(&avg_hertrate_prompt, NULL, &row->child.widget1, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&avg_hertrate_prompt, GX_COLOR_ID_HONBOW_RED, 
				GX_COLOR_ID_HONBOW_RED, GX_COLOR_ID_HONBOW_RED);	
			gx_prompt_font_set(&avg_hertrate_prompt, GX_FONT_ID_SIZE_46);
		
			string.gx_string_ptr = avg_hertrate_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&avg_hertrate_prompt, &string);	

			gx_utility_rectangle_define(&childsize, 12+len*27 + 2, 266 + (i-1)*112 , 12+len*27 + 2 + 61, 296 + (i-1)*112);
			gx_widget_create(&row->child.widget2, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget2, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

			gx_prompt_create(&row->child.desc1, NULL, &row->child.widget2, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc1, GX_COLOR_ID_HONBOW_DARK_BLACK, 
				GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
			gx_prompt_font_set(&row->child.desc1, GX_FONT_ID_SIZE_30);
			sport_summary_units_prompt_get(&string, index);
			gx_prompt_text_set_ext(&row->child.desc1, &string);
		}else if (index == 5) {  //AVG PACE
			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 324 + (i-1)*112);
			gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_event_process_set(&row->widget, sport_summary_common_event);
			gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	

			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 242 + (i-1)*112);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_summary_common_event);
			gx_prompt_create(&row->child.desc0, NULL, &row->child.widget0, 0, 
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc0, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE,GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&row->child.desc0, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&row->child.desc0, &string);
			snprintf(avg_pace_prompt_buff, 10, "%d\'%02d\"", (int)sport_summary.AvgPace, 
				(int)(sport_summary.AvgPace *100)%100);
			int len = strlen(avg_pace_prompt_buff);
			gx_utility_rectangle_define(&childsize, 12, 252 + (i-1)*112, 12+40+(len-3)*28, 298 + (i-1)*112);
			gx_widget_create(&row->child.widget1, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget1, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			gx_prompt_create(&avg_pace_prompt, NULL, &row->child.widget1, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&avg_pace_prompt, GX_COLOR_ID_HONBOW_BLUE, 
				GX_COLOR_ID_HONBOW_BLUE, GX_COLOR_ID_HONBOW_BLUE);	
			gx_prompt_font_set(&avg_pace_prompt, GX_FONT_ID_SIZE_46);
		
			string.gx_string_ptr = avg_pace_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&avg_pace_prompt, &string);	
			gx_utility_rectangle_define(&childsize, 12+40+(len-3)*28 + 2, 266 + (i-1)*112, 
				12+40+(len-3)*28 + 2 + 55, 296 + (i-1)*112);
			gx_widget_create(&row->child.widget2, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget2, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

			gx_prompt_create(&row->child.desc1, NULL, &row->child.widget2, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc1, GX_COLOR_ID_HONBOW_DARK_BLACK, 
				GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
			gx_prompt_font_set(&row->child.desc1, GX_FONT_ID_SIZE_30);
			sport_summary_units_prompt_get(&string, index);
			gx_prompt_text_set_ext(&row->child.desc1, &string);
		}else if (index == 6) {  //AVG stride
			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 324 + (i-1)*112);
			gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_event_process_set(&row->widget, sport_summary_common_event);
			gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	

			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 242 + (i-1)*112);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_summary_common_event);
			gx_prompt_create(&row->child.desc0, NULL, &row->child.widget0, 0, 
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc0, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&row->child.desc0, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&row->child.desc0, &string);

			snprintf(avg_stride_prompt_buff, 10, "%d", sport_summary.AvgStride);
			int len = strlen(avg_stride_prompt_buff);
			gx_utility_rectangle_define(&childsize, 12, 252 + (i-1)*112, 12+(len)*27, 298 + (i-1)*112);
			gx_widget_create(&row->child.widget1, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget1, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

			gx_prompt_create(&avg_stride_prompt, NULL, &row->child.widget1, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&avg_stride_prompt, GX_COLOR_ID_HONBOW_BLUE, 
				GX_COLOR_ID_HONBOW_BLUE, GX_COLOR_ID_HONBOW_BLUE);	
			gx_prompt_font_set(&avg_stride_prompt, GX_FONT_ID_SIZE_46);
		
			string.gx_string_ptr = avg_stride_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&avg_stride_prompt, &string);	

			gx_utility_rectangle_define(&childsize, 12+(len)*27 + 2, 266 + (i-1)*112 , 
				12+(len)*27 + 2 + 64, 296 + (i-1)*112);
			gx_widget_create(&row->child.widget2, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget2, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

			gx_prompt_create(&row->child.desc1, NULL, &row->child.widget2, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc1, GX_COLOR_ID_HONBOW_DARK_BLACK, 
				GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
			gx_prompt_font_set(&row->child.desc1, GX_FONT_ID_SIZE_30);
			sport_summary_units_prompt_get(&string, index);
			gx_prompt_text_set_ext(&row->child.desc1, &string);
		}else if (index == 7) {  //steps
			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 324 + (i-1)*112);
			gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE,
				 GX_ID_NONE, &childsize);
			gx_widget_event_process_set(&row->widget, sport_summary_common_event);
			gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	

			gx_utility_rectangle_define(&childsize, 12, 212 + (i-1)*112, 308, 242 + (i-1)*112);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_summary_common_event);
			gx_prompt_create(&row->child.desc0, NULL, &row->child.widget0, 0, 
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc0, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&row->child.desc0, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&row->child.desc0, &string);
			
			gx_utility_rectangle_define(&childsize, 12, 252 + (i-1)*112, 308, 298 + (i-1)*112);
			gx_widget_create(&row->child.widget1, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget1, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
			gx_prompt_create(&total_steps_prompt, NULL, &row->child.widget1, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&total_steps_prompt, GX_COLOR_ID_HONBOW_GREEN, 
				GX_COLOR_ID_HONBOW_GREEN, GX_COLOR_ID_HONBOW_GREEN);	
			gx_prompt_font_set(&total_steps_prompt, GX_FONT_ID_SIZE_46);
			snprintf(total_steps_prompt_buff, 10, "%d", sport_summary.TotalSteps);
			string.gx_string_ptr = total_steps_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&total_steps_prompt, &string);			
		}else if (index == 8) {
			gx_utility_rectangle_define(&childsize, 12, 996, 308, 1220);
			gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
				GX_ID_NONE, &childsize);
			gx_widget_event_process_set(&row->widget, sport_summary_common_event);
			gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);	

			gx_utility_rectangle_define(&childsize, 12, 996, 308, 1026);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_summary_common_event);

			gx_prompt_create(&row->child.desc0, NULL, &row->child.widget0, 0, 
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc0, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&row->child.desc0, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&row->child.desc0, &string);

			gx_utility_rectangle_define(&childsize, 12, 1038, 308,1168);
			gx_widget_create(&row->child.widget1, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget1, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			summary_bar_create(&row->child.widget1);
			gx_widget_draw_set(&row->child.widget1, heart_bar_draw);

			gx_utility_rectangle_define(&childsize, 12, 1168, 308,1220);
			gx_widget_create(&row->child.widget2, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget2, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);			
			summary_prompt_create(&row->child.widget2);				
		}else if (index == 9) {
			gx_utility_rectangle_define(&childsize, 12, 1220, 308, 1365);
			gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
				GX_ID_NONE, &childsize);
			gx_widget_event_process_set(&row->widget, sport_summary_common_event);
			gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	

			gx_utility_rectangle_define(&childsize, 12, 1220, 308, 1365);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

			gx_widget_status_remove(&row->child.widget0, GX_STATUS_ACCEPTS_FOCUS);
			get_summary_info_string(&string);		
			UINT status;	
			status = gx_multi_line_text_input_create(&summary_text_input, "multi_text", 
						&row->child.widget0, summary_text_buffer, 100, 
						GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE|GX_STYLE_TEXT_LEFT,
						GX_ID_NONE, &childsize);
			gx_widget_status_remove(&summary_text_input, GX_STATUS_ACCEPTS_FOCUS);
			if (status == GX_SUCCESS) {
				gx_multi_line_text_view_font_set(
						(GX_MULTI_LINE_TEXT_VIEW *)&summary_text_input, GX_FONT_ID_SIZE_26);
				gx_multi_line_text_input_fill_color_set(
					&summary_text_input, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
					GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
				gx_multi_line_text_input_text_color_set(
					&summary_text_input, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, 
					GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
				gx_multi_line_text_view_whitespace_set(
					(GX_MULTI_LINE_TEXT_VIEW *)&summary_text_input, 0);
				gx_multi_line_text_view_line_space_set(
					(GX_MULTI_LINE_TEXT_VIEW *)&summary_text_input, 0);
				gx_multi_line_text_input_text_set_ext(
					&summary_text_input, &string);
			}
		} else if (index == 10){
			gx_utility_rectangle_define(&childsize, 12, 1365, 308, 1455);
			gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
				GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->widget, summary_confirm_button_event);

			gx_utility_rectangle_define(&childsize, 12, 1365, 308, 1441);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 2, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_summary_common_event);
			gx_widget_draw_set(&row->child.widget0, gps_confirm_window_draw);
		} else {
			;
		}
		row->child.id = index;	
	}
}

static VOID sport_summary_list_child_creat(GX_VERTICAL_LIST *list)
{
	INT count;
	
	for (count = 0; count < max_child; count++) {
		sport_summary_list_row_create(list, 
			(GX_WIDGET *)&summary_data_list_row_memory[count], count);
	}
}

void sport_summary_list_create(GX_WIDGET *widget)
{
	GX_RECTANGLE child_size;
	gx_utility_rectangle_define(&child_size, 12, 58, 308, 359);
	gx_vertical_list_create(&summary_data_list, NULL, widget, max_child, NULL,
		GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child_size);
	gx_widget_fill_color_set(&summary_data_list, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
	sport_summary_list_child_creat(&summary_data_list);
}

Custom_title_icon_t list_icon = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_BTN_BACK,
};

void app_sport_summary_window_init(GX_WINDOW *window, GX_BOOL button_flag)
{
	GX_RECTANGLE win_size;
	GX_STRING string;	
	GX_BOOL result;
	gx_widget_created_test(&sport_summary_window, &result);

	if (!result){
		get_sport_summary_data();
		gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
		gx_widget_create(&sport_summary_window, "sport_summary_window", window, 
			GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
			0, &win_size);
		gx_widget_fill_color_set(&sport_summary_window, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
		GX_WIDGET *parent = &sport_summary_window;

		if (button_flag){
			max_child = 11;
			gx_widget_event_process_set(&sport_summary_window, sport_summary_window_event);
			custom_window_title_create(parent, NULL, &summary_prompt, &time_prompt);
		} else {
			max_child = 10;
			gx_widget_event_process_set(&sport_summary_window, sport_list_window_event);
			custom_window_title_create(parent, &list_icon, &summary_prompt, &time_prompt);
		}		
		get_summary_title_string(&string);
		gx_prompt_text_set_ext(&summary_prompt, &string);
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
		time_prompt_buff[7] = 0;
		string.gx_string_ptr = time_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&time_prompt, &string);
		sport_summary_list_create(parent);
	}
}

