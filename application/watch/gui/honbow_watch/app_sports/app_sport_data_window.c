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

static uint16_t s_hr;
#define WIDGET_ID_0  1
GX_WIDGET sport_data_window;

typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID bg_color;
	UINT segment;
	UINT hr;
	GX_WIDGET child[5];
} Hr_bar_icon_t;

static Hr_bar_icon_t hr_bar = {
	.bg_color = GX_COLOR_ID_HONBOW_BLACK,
	.segment = 5,
};

static Custom_title_icon_t icon_title = {
	.bg_color = GX_COLOR_ID_HONBOW_BLACK,
	.icon = GX_PIXELMAP_ID_APP_SPORT_IC_HEART_SPORT,
};

static Custom_title_icon_t icon_heart = {
	.bg_color = GX_COLOR_ID_HONBOW_BLACK,
	.icon = GX_PIXELMAP_ID_APP_SPORT_IC_HEART_SPORT,
};

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

static GX_PROMPT duration_prompt;
static char duration_prompt_buff[14];
static GX_PROMPT distance_prompt;
static char distance_prompt_buff[10];
static GX_PROMPT avg_pace_prompt;
static char avg_pace_prompt_buff[16];
static GX_PROMPT time_prompt;
static GX_PROMPT heart_prompt;
extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];
static char heart_prompt_buff[8];

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

SPORT_DATA_LIST_ROW sport_data_list_momery;

static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 1
extern GX_WINDOW_ROOT *root;

static const GX_CHAR *lauguage_en_sport_data_element[3] = {"Duration", "Distance", "Avg Pace"};

static const GX_CHAR **sport_data_string[] = {
	//[LANGUAGE_CHINESE] = lauguage_ch_setting_element,
	[LANGUAGE_ENGLISH] = lauguage_en_sport_data_element};
static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return sport_data_string[id];
}

void app_sport_data_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

void km_prompt_get(GX_STRING *prompt, GX_BOOL pace)
{	
	if (pace) {
		prompt->gx_string_ptr = "/km";
	} else {
		prompt->gx_string_ptr = "km";
	}	
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

uint32_t sport_time_count = 0;

float get_distance_data(uint32_t count)
{
	float data = count/100.0;
	return data;
}

uint16_t get_hr_realtime_data(int32_t count)
{
	int data = rand();
	return  (data%220 + 30);
}

bool get_gps_connet_status(void);
static RunDataStruct_t Run_data;
extern GX_RESOURCE_ID count_down_icon;
UINT sport_data_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
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
			jump_sport_page(&sport_paused_window, &sport_data_window, SCREEN_SLIDE_LEFT);
			gui_sport_cfg.status = SPORT_PAUSE;
			//gui_set_sport_param(&gui_sport_cfg);
	 	}
		break;
	}	
	case GX_EVENT_TIMER: 
		if (event_ptr->gx_event_payload.gx_event_timer_id == TIME_FRESH_TIMER_ID) {
			sport_time_count++;
			struct timespec ts;

			if (get_gps_connet_status()) {
				icon_title.icon = GX_PIXELMAP_ID_APP_SPORT_IC_GPS_ON;
			}else {
				icon_title.icon = GX_PIXELMAP_ID_APP_SPORT_IC_GPS_DOWN;
			}
			GX_STRING string;
			clock_gettime(CLOCK_REALTIME, &ts);
			time_t now = ts.tv_sec;
			struct tm *tm_now = gmtime(&now);
			snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
			time_prompt_buff[7] = 0;
			string.gx_string_ptr = time_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&time_prompt, &string);
			s_hr = get_hr_realtime_data(sport_time_count);	

			if (s_hr)	{
				snprintf(heart_prompt_buff,  8, "%d", s_hr);
			} else {
				snprintf(heart_prompt_buff,  8, "%s", "--");
			}
			string.gx_string_ptr = heart_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&heart_prompt, &string);	
			
			snprintf(duration_prompt_buff, 14, "%02d:%02d:%02d", sport_time_count/3600, 
				(sport_time_count/60)%60, sport_time_count%60);
			string.gx_string_ptr = duration_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&duration_prompt, &string);
			
			gui_get_sport_running_data(&Run_data);
			// float data = get_distance_data(sport_time_count);
			float data = Run_data.distance/1000.0;
			snprintf(distance_prompt_buff, 10, "%02d.%02d", (int)data, (int)(data*100)%100);
			string.gx_string_ptr = distance_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&distance_prompt, &string);

			data = Run_data.avr_pace;
			snprintf(avg_pace_prompt_buff, 16, "%02d\'%02d\"", (int)data, (int)(data*100)%100);
			string.gx_string_ptr = avg_pace_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&avg_pace_prompt, &string);

			gx_system_dirty_mark(window);
		}		
		return 0;
	default :
		break;
	}
	return gx_widget_event_process(window, event_ptr);	
}

