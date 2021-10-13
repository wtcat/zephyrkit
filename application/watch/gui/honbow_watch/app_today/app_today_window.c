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
#include "app_sports_window.h"
#include "windows_manager.h"

static TODAY_OVERVIEWS_Typdef today_overview;
static CHAR steps_buf[10];
static CHAR goal_steps_buf[10];
static CHAR active_buf[10];
static CHAR active_goal_buf[10];
static CHAR exe_buf[10];
static CHAR exe_goal_buf[10];
static CHAR dist_buf[10];
static CHAR cal_buf[10];
static CHAR sleep_hour_buf[3][4];
static CHAR sleep_min_buf[3][4];
static CHAR workout_buf[10][10];
extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);
GX_WIDGET today_main_page;
GX_WIDGET temp_black_page;
static GX_PROMPT today_prompt;
static GX_PROMPT time_prompt;
extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];
GX_PROMPT temp_prompt;
static GX_VERTICAL_LIST app_today_list;
extern APP_TODAY_WINDOW_CONTROL_BLOCK app_today_window;
typedef struct APP_TODAY_ROW_STRUCT {
	GX_WIDGET widget;	
	UCHAR id;
} APP_TODAY_LIST_ROW_T;
#define TADAY_LIST_CHILD_NUM  22
static APP_TODAY_LIST_ROW_T app_today_list_row_memory[TADAY_LIST_CHILD_NUM];

GX_RESOURCE_ID work_out_icon[11] = {
	GX_PIXELMAP_ID_WORK_OUT_A,
	GX_PIXELMAP_ID_WORK_OUT_B,
	GX_PIXELMAP_ID_WORK_OUT_C,
	GX_PIXELMAP_ID_WORK_OUT_D,
	GX_PIXELMAP_ID_WORK_OUT_E,
	GX_PIXELMAP_ID_WORK_OUT_F,
	GX_PIXELMAP_ID_WORK_OUT_G,
	GX_PIXELMAP_ID_WORK_OUT_H,
	GX_PIXELMAP_ID_WORK_OUT_I,
	GX_PIXELMAP_ID_WORK_OUT_J,
	GX_PIXELMAP_ID_WORK_OUT_K
};

GX_RESOURCE_ID activity_icon[11] = {
	GX_PIXELMAP_ID_ACTIVITY_A, //%0
	GX_PIXELMAP_ID_ACTIVITY_B, //10%
	GX_PIXELMAP_ID_ACTIVITY_C, //20%
	GX_PIXELMAP_ID_ACTIVITY_D, //30%
	GX_PIXELMAP_ID_ACTIVITY_E,	//40%
	GX_PIXELMAP_ID_ACTIVITY_F,	//50%
	GX_PIXELMAP_ID_ACTIVITY_G_1_,	//60%
	GX_PIXELMAP_ID_ACTIVITY_G,	//70%
	GX_PIXELMAP_ID_ACTIVITY_H,	//80%
	GX_PIXELMAP_ID_ACTIVITY_I,	//90%
	GX_PIXELMAP_ID_ACTIVITY_J 	//100%
};

GX_RESOURCE_ID step_icon[11] = {
	GX_PIXELMAP_ID_STEP_A,
	GX_PIXELMAP_ID_STEP_B,
	GX_PIXELMAP_ID_STEP_C,
	GX_PIXELMAP_ID_STEP_D,
	GX_PIXELMAP_ID_STEP_E,
	GX_PIXELMAP_ID_STEP_F,
	GX_PIXELMAP_ID_STEP_G,
	GX_PIXELMAP_ID_STEP_H,
	GX_PIXELMAP_ID_STEP_I,
	GX_PIXELMAP_ID_STEP_J,
	GX_PIXELMAP_ID_STEP_K
};

static GX_RESOURCE_ID today_sport_icon[] = {
	GX_PIXELMAP_ID_IC_RUN_T,
	GX_PIXELMAP_ID_IC_WALK_T,
	GX_PIXELMAP_ID_IC__BICYCLE_T,
	GX_PIXELMAP_ID_IC_INDOOR_RUNNING_T,
	GX_PIXELMAP_ID_IC_INDOOR_WALKING_T,
	GX_PIXELMAP_ID_IC_ON_FOOT_T,
	GX_PIXELMAP_ID_IIC_NDOOR_BICYCLE_T,
	GX_PIXELMAP_ID_IC_ELLIPTICAL_MACHINE_T,
	GX_PIXELMAP_ID_IC_ROWING_MACHINE_T,
	GX_PIXELMAP_ID_IC_YOGA_T,
	GX_PIXELMAP_ID_IC_MOUNTAIN_CLIMBING_T,
	GX_PIXELMAP_ID_IC_OUTDOOR_SWIMMING_T,
	GX_PIXELMAP_ID_IC_SWIMMING_T,
	GX_PIXELMAP_ID_IC_OTHER_T
};

static const GX_CHAR *lauguage_en_sport_summary_element[9] = { NULL, "Steps",
					"Ative hours", "Exercise minutes", NULL, "Distaence", 
					"Calorise", "Sleep", "Workout"};

static const GX_CHAR **sport_today_summary_string[] = {
	[LANGUAGE_ENGLISH] = lauguage_en_sport_summary_element
};

static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return sport_today_summary_string[id];
}

void app_today_summary_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

void get_today_overview_data(TODAY_OVERVIEWS_Typdef *data)
{
	data->steps = 50000;
	data->goal_steps = 200000;
	data->active_hours = 5;
	data->goal_active_hours = 8;
	data->exc_min = 28;
	data->goal_exc_min = 30;
	data->distance = 522;
	data->calorise = 300;
	data->sleep[0].sleep_lenth = 550;
	data->sleep[1].sleep_lenth = 250;
	data->sleep[2].sleep_lenth = 795;

	for(int i = 0; i < 10; i++)	{
		data->sport[i].sport_lenth = rand()%356400;
		data->sport[i].type = rand()%14;
	} 
}

VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 3

UINT app_today_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {			
		if (window->gx_widget_first_child == NULL){
			gx_widget_attach(&app_today_window, &temp_black_page);
			gx_widget_resize(&temp_black_page, &app_today_window.gx_widget_size);
		}	
	} 
	break;	
	default:
		break;
	}
	return gx_window_event_process((GX_WINDOW *)window, event_ptr);
}

UINT temp_black_page_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {	
		app_today_main_page_init(NULL);
		gx_widget_detach(window);
		gx_widget_attach(&app_today_window, &today_main_page);
		gx_widget_resize(&today_main_page, &today_main_page.gx_widget_parent->gx_widget_size);	
	} 
	break;	
	default:
		break;
	}
	return gx_widget_event_process(window, event_ptr);
}

UINT app_today_main_page_event(GX_WIDGET *window, GX_EVENT *event_ptr)
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
		if (delta_x >= 50) {
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);
			windows_mgr_page_jump((GX_WINDOW *)&app_today_window, NULL, WINDOWS_OP_BACK);
			gx_widget_delete(&today_main_page);
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
	} 
	break;
	default:
		break;
	}
	return gx_widget_event_process(window, event_ptr);
}


static const GX_CHAR *get_language_today_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_ENGLISH:
		return "Today";
	default:
		return "Today";
	}
}

GX_WIDGET *app_today_menu_screen_list[] = {
	&today_main_page,
	GX_NULL,
	GX_NULL,
};

//SCREEN_SLIDE_LEFT
void jump_today_page(GX_WIDGET *next, GX_WIDGET *prev, uint8_t dir)
{
	app_today_menu_screen_list[0] = prev;
	app_today_menu_screen_list[1] = next;
	custom_animation_start(app_today_menu_screen_list, (GX_WIDGET *)&app_today_window, dir);
}

void today_child_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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

typedef struct{
	GX_WIDGET widget;
	GX_RESOURCE_ID icon;	
}ACTIVETY_ICON_T;
ACTIVETY_ICON_T activety_icon_memory[3];