UINT sport_data_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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

void heartrate_icon_draw_func(GX_WIDGET *widget)
{
	INT xpos;
	INT ypos;

	Custom_title_icon_t *icon_tips = CONTAINER_OF(widget, Custom_title_icon_t, widget);
	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(icon_tips->bg_color, icon_tips->bg_color, 
		GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
	GX_PIXELMAP *map;
	gx_context_pixelmap_get(icon_tips->icon, &map);
	xpos = widget->gx_widget_size.gx_rectangle_left;	
	ypos = widget->gx_widget_size.gx_rectangle_top;
	gx_canvas_pixelmap_draw(xpos, ypos, map);
}

#define HR_BAR_START_X  12
#define HR_BAR_START_Y  110

void heartrate_bar_draw_func(GX_WIDGET *widget)
{
	INT i = 0;
	GX_RECTANGLE fillrect;
	Hr_bar_icon_t *bar_tips = CONTAINER_OF(widget, Hr_bar_icon_t, widget);
	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
		GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
	gx_canvas_rectangle_draw(&fillrect);
	GX_COLOR color[bar_tips->segment];
	bar_tips->hr = s_hr;

	for(i = 0; i < bar_tips->segment; i++){
		gx_context_color_get(GX_COLOR_ID_HONBOW_DARK_BLACK, &color[i]);
	}

	if ((bar_tips->hr > 30) & (bar_tips->hr <= 120)) {
		gx_context_color_get(GX_COLOR_ID_HONBOW_BLUE, &color[0]); 
	} else if ((bar_tips->hr > 120) & (bar_tips->hr <= 150)) {
		gx_context_color_get(GX_COLOR_ID_HONBOW_BLUE, &color[0]); 
		gx_context_color_get(GX_COLOR_ID_HONBOW_GREEN, &color[1]); 
	} else if ((bar_tips->hr > 150) & (bar_tips->hr <= 180)) {
		gx_context_color_get(GX_COLOR_ID_HONBOW_BLUE, &color[0]); 
		gx_context_color_get(GX_COLOR_ID_HONBOW_GREEN, &color[1]);
		gx_context_color_get(GX_COLOR_ID_HONBOW_YELLOW, &color[2]);
	} else if ((bar_tips->hr > 180) & (bar_tips->hr <= 200)) {
		gx_context_color_get(GX_COLOR_ID_HONBOW_BLUE, &color[0]); 
		gx_context_color_get(GX_COLOR_ID_HONBOW_GREEN, &color[1]);
		gx_context_color_get(GX_COLOR_ID_HONBOW_YELLOW, &color[2]);
		gx_context_color_get(GX_COLOR_ID_HONBOW_ORANGE , &color[3]);
	} else if (bar_tips->hr > 200) {
		gx_context_color_get(GX_COLOR_ID_HONBOW_BLUE, &color[0]); 
		gx_context_color_get(GX_COLOR_ID_HONBOW_GREEN, &color[1]);
		gx_context_color_get(GX_COLOR_ID_HONBOW_YELLOW, &color[2]);
		gx_context_color_get(GX_COLOR_ID_HONBOW_ORANGE , &color[3]);
		gx_context_color_get(GX_COLOR_ID_HONBOW_RED , &color[4]);
	} else {
		for(i = 0; i < bar_tips->segment; i++) {
			gx_context_color_get(GX_COLOR_ID_HONBOW_DARK_BLACK, &color[i]);
		}
	}			
	GX_COLOR black;
	gx_context_color_get(GX_COLOR_ID_HONBOW_BLACK, &black);
	uint16_t half_line_width = 6;
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);

	for (i = 0; i < bar_tips->segment; i++) {
		gx_widget_client_get(&bar_tips->child[i], 0, &fillrect);
		gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);

		if (i == 0) {
			gx_brush_define(&current_context->gx_draw_context_brush, 
					color[i], color[i], GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
			gx_context_brush_width_set(12);
			gx_canvas_line_draw(
				bar_tips->child[i].gx_widget_size.gx_rectangle_right,
				bar_tips->child[i].gx_widget_size.gx_rectangle_top + half_line_width,
				bar_tips->child[i].gx_widget_size.gx_rectangle_left + half_line_width,
				bar_tips->child[i].gx_widget_size.gx_rectangle_top + half_line_width);

			gx_brush_define(&current_context->gx_draw_context_brush, 
					black, black, GX_BRUSH_ALIAS);
			gx_context_brush_width_set(1);
			gx_canvas_line_draw(
				bar_tips->child[i].gx_widget_size.gx_rectangle_right+1,
				bar_tips->child[i].gx_widget_size.gx_rectangle_top,
				bar_tips->child[i].gx_widget_size.gx_rectangle_right+1,
				bar_tips->child[i].gx_widget_size.gx_rectangle_bottom );

		} else if (i == 3) {
			i = 4;
			gx_brush_define(&current_context->gx_draw_context_brush, 
					color[i], color[i], GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
			gx_context_brush_width_set(12);
			gx_canvas_line_draw(
				bar_tips->child[i].gx_widget_size.gx_rectangle_left,
				bar_tips->child[i].gx_widget_size.gx_rectangle_top + half_line_width,
				bar_tips->child[i].gx_widget_size.gx_rectangle_right - 8,
				bar_tips->child[i].gx_widget_size.gx_rectangle_top + half_line_width);

			gx_brush_define(&current_context->gx_draw_context_brush, 
					black, black, GX_BRUSH_ALIAS);
			gx_context_brush_width_set(1);
			gx_canvas_line_draw(
				bar_tips->child[i].gx_widget_size.gx_rectangle_left,
				bar_tips->child[i].gx_widget_size.gx_rectangle_top,
				bar_tips->child[i].gx_widget_size.gx_rectangle_left,
				bar_tips->child[i].gx_widget_size.gx_rectangle_bottom );	
			i = 3;		
		} else if(i == 4) {
			i = 3;
			gx_brush_define(&current_context->gx_draw_context_brush, 
					color[i], color[i], GX_BRUSH_ALIAS);
			gx_context_brush_width_set(12);
			gx_canvas_line_draw(
				bar_tips->child[i].gx_widget_size.gx_rectangle_left,
				bar_tips->child[i].gx_widget_size.gx_rectangle_top + half_line_width,
				bar_tips->child[i].gx_widget_size.gx_rectangle_right,
				bar_tips->child[i].gx_widget_size.gx_rectangle_top + half_line_width);
			i = 4;
		} else {
			gx_brush_define(&current_context->gx_draw_context_brush, 
					color[i], color[i], GX_BRUSH_ALIAS);
			gx_context_brush_width_set(12);
			gx_canvas_line_draw(
				bar_tips->child[i].gx_widget_size.gx_rectangle_left,
				bar_tips->child[i].gx_widget_size.gx_rectangle_top + half_line_width,
				bar_tips->child[i].gx_widget_size.gx_rectangle_right,
				bar_tips->child[i].gx_widget_size.gx_rectangle_top + half_line_width);
		}
	}
}

void heartrate_title_create(GX_WIDGET *parent, Custom_title_icon_t *icon_tips, GX_PROMPT *data_tips)
{
	bool have_icon = false;
	
	if (icon_tips != NULL) {
		GX_RECTANGLE size;
		gx_utility_rectangle_define(&size, 91, 63, 131, 103);
		gx_widget_create(&icon_tips->widget, NULL, parent, GX_STYLE_ENABLED, GX_ID_NONE, &size);
		gx_widget_draw_set(&icon_tips->widget, heartrate_icon_draw_func);
		gx_widget_event_process_set(&icon_tips->widget, sport_data_common_event);
		have_icon = true;
	}

	if (data_tips != NULL) {	
		GX_RECTANGLE childsize;
		gx_utility_rectangle_define(&childsize, TITLE_START_POS_X, TITLE_START_POS_Y,
			TITLE_START_POS_X + 79 - 1, TITLE_START_POS_Y + 46 - 1);
		gx_prompt_create(data_tips, NULL, parent, 0, 
			GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE,0, &childsize);
		gx_prompt_text_color_set(data_tips, GX_COLOR_ID_HONBOW_RED, 
			GX_COLOR_ID_HONBOW_RED, GX_COLOR_ID_HONBOW_RED);
		gx_prompt_font_set(data_tips, GX_FONT_ID_SIZE_46);
	}	
}

void heartrate_bar_create(GX_WIDGET *parent, Hr_bar_icon_t *hr_promt)
{
	GX_RECTANGLE size;
	INT i = 0;
	gx_utility_rectangle_define(&size, 12, 110, 308, 122);
	gx_widget_create(&hr_promt->widget, "NULL", parent, GX_STYLE_ENABLED, 7, &size);
	gx_widget_fill_color_set(&hr_promt->widget, GX_COLOR_ID_HONBOW_BLUE, 
		GX_COLOR_ID_HONBOW_BLUE, GX_COLOR_ID_HONBOW_BLUE);
	gx_widget_event_process_set(&hr_promt->widget, sport_data_common_event);

	for(i = 0; i < 5; i++){
		gx_utility_rectangle_define(&size, 12 + (i * 60), 110, 70 + (i * 60), 122);
		gx_widget_create(&hr_promt->child[i], NULL, &hr_promt->widget, GX_STYLE_ENABLED, i, &size);
		gx_widget_fill_color_set(&hr_promt->child[i], GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
		gx_widget_event_process_set(&hr_promt->child[i], sport_data_common_event);
	}
	gx_widget_draw_set(&hr_promt->widget, heartrate_bar_draw_func);
}

static GX_VERTICAL_LIST sporting_data_list;

typedef struct {
	GX_WIDGET widget0;
	GX_WIDGET widget1;	
	GX_WIDGET widget2;	
	GX_PROMPT desc0;
	GX_PROMPT desc1;
	UCHAR id;
} SPORTING_DataIconButton_TypeDef;

typedef struct SPORTING_DATA_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	SPORTING_DataIconButton_TypeDef child;
} SPORTING_DATA_LIST_ROW;

#define SPORTING_DATA_LIST_VISIBLE_ROWS 2
static SPORTING_DATA_LIST_ROW sporting_data_list_row_memory[SPORTING_DATA_LIST_VISIBLE_ROWS + 1];

static VOID sporting_data_callback(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	SPORTING_DATA_LIST_ROW *row = CONTAINER_OF(widget, SPORTING_DATA_LIST_ROW, widget);
	row->child.id = index;
}

static VOID sport_data_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	SPORTING_DATA_LIST_ROW *row = (SPORTING_DATA_LIST_ROW *)widget;
	GX_BOOL result;

	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 148, 308, 250);
		gx_widget_create(&row->widget, NULL, list, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&row->widget, sport_data_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);		
		GX_STRING string;
		app_sport_data_list_prompt_get(index, &string);
		if (index == 0){
			gx_utility_rectangle_define(&childsize, 12, 148, 308, 178);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_data_common_event);

			gx_prompt_create(&row->child.desc0, NULL, &row->child.widget0, 0, 
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc0, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&row->child.desc0, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&row->child.desc0, &string);

			gx_utility_rectangle_define(&childsize, 12, 188, 308, 234);
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
			snprintf(duration_prompt_buff, 14, "00:00:00");
			string.gx_string_ptr = duration_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&duration_prompt, &string);
		}
		else if (index == 1) {			
			gx_utility_rectangle_define(&childsize, 12, 148, 308, 178);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, 
				GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_data_common_event);

			gx_prompt_create(&row->child.desc0, NULL, &row->child.widget0, 0, 
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc0, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&row->child.desc0, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&row->child.desc0, &string);

			gx_utility_rectangle_define(&childsize, 12, 188, 130, 234);
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
			snprintf(distance_prompt_buff, 10, "00.00");
			string.gx_string_ptr = distance_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&distance_prompt, &string);

			gx_utility_rectangle_define(&childsize, 132, 202, 174, 232);
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
			km_prompt_get(&string, 0);
			gx_prompt_text_set_ext(&row->child.desc1, &string);
		}else if (index == 2) {
			gx_utility_rectangle_define(&childsize, 12, 148, 308, 178);
			gx_widget_create(&row->child.widget0, NULL, &row->widget, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			//gx_widget_event_process_set(&row->widget, sport_data_list_widget_event);
			gx_widget_fill_color_set(&row->child.widget0, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
									GX_COLOR_ID_HONBOW_BLACK);	
			gx_widget_event_process_set(&row->child.widget0, sport_data_common_event);

			gx_prompt_create(&row->child.desc0, NULL, &row->child.widget0, 0, 
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&row->child.desc0, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
										GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&row->child.desc0, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&row->child.desc0, &string);

			gx_utility_rectangle_define(&childsize, 12, 188, 140, 234);
			gx_widget_create(&row->child.widget1, NULL, &row->widget, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
			//gx_widget_event_process_set(&row->widget, sport_data_list_widget_event);
			gx_widget_fill_color_set(&row->child.widget1, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
									GX_COLOR_ID_HONBOW_BLACK);

			gx_prompt_create(&avg_pace_prompt, NULL, &row->child.widget1, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
					0, &childsize);
			gx_prompt_text_color_set(&avg_pace_prompt, GX_COLOR_ID_HONBOW_BLUE, 
				GX_COLOR_ID_HONBOW_BLUE, GX_COLOR_ID_HONBOW_BLUE);	
			gx_prompt_font_set(&avg_pace_prompt, GX_FONT_ID_SIZE_46);
			snprintf(avg_pace_prompt_buff, 16, "00\'00\"");
			string.gx_string_ptr = avg_pace_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&avg_pace_prompt, &string);	

			gx_utility_rectangle_define(&childsize, 142, 202, 197, 232);
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
			km_prompt_get(&string, 1);
			gx_prompt_text_set_ext(&row->child.desc1, &string);
		}
		row->child.id = index;
	}
}