void activety_icon_widget_draw(GX_WIDGET *widget)
{
	ACTIVETY_ICON_T *row = CONTAINER_OF(widget, ACTIVETY_ICON_T, widget);
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

void today_percent_radial_bar_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_TODAY_LIST_ROW_T *row = (APP_TODAY_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	int i = 0;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 58, 319, 348);
		gx_widget_create(&row->widget, NULL, list, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

		ACTIVETY_ICON_T *workout_widget = &activety_icon_memory[0];
		gx_utility_rectangle_define(&childsize, 33, 71, 159, 232);	
		gx_widget_create(&workout_widget->widget, NULL, &row->widget, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&workout_widget->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&workout_widget->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_draw_set(&workout_widget->widget, activety_icon_widget_draw);	
		i = (today_overview.exc_min * 10)/today_overview.goal_exc_min;	
		if (i >= 10) {
			i = 10;
		}
		workout_widget->icon = work_out_icon[i];
		ACTIVETY_ICON_T *active_widget = &activety_icon_memory[1];
		gx_utility_rectangle_define(&childsize, 161, 71, 287, 232);	
		gx_widget_create(&active_widget->widget, NULL, &row->widget, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&active_widget->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&active_widget->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_draw_set(&active_widget->widget, activety_icon_widget_draw);
		i = (today_overview.active_hours * 10)/today_overview.goal_active_hours;	
		if (i >= 10) {
			i = 10;
		}					
		active_widget->icon = activity_icon[i];

		ACTIVETY_ICON_T *steps_widget = &activety_icon_memory[2];
		gx_utility_rectangle_define(&childsize, 58, 232, 272, 335);	
		gx_widget_create(&steps_widget->widget, NULL, &row->widget, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&steps_widget->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&steps_widget->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_draw_set(&steps_widget->widget, activety_icon_widget_draw);
		i = (today_overview.steps * 10)/today_overview.goal_steps;	
		if (i >= 10) {
			i = 10;
		}							
		steps_widget->icon = step_icon[i];		
	}
}

typedef struct {
	GX_PROMPT title;
	GX_PROMPT data;
	GX_PROMPT goal;
}TODAY_LIST_WIDGET_T;
TODAY_LIST_WIDGET_T today_list_widget_memory;

#define WIDE_30  27
void today_list_steps_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_TODAY_LIST_ROW_T *row = (APP_TODAY_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 348, 319, 460);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
			index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);		
		TODAY_LIST_WIDGET_T *single_widget = &today_list_widget_memory;
		gx_utility_rectangle_define(&childsize, 12, 348, 319, 378);	
		gx_prompt_create(&single_widget->title, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&single_widget->title, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&single_widget->title, GX_FONT_ID_SIZE_30);
		app_today_summary_list_prompt_get(index, &string);	
		gx_prompt_text_set_ext(&single_widget->title, &string);

		if(today_overview.steps >= 1000000) {
			snprintf(steps_buf, 10, "%dk", today_overview.steps/1000);
		} else {
			snprintf(steps_buf, 10, "%d", today_overview.steps);
		}		
		gx_utility_rectangle_define(&childsize, 12, 388, 12 + WIDE_30*strlen(steps_buf), 434);		
		gx_prompt_create(&single_widget->data, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&single_widget->data, GX_COLOR_ID_HONBOW_GREEN, 
			GX_COLOR_ID_HONBOW_GREEN, GX_COLOR_ID_HONBOW_GREEN);	
		gx_prompt_font_set(&single_widget->data, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = steps_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&single_widget->data, &string);
		

		if (today_overview.goal_steps >= 10000)	{
			snprintf(goal_steps_buf, 10, "/%dk", today_overview.goal_steps/1000);
		} else {
			snprintf(goal_steps_buf, 10, "/%d", today_overview.goal_steps);
		}				
		gx_utility_rectangle_define(&childsize, 1+12 + WIDE_30*strlen(steps_buf), 388, 
		 	1+ 12 + WIDE_30*(strlen(steps_buf) + strlen(goal_steps_buf)), 434);
		gx_prompt_create(&single_widget->goal, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&single_widget->goal, GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&single_widget->goal, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = goal_steps_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&single_widget->goal, &string);
	}
}

TODAY_LIST_WIDGET_T today_list_active_memory;
void today_list_active_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_TODAY_LIST_ROW_T *row = (APP_TODAY_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 460, 319, 572);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
			index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
		TODAY_LIST_WIDGET_T *active_prompt = &today_list_active_memory;				
		gx_utility_rectangle_define(&childsize, 12, 460, 319, 490);		
		gx_prompt_create(&active_prompt->title, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&active_prompt->title, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&active_prompt->title, GX_FONT_ID_SIZE_30);
		app_today_summary_list_prompt_get(index, &string);	
		gx_prompt_text_set_ext(&active_prompt->title, &string);
				
		snprintf(active_buf, 10, "%d", today_overview.active_hours%24);				
		gx_utility_rectangle_define(&childsize, 12, 500, 12 + WIDE_30*strlen(active_buf), 546);		
		gx_prompt_create(&active_prompt->data, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&active_prompt->data, GX_COLOR_ID_HONBOW_BLUE, 
			GX_COLOR_ID_HONBOW_BLUE, GX_COLOR_ID_HONBOW_BLUE);	
		gx_prompt_font_set(&active_prompt->data, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = active_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&active_prompt->data, &string);
		
		snprintf(active_goal_buf, 10, "/%dh", today_overview.goal_active_hours%24);		
		gx_utility_rectangle_define(&childsize, 1+12 + WIDE_30*strlen(active_buf), 500, 
			1+ 12 + WIDE_30*(strlen(active_buf) + strlen(active_goal_buf)), 546);		
		gx_prompt_create(&active_prompt->goal, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&active_prompt->goal, GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&active_prompt->goal, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = active_goal_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&active_prompt->goal, &string);
	}
}

TODAY_LIST_WIDGET_T today_list_exericise_memory;
void today_list_exercise_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_TODAY_LIST_ROW_T *row = (APP_TODAY_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 572, 319, 684);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								GX_COLOR_ID_HONBOW_BLACK);
		TODAY_LIST_WIDGET_T *exercise_promt = &today_list_exericise_memory;
		gx_utility_rectangle_define(&childsize, 12, 572, 319, 602);		
		gx_prompt_create(&exercise_promt->title, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&exercise_promt->title, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
									GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&exercise_promt->title, GX_FONT_ID_SIZE_30);
		app_today_summary_list_prompt_get(index, &string);	
		gx_prompt_text_set_ext(&exercise_promt->title, &string);
				
		snprintf(exe_buf, 10, "%d", today_overview.exc_min);				
		gx_utility_rectangle_define(&childsize, 12, 612, 12 + WIDE_30*strlen(exe_buf), 658);	
		gx_prompt_create(&exercise_promt->data, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&exercise_promt->data, GX_COLOR_ID_HONBOW_ORANGE, GX_COLOR_ID_HONBOW_ORANGE,
									GX_COLOR_ID_HONBOW_ORANGE);	
		gx_prompt_font_set(&exercise_promt->data, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = exe_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&exercise_promt->data, &string);
		
		snprintf(exe_goal_buf, 10, "/%dmin", today_overview.goal_exc_min%24);		
		gx_utility_rectangle_define(&childsize, 1+12 + WIDE_30*strlen(exe_buf), 612, 
			1+ 12 + WIDE_30*(strlen(exe_buf) + strlen(exe_goal_buf)), 658);		
		gx_prompt_create(&exercise_promt->goal, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&exercise_promt->goal, GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK,
									GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&exercise_promt->goal, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = exe_goal_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&exercise_promt->goal, &string);	
	}
}

void line_widget_draw(GX_WIDGET *widget)
{	
	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);
	gx_canvas_rectangle_draw(&fillrect);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_DARK_BLACK, 
		GX_COLOR_ID_HONBOW_DARK_BLACK, GX_BRUSH_ALIAS);
	gx_context_brush_width_set(1);
	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left, 
						widget->gx_widget_size.gx_rectangle_top,
						widget->gx_widget_size.gx_rectangle_right,
						widget->gx_widget_size.gx_rectangle_top);
}