static VOID sport_data_list_child_creat(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < SPORTING_DATA_LIST_VISIBLE_ROWS + 1; count++) {
		sport_data_list_row_create(list, 
			(GX_WIDGET *)&sporting_data_list_row_memory[count], count);
	}
}

void sport_data_list_create(GX_WIDGET *widget)
{
	GX_RECTANGLE child_size;
	gx_utility_rectangle_define(&child_size, 12, 148, 250, 359);
	gx_vertical_list_create(&sporting_data_list, NULL, widget, 3, sporting_data_callback,
		GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child_size);
	gx_widget_fill_color_set(&sporting_data_list, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
	sport_data_list_child_creat(&sporting_data_list);
}

void app_sport_data_window_init(GX_WINDOW *window)
{
	GX_RECTANGLE win_size;
	GX_STRING string;
	GX_BOOL result;
	gx_widget_created_test(&sport_data_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
		gx_widget_create(&sport_data_window, "sport_data_window", window, 
			GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
			0, &win_size);
		gx_widget_fill_color_set(&sport_data_window, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
		gx_widget_event_process_set(&sport_data_window, sport_data_window_event);
		
		GX_WIDGET *parent = &sport_data_window;
		if (get_gps_connet_status()) {
				icon_title.icon = GX_PIXELMAP_ID_APP_SPORT_IC_GPS_ON;
			}else {
				icon_title.icon = GX_PIXELMAP_ID_APP_SPORT_IC_GPS_DOWN;
		}
		custom_window_title_create(parent, &icon_title, NULL, &time_prompt);

		heartrate_title_create(parent, &icon_heart, &heart_prompt);
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
		time_prompt_buff[7] = 0;
		string.gx_string_ptr = time_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&time_prompt, &string);

		uint16_t hr = 0;	
		if (hr)	{
			snprintf(heart_prompt_buff,  8, "%d", hr);
		} else {
			snprintf(heart_prompt_buff,  8, "%s", "--");
		}
		string.gx_string_ptr = heart_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&heart_prompt, &string);

		heartrate_bar_create(parent, &hr_bar);

		sport_data_list_create(parent);
	}
}