void today_list_line_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_TODAY_LIST_ROW_T *row = (APP_TODAY_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 684, 308, 711);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_draw_set(&row->widget, line_widget_draw);			
	}
}

TODAY_LIST_WIDGET_T today_list_distance_memory;
void today_list_distance_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_TODAY_LIST_ROW_T *row = (APP_TODAY_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 711, 319, 823);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								GX_COLOR_ID_HONBOW_BLACK);
		TODAY_LIST_WIDGET_T *distance_prompt = &today_list_distance_memory;
		gx_utility_rectangle_define(&childsize, 12, 711, 319, 741);		
		gx_prompt_create(&distance_prompt->title, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&distance_prompt->title, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
									GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&distance_prompt->title, GX_FONT_ID_SIZE_30);
		app_today_summary_list_prompt_get(index, &string);	
		gx_prompt_text_set_ext(&distance_prompt->title, &string);
				
		snprintf(dist_buf, 10, "%.1f", today_overview.distance/100.0);	
		gx_utility_rectangle_define(&childsize, 12, 751, 12 + strlen(dist_buf) * WIDE_30, 797);	
		gx_prompt_create(&distance_prompt->data, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&distance_prompt->data, GX_COLOR_ID_HONBOW_GREEN, GX_COLOR_ID_HONBOW_GREEN,
									GX_COLOR_ID_HONBOW_GREEN);	
		gx_prompt_font_set(&distance_prompt->data, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = dist_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&distance_prompt->data, &string);

		gx_utility_rectangle_define(&childsize, 12 + strlen(dist_buf) * WIDE_30, 751, 319, 797);
		gx_prompt_create(&distance_prompt->goal, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&distance_prompt->goal, GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK,
									GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&distance_prompt->goal, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = "km";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&distance_prompt->goal, &string);
	}
}

TODAY_LIST_WIDGET_T today_list_calorise_memory;
void today_list_calorise_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_TODAY_LIST_ROW_T *row = (APP_TODAY_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 823, 319, 935);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								GX_COLOR_ID_HONBOW_BLACK);

		TODAY_LIST_WIDGET_T *calorise_prompt = &today_list_calorise_memory;
		gx_utility_rectangle_define(&childsize, 12, 823, 319, 853);		
		gx_prompt_create(&calorise_prompt->title, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&calorise_prompt->title, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
									GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&calorise_prompt->title, GX_FONT_ID_SIZE_30);
		app_today_summary_list_prompt_get(index, &string);	
		gx_prompt_text_set_ext(&calorise_prompt->title, &string);
				
		snprintf(cal_buf, 10, "%d", today_overview.calorise);				
		gx_utility_rectangle_define(&childsize, 12, 863, 12 + strlen(cal_buf) * WIDE_30, 909);	
		gx_prompt_create(&calorise_prompt->data, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&calorise_prompt->data, GX_COLOR_ID_HONBOW_RED, GX_COLOR_ID_HONBOW_RED,
									GX_COLOR_ID_HONBOW_RED);	
		gx_prompt_font_set(&calorise_prompt->data, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = cal_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&calorise_prompt->data, &string);	

		gx_utility_rectangle_define(&childsize, 12 + strlen(cal_buf) * WIDE_30, 863, 319, 909);		
		gx_prompt_create(&calorise_prompt->goal, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&calorise_prompt->goal, GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK,
									GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&calorise_prompt->goal, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = "kcal";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&calorise_prompt->goal, &string);		
	}
}

GX_PROMPT sleep_title;
void today_list_sleep_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_TODAY_LIST_ROW_T *row = (APP_TODAY_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 935, 319, 983);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								GX_COLOR_ID_HONBOW_BLACK);

		gx_utility_rectangle_define(&childsize, 12, 935, 319, 965);	
		gx_prompt_create(&sleep_title, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&sleep_title, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
									GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&sleep_title, GX_FONT_ID_SIZE_30);
		app_today_summary_list_prompt_get(index, &string);	
		gx_prompt_text_set_ext(&sleep_title, &string);
	}
}

SLEEP_DATA_STRUCT_DEF sleep_data;
typedef struct {
	GX_WIDGET widget;
	GX_PROMPT decs;
    GX_PROMPT decs2;
    GX_PROMPT decs3;
    GX_PROMPT decs4;
	GX_RESOURCE_ID icon;
	GX_RESOURCE_ID bg_color;
	GX_RESOURCE_ID nor_color;
    UINT id;
}TODAY_SLEEP_LIST_BUTTON_T;
TODAY_SLEEP_LIST_BUTTON_T sleep_list_button_memory[3];
TODAY_SLEEP_LIST_BUTTON_T sport_list_button_memory[10];

void sleep_list_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	APP_TODAY_LIST_ROW_T *row = CONTAINER_OF(widget, APP_TODAY_LIST_ROW_T, widget);		
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(1, GX_EVENT_CLICKED):	
		init_sleep_data_test(row->id, &sleep_data);
		app_sleep_data_window_init(NULL, &sleep_data);
		jump_today_page(&sleep_data_widget, &today_main_page, SCREEN_SLIDE_LEFT);
	break;
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

void workout_list_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	APP_TODAY_LIST_ROW_T *row = CONTAINER_OF(widget, APP_TODAY_LIST_ROW_T, widget);	
	switch (event_ptr->gx_event_type) {	
	case GX_SIGNAL(1, GX_EVENT_CLICKED):	
		if (21 == row->id){
			app_no_sport_record_window_init(NULL);
			jump_today_page(&no_sport_record_window, &today_main_page, SCREEN_SLIDE_LEFT);
		} else {
			app_sport_summary_window_init(NULL, false);
			jump_today_page(&sport_summary_window, &today_main_page, SCREEN_SLIDE_LEFT);
		}
		
	break;
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

void sleep_list_button_common_draw(GX_WIDGET *widget)
{
	TODAY_SLEEP_LIST_BUTTON_T *row = CONTAINER_OF(widget, TODAY_SLEEP_LIST_BUTTON_T, widget);
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
		xpos = widget->gx_widget_size.gx_rectangle_left+10;
		ypos = widget->gx_widget_size.gx_rectangle_top+15;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

void sleep_list_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_TODAY_LIST_ROW_T *row = (APP_TODAY_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		row->id = index;
		gx_utility_rectangle_define(&childsize, 12, 983 + (index-8) * 90, 319, 1073 + (index-8) * 90);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
			index+1, &childsize);
		gx_widget_event_process_set(&row->widget, sleep_list_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		TODAY_SLEEP_LIST_BUTTON_T *sleep_button = &sleep_list_button_memory[index - 8];
		gx_utility_rectangle_define(&childsize, 12, 983 + (index-8) * 90, 308, 1063 + (index-8) * 90);
		gx_widget_create(&sleep_button->widget, NULL, &row->widget, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 1, &childsize);
		gx_widget_event_process_set(&sleep_button->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&sleep_button->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
		snprintf(sleep_hour_buf[index-8], 4, "%d", 
			(today_overview.sleep[index-8].sleep_lenth/60)%100);	

		sleep_button->nor_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
		sleep_button->id = index;
		sleep_button->icon = GX_PIXELMAP_ID_IC_MOON;
		gx_widget_draw_set(&sleep_button->widget, sleep_list_button_common_draw);
		GX_RECTANGLE size;
		gx_utility_rectangle_define (&size, 76, 
			1000 + (index-8) * 90, 
			76 + 27 * strlen(sleep_hour_buf[index-8]), 
			1046 + (index-8) * 90);
		gx_prompt_create(&sleep_button->decs, NULL, &sleep_button->widget, 0,
			GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
			0, &size);
		gx_prompt_text_color_set(&sleep_button->decs, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&sleep_button->decs, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = sleep_hour_buf[index-8];
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&sleep_button->decs, &string);	

		gx_utility_rectangle_define (&size, 76 + 27 * strlen(sleep_hour_buf[index-8]), 
						1014 + (index-8) * 90, 
						17 + 76 + 27 * strlen(sleep_hour_buf[index-8]), 
						1046 + (index-8) * 90);
		gx_prompt_create(&sleep_button->decs2, NULL, &sleep_button->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &size);
		gx_prompt_text_color_set(&sleep_button->decs2, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&sleep_button->decs2, GX_FONT_ID_SIZE_30);
		string.gx_string_ptr = "h";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&sleep_button->decs2, &string);	
		snprintf(sleep_min_buf[index-8], 4, "%d", (today_overview.sleep[index-8].sleep_lenth%60));
		gx_utility_rectangle_define (&size, 17 + 76 + 27 * strlen(sleep_hour_buf[index-8]), 
			1000 + (index-8) * 90 , 
			17 + 76 + 27 * (strlen(sleep_hour_buf[index-8]) + strlen(sleep_min_buf[index-8])), 
			1046 + (index-8) * 90);
		gx_prompt_create(&sleep_button->decs3, NULL, &sleep_button->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &size);
		gx_prompt_text_color_set(&sleep_button->decs3, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&sleep_button->decs3, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = sleep_min_buf[index-8];
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&sleep_button->decs3, &string);	

		gx_utility_rectangle_define (&size, 
			17 + 76 + 27 * (strlen(sleep_hour_buf[index-8]) + strlen(sleep_min_buf[index-8])), 
								1014 + (index-8) * 90, 
								308,
								1046 + (index-8) * 90);
		gx_prompt_create(&sleep_button->decs4, NULL, &sleep_button->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &size);
		gx_prompt_text_color_set(&sleep_button->decs4, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&sleep_button->decs4, GX_FONT_ID_SIZE_30);
		string.gx_string_ptr = "min";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&sleep_button->decs4, &string);
	}		
}

GX_PROMPT sport_title;
void today_sport_rec_tittl_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_TODAY_LIST_ROW_T *row = (APP_TODAY_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 1253, 319, 1311);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
			index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

		gx_utility_rectangle_define(&childsize, 12, 1269, 319, 1299);	
		gx_prompt_create(&sport_title, NULL, &row->widget, 0,
			GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 0, &childsize);
		gx_prompt_text_color_set(&sport_title, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&sport_title, GX_FONT_ID_SIZE_30);
		app_today_summary_list_prompt_get(index - 3, &string);	
		gx_prompt_text_set_ext(&sport_title, &string);
	}
}

void workout_list_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	int i = index -12;
	APP_TODAY_LIST_ROW_T *row = (APP_TODAY_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	row->id = index;
	gx_widget_created_test(&row->widget, &result);

	if (!result) {
		gx_utility_rectangle_define(&childsize, 
						12, 
						1311 + i * 90, 
						319, 
						1401 + i * 90);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
			index+1, &childsize);
		gx_widget_event_process_set(&row->widget, workout_list_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

		TODAY_SLEEP_LIST_BUTTON_T *sport_button = &sport_list_button_memory[i];
		gx_utility_rectangle_define(&childsize, 12, 
						1311 + i * 90, 
						308, 
						1391 + i * 90);
		gx_widget_create(&sport_button->widget, NULL, &row->widget, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 1, &childsize);
		gx_widget_event_process_set(&sport_button->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&sport_button->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		sport_button->nor_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
		sport_button->id = index;
		sport_button->icon = today_sport_icon[today_overview.sport[i].type];
		gx_widget_draw_set(&sport_button->widget, sleep_list_button_common_draw);
		GX_RECTANGLE size;
		gx_utility_rectangle_define (&size, 
					82, 
					1331 + i * 90, 
					264, 
					1371 + i * 90);
		gx_prompt_create(&sport_button->decs, NULL, &sport_button->widget, 0,
			GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 0, &size);
		gx_prompt_text_color_set(&sport_button->decs, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&sport_button->decs, GX_FONT_ID_SIZE_46);
		snprintf(workout_buf[i], 10, "%02d:%02d:%02d", 
				(today_overview.sport[i].sport_lenth/3600)%100,
				(today_overview.sport[i].sport_lenth%3660)/60,
				today_overview.sport[i].sport_lenth%60);
		string.gx_string_ptr = workout_buf[i];
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&sport_button->decs, &string);	
	}
}


void today_list_elempent_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_BOOL result;
	APP_TODAY_LIST_ROW_T *row = CONTAINER_OF(widget, APP_TODAY_LIST_ROW_T, widget);
	gx_widget_created_test(&row->widget, &result);
	if(!result) {
		switch (index) {
		case 0:
			today_percent_radial_bar_create(list, widget, index);
		break;
		case 1:
			today_list_steps_create(list, widget, index);
		break;
		case 2:
			today_list_active_create(list, widget, index);
		break;
		case 3:
			today_list_exercise_create(list, widget, index);
		break;
		case 4:
			today_list_line_create(list, widget, index);
		break;
		case 5:
			today_list_distance_create(list, widget, index);
		break;
		case 6:
			today_list_calorise_create(list, widget, index);
		break;
		case 7:
			today_list_sleep_create(list, widget, index);
		break;
		case 8:
			sleep_list_create(list, widget, index);
		break;
		case 9:
			sleep_list_create(list, widget, index);
		break;
		case 10:
			sleep_list_create(list, widget, index);
		break;
		case 11:
			today_sport_rec_tittl_create(list, widget, index);
		break;
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
			workout_list_create(list, widget, index);
		break;	
		default:
		break;
		}			
	}
}

static VOID sport_today_list_child_creat(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < 22; count++) {
		today_list_elempent_create(list, 
			(GX_WIDGET *)&app_today_list_row_memory[count], count);
	}
}

void sport_today_list_create(GX_WIDGET *widget)
{
	GX_RECTANGLE child_size;
	gx_utility_rectangle_define(&child_size, 0, 58, 319, 359);
	gx_vertical_list_create(&app_today_list, NULL, widget, 22, NULL,
		GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child_size);
	gx_widget_fill_color_set(&app_today_list, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
	sport_today_list_child_creat(&app_today_list);
}

void app_today_main_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE win_size;
	// GX_STRING string;
	get_today_overview_data(&today_overview);
	GX_BOOL result;
	gx_widget_created_test(&today_main_page, &result);
	if (!result)
	{
		gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
		gx_widget_create(&today_main_page, "today_main_page", window, 
			GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
			0, &win_size);
		gx_widget_fill_color_set(&today_main_page, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
		gx_widget_event_process_set(&today_main_page, app_today_main_page_event);
		
		GX_WIDGET *parent = &today_main_page;
		custom_window_title_create(parent, NULL, &today_prompt, &time_prompt);

		GX_STRING string;
		string.gx_string_ptr = get_language_today_title();
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&today_prompt, &string);
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
		time_prompt_buff[7] = 0;
		string.gx_string_ptr = time_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&time_prompt, &string);
		sport_today_list_create(parent);
	}	
}

void app_today_window_init(GX_WINDOW *window)
{
	gx_widget_event_process_set(window, app_today_window_event);

	GX_RECTANGLE win_size;
	GX_BOOL result;
	gx_widget_created_test(&temp_black_page, &result);
	if (!result) {
		gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
		gx_widget_create(&temp_black_page, "temp_black_window", window, 
			GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED, 
			0, &win_size);
		gx_widget_fill_color_set(&temp_black_page, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
		gx_widget_event_process_set(&temp_black_page, temp_black_page_event);
	}
}

GUI_APP_DEFINE_WITH_INIT(today, APP_ID_01_TODAY_OVERVIEW, (GX_WIDGET *)&app_today_window,
	GX_PIXELMAP_ID_APP_LIST_ICON_TODAY_OVERVIEW_90_90, GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY,
	app_today_window_init);