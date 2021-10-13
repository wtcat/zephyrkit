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
#include "app_heart_window.h"

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

GX_WIDGET heart_main_page;
static GX_WIDGET test_window;
static GX_WIDGET hr_data_window;
static HEART_DETECTION_WIGET_T heart_hr_data_widget_memory;
static uint32_t previous_hr = 0;
time_t previous_ts = 0; 
static GX_PROMPT heart_prompt;
static GX_PROMPT time_prompt;
static GX_PROMPT pre_data_promt;
static GX_PROMPT hr_previous_data_promt;
static char pre_data_buf[20];
GX_PROMPT max_promt, min_promt;
CHAR max_buf[5];
CHAR min_buf[5];

extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];
static AllDaysHr_T day_hr = {0};

static GX_VERTICAL_LIST app_heart_list;
extern APP_HEART_WINDOW_CONTROL_BLOCK app_heart_window;

typedef struct APP_HEART_ROW_STRUCT {
	GX_WIDGET widget;	
	UCHAR id;
} APP_HEART_LIST_ROW_T;
#define HEART_LIST_CHILD_NUM  4
static APP_HEART_LIST_ROW_T app_heart_list_row_memory[HEART_LIST_CHILD_NUM];

static const GX_CHAR *lauguage_en_sport_summary_element[] = { NULL, "Measuring", "Resting", 
	"Heart Rate"};

static const GX_CHAR **sport_today_summary_string[] = {
	[LANGUAGE_ENGLISH] = lauguage_en_sport_summary_element
};

static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return sport_today_summary_string[id];
}

void app_heart_summary_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 3

uint8_t resting_hr = 0;
GX_PROMPT resting_prompt;
char resting_buf[5];

static GX_PROMPT hr_data_promt;
static GX_PROMPT hr_bpm_promt;
static CHAR hr_data_buf[5];
static uint8_t HeartRate = 0;

#define DATA_OK   1
#define DATA_MEASURING  2
uint8_t window_status = DATA_MEASURING;

void heart_data_widget_switch()
{
	if(window_status == DATA_OK){
		if (!HeartRate) {
			window_status = DATA_MEASURING;
			gx_widget_detach(&hr_data_window);
			init_test_data_window(&app_heart_list_row_memory[0].widget);
			gx_widget_attach(&app_heart_list_row_memory[0].widget, &test_window);
			gx_widget_resize(&test_window, &app_heart_list_row_memory[0].widget.gx_widget_size);
			Gui_close_hr();
		}		
	}else {
		if (HeartRate) {
			window_status = DATA_OK;
			gx_widget_detach(&test_window);
			init_hr_data_window(&app_heart_list_row_memory[0].widget);
			gx_widget_attach(&app_heart_list_row_memory[0].widget, &hr_data_window);
			gx_widget_resize(&hr_data_window, &app_heart_list_row_memory[0].widget.gx_widget_size);
		}
	}
}

UINT app_heart_main_page_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	static uint8_t flag = 1;
	static UINT count = 0;
	static UINT times = 0;
	static uint8_t min_hr = 0, max_hr = 0;
	static PRE_HR_T pre_hr;
	GX_STRING string;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {		
		gx_system_timer_start(window, TIME_FRESH_TIMER_ID, 50, 50);
		count = 0;
		Gui_open_hr();		
		Gui_get_pre_hr(&pre_hr);
		times++;
		printf("open times : %d\n", times);
	} 
	break;
	case GX_EVENT_HIDE: {
		gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);	
		HeartRate = 0;
		memset(&day_hr, 0, sizeof(AllDaysHr_T));
	} 
	break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}	
	case GX_EVENT_PEN_UP: {
	 	delta_x = (event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
	 	if (delta_x >= 50) {
			windows_mgr_page_jump((GX_WINDOW *)&heart_main_page, NULL, WINDOWS_OP_BACK);
			flag = 1;			
			//screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);
			gx_widget_delete(&hr_data_window);	
			Gui_close_hr();									
			init_test_data_window(&app_heart_list_row_memory[0].widget);
			gx_widget_attach(&app_heart_list_row_memory[0].widget, &test_window);
			gx_widget_resize(&test_window, &app_heart_list_row_memory[0].widget.gx_widget_size);
			window_status = DATA_MEASURING;	
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
			windows_mgr_page_jump((GX_WINDOW *)&heart_main_page, NULL, WINDOWS_OP_BACK);
			flag = 1;	
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);
			gx_widget_delete(&hr_data_window);
			Gui_close_hr();				
			init_test_data_window(&app_heart_list_row_memory[0].widget);
			gx_widget_attach(&app_heart_list_row_memory[0].widget, &test_window);
			gx_widget_resize(&test_window, &app_heart_list_row_memory[0].widget.gx_widget_size);
			window_status = DATA_MEASURING;		
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

			HeartRate = Gui_get_hr();
			if (HeartRate) {
				count = 0;
				flag = 0;
			} else {
				count++;
			}
			if (count == 20) {
				if (flag) {
					jump_heart_page(&please_wear_window, &heart_main_page,  SCREEN_SLIDE_LEFT);
					flag = 0;
				}
			}
			resting_hr = Gui_get_resting_hr();			
			previous_hr = pre_hr.pre_hr;
			previous_ts = pre_hr.pre_time;
			if (!previous_ts) {
				previous_ts = now;
			}				
			if (!resting_hr) {
				snprintf(resting_buf, 5, "%s", "--");
			}else {
				snprintf(resting_buf, 5, "%3d", resting_hr);
			}		
			string.gx_string_ptr = resting_buf;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&resting_prompt, &string);	
			
			get_24hour_hr_data(&day_hr);
			gx_system_dirty_mark((GX_WIDGET *) &app_heart_list_row_memory[1]);			

			if((min_hr > day_hr.hour_hr[tm_now->tm_hour].min_hr) || (0 == min_hr)){				
				min_hr = day_hr.hour_hr[tm_now->tm_hour].min_hr;
				if (min_hr) {
					snprintf(min_buf, 5, "%3d", min_hr);
				} else {
					snprintf(min_buf, 5, "%s", "--");
				}				
				string.gx_string_ptr = min_buf;
				string.gx_string_length = strlen(string.gx_string_ptr);
				gx_prompt_text_set_ext(&min_promt, &string);
			}

			if(max_hr < day_hr.hour_hr[tm_now->tm_hour].max_hr) {				
				max_hr = day_hr.hour_hr[tm_now->tm_hour].max_hr;
				if (max_hr) {
					snprintf(max_buf, 5, "%3d", max_hr);
				} else {
					snprintf(max_buf, 5, "%s", "--");
				}				
				string.gx_string_ptr = max_buf;
				string.gx_string_length = strlen(string.gx_string_ptr);
				gx_prompt_text_set_ext(&max_promt, &string);
			}
			
			if (HeartRate) {
				heart_data_widget_switch();				
			}
			if (window_status == DATA_OK) {
				if (HeartRate) {
					snprintf(hr_data_buf, 5, "%3d", HeartRate);
					string.gx_string_ptr = hr_data_buf;
					string.gx_string_length = strlen(string.gx_string_ptr);
					gx_prompt_text_set_ext(&hr_data_promt, &string);
				} else {
					snprintf(hr_data_buf, 5, "%s", "--");
					string.gx_string_ptr = hr_data_buf;
					string.gx_string_length = strlen(string.gx_string_ptr);
					gx_prompt_text_set_ext(&hr_data_promt, &string);
				}				
		
				if(previous_hr) {
					gx_prompt_text_color_set(&hr_previous_data_promt, GX_COLOR_ID_HONBOW_DARK_BLACK, 
						GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);
					now = now - previous_ts;
					tm_now = gmtime(&now);
					if (tm_now->tm_year - 70)	{
						snprintf(pre_data_buf, 20, "%dBPM(%dyears ago)", previous_hr, (tm_now->tm_year - 70));
					} else if(tm_now->tm_mon) {
						snprintf(pre_data_buf, 20, "%dBPM(%dmons ago)", previous_hr,(tm_now->tm_mon));
					} else if (tm_now->tm_mday - 1) {
						snprintf(pre_data_buf, 20, "%dBPM(%ddays ago)", previous_hr, (tm_now->tm_mday - 1));
					} else if (tm_now->tm_hour) {
						snprintf(pre_data_buf, 20, "%dBPM(%dhours ago)", previous_hr, (tm_now->tm_hour));
					} else if (tm_now->tm_min) {
						snprintf(pre_data_buf, 20, "%dBPM(%dmin ago)", previous_hr, (tm_now->tm_min));
					} else {
						snprintf(pre_data_buf, 20, "%dBPM(1min ago)", previous_hr);
					}
					string.gx_string_ptr = pre_data_buf;
					string.gx_string_length = strlen(string.gx_string_ptr);
					gx_prompt_text_set_ext(&hr_previous_data_promt, &string);
				}else 	{
					gx_prompt_text_color_set(&hr_previous_data_promt, GX_COLOR_ID_HONBOW_BLACK, 
						GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
				}
			}

			if (window_status == DATA_MEASURING) {
				if(previous_hr) {
					gx_prompt_text_color_set(&pre_data_promt, GX_COLOR_ID_HONBOW_DARK_BLACK, 
						GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);
					now = now - previous_ts;
					tm_now = gmtime(&now);
					if (tm_now->tm_year - 70)	{
						snprintf(pre_data_buf, 20,"%dBPM(%dyears ago)", previous_hr, 
							(tm_now->tm_year - 70));
					} else if(tm_now->tm_mon) {
						snprintf(pre_data_buf, 20,"%dBPM(%dmons ago)", previous_hr,(tm_now->tm_mon));
					} else if (tm_now->tm_mday - 1) {
						snprintf(pre_data_buf, 20,"%dBPM(%ddays ago)", previous_hr, (tm_now->tm_mday - 1));
					} else if (tm_now->tm_hour) {
						snprintf(pre_data_buf, 20,"%dBPM(%dhours ago)", previous_hr, (tm_now->tm_hour));
					} else if (tm_now->tm_min) {
						snprintf(pre_data_buf, 20,"%dBPM(%dmin ago)", previous_hr, (tm_now->tm_min));
					} else {
						snprintf(pre_data_buf, 20,"%dBPM(1min ago)", previous_hr);
					}
					string.gx_string_ptr = pre_data_buf;
					string.gx_string_length = strlen(string.gx_string_ptr);
					gx_prompt_text_set_ext(&pre_data_promt, &string);
				}else 	{
					gx_prompt_text_color_set(&pre_data_promt, GX_COLOR_ID_HONBOW_BLACK, 
						GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
				}
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

static const GX_CHAR *get_language_heart_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_ENGLISH:
		return "Heart Rate";
	default:
		return "Heart Rate";
	}
}

GX_WIDGET *app_heart_menu_screen_list[] = {
	&heart_main_page,
	GX_NULL,
	GX_NULL,
};

//SCREEN_SLIDE_LEFT
void jump_heart_page(GX_WIDGET *next, GX_WIDGET *prev, uint8_t dir)
{
	app_heart_menu_screen_list[0] = prev;
	app_heart_menu_screen_list[1] = next;
	custom_animation_start(app_heart_menu_screen_list, (GX_WIDGET *)&app_heart_window, dir);
}

void heart_child_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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

HEART_DETECTION_WIGET_T heart_detection_widget_memory;
void heart_icon_widget_draw(GX_WIDGET *widget)
{
	MaxMinHr_Widget_T *row = CONTAINER_OF(widget, MaxMinHr_Widget_T, hr_widget);
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

static GX_SPRITE sprite;
static GX_SPRITE hr_data_sprite;

GX_SPRITE_FRAME heart_data_screen_sprite_frame_list[6] =
{
    {
        GX_PIXELMAP_ID_APP_HEART_SMALL_BEAT01,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        20,                                   /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_HEART_SMALL_BEAT02,   /* pixelmap id                    */
        0,                                   /* x offset                       */
        0,                                   /* y offset                       */
        10,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_HEART_SMALL_BEAT03,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        5,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_HEART_SMALL_BEAT04,   /* pixelmap id                    */
        0,                                   /* x offset                       */
        0,                                   /* y offset                       */
        5,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_HEART_SMALL_BEAT03,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        5,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_HEART_SMALL_BEAT02,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        10,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    }
};

GX_SPRITE_FRAME heart_screen_sprite_frame_list[6] =
{
    {
        GX_PIXELMAP_ID_APP_HEART_BIG_BEAT01,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        20,                                   /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_HEART_BIG_BEAT02,   /* pixelmap id                    */
        0,                                   /* x offset                       */
        0,                                   /* y offset                       */
        10,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_HEART_BIG_BEAT03,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        5,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_HEART_BIG_BEAT04,   /* pixelmap id                    */
        0,                                   /* x offset                       */
        0,                                   /* y offset                       */
        5,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_HEART_BIG_BEAT03,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        5,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_HEART_BIG_BEAT02,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        10,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    }
};


//12, 58, 308, 196);
void init_hr_data_window(GX_WIDGET *parent)
{
	GX_RECTANGLE childsize;
	GX_STRING string;
	GX_BOOL result;
	gx_widget_created_test(&hr_data_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 
		parent->gx_widget_size.gx_rectangle_left, 
		parent->gx_widget_size.gx_rectangle_top,
		parent->gx_widget_size.gx_rectangle_right,
		parent->gx_widget_size.gx_rectangle_bottom);
		gx_widget_create(&hr_data_window, "heart_data_widget", parent, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&hr_data_window, heart_child_widget_common_event);
		gx_widget_fill_color_set(&hr_data_window, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
		
		HEART_DETECTION_WIGET_T *hr_data_widget = &heart_hr_data_widget_memory;

		gx_utility_rectangle_define(&childsize,
		parent->gx_widget_size.gx_rectangle_left, 
		parent->gx_widget_size.gx_rectangle_top,
		parent->gx_widget_size.gx_rectangle_right,
		parent->gx_widget_size.gx_rectangle_bottom - 112);		
		gx_prompt_create(&hr_data_widget->decs1, NULL, &hr_data_window, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&hr_data_widget->decs1, GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&hr_data_widget->decs1, GX_FONT_ID_SIZE_26);
		app_heart_summary_list_prompt_get(3, &string);	
		gx_prompt_text_set_ext(&hr_data_widget->decs1, &string);

		gx_utility_rectangle_define(&childsize, 
		parent->gx_widget_size.gx_rectangle_left + 8, 
		parent->gx_widget_size.gx_rectangle_top + 42,
		parent->gx_widget_size.gx_rectangle_right -  248,
		parent->gx_widget_size.gx_rectangle_bottom - 56);
		gx_widget_create(&hr_data_widget->widget, NULL, &hr_data_window, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_fill_color_set(&hr_data_widget->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		gx_sprite_create(&hr_data_sprite, NULL, 
			&hr_data_widget->widget, heart_data_screen_sprite_frame_list,
			6, GX_STYLE_BORDER_NONE|GX_STYLE_TRANSPARENT|GX_STYLE_ENABLED|GX_STYLE_SPRITE_AUTO|GX_STYLE_SPRITE_LOOP,
			GX_ID_NONE, &childsize);	

		gx_utility_rectangle_define(&childsize, 
		parent->gx_widget_size.gx_rectangle_left + 52, 
		parent->gx_widget_size.gx_rectangle_top + 36,
		parent->gx_widget_size.gx_rectangle_right - 146,
		parent->gx_widget_size.gx_rectangle_bottom - 56);
		gx_prompt_create(&hr_data_promt, NULL, &hr_data_window, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&hr_data_promt, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&hr_data_promt, GX_FONT_ID_SIZE_46);
		if (HeartRate) {
			snprintf(hr_data_buf, 5,"%3d", HeartRate);
		} else {
			snprintf(hr_data_buf, 5, "%s", "--");
		}	
		string.gx_string_ptr = hr_data_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&hr_data_promt, &string);

		gx_utility_rectangle_define(&childsize, 
		parent->gx_widget_size.gx_rectangle_left + 150, 
		parent->gx_widget_size.gx_rectangle_top + 52,
		parent->gx_widget_size.gx_rectangle_right - 66,
		parent->gx_widget_size.gx_rectangle_bottom - 56);
		//162, 110, 242, 140);
		gx_prompt_create(&hr_bpm_promt, NULL, &hr_data_window, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&hr_bpm_promt, GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&hr_bpm_promt, GX_FONT_ID_SIZE_30);
		string.gx_string_ptr = "BPM";
		string.gx_string_length = strlen(string.gx_string_ptr);	
		gx_prompt_text_set_ext(&hr_bpm_promt, &string);

		
		gx_utility_rectangle_define(&childsize, 
		parent->gx_widget_size.gx_rectangle_left, 
		parent->gx_widget_size.gx_rectangle_top + 102,
		parent->gx_widget_size.gx_rectangle_right,
		parent->gx_widget_size.gx_rectangle_bottom - 10);
		//12, 160, 308, 186);
		gx_prompt_create(&hr_previous_data_promt, NULL, &hr_data_window, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&hr_previous_data_promt, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
		gx_prompt_font_set(&hr_previous_data_promt, GX_FONT_ID_SIZE_26);
		snprintf(pre_data_buf, 20, "%dBPM(1min ago)", previous_hr%1000);	
		string.gx_string_ptr = pre_data_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&hr_previous_data_promt, &string);	
	}
}

//12, 58, 308, 196);
void init_test_data_window(GX_WIDGET *parent)
{
	GX_RECTANGLE childsize;
	GX_STRING string;
	GX_BOOL result;
	gx_widget_created_test(&test_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize,
		parent->gx_widget_size.gx_rectangle_left, 
		parent->gx_widget_size.gx_rectangle_top,
		parent->gx_widget_size.gx_rectangle_right,
		parent->gx_widget_size.gx_rectangle_bottom);
		//  12, 58, 308, 196);
		gx_widget_create(&test_window, "test_window", parent, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&test_window, heart_child_widget_common_event);
		gx_widget_fill_color_set(&test_window, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		
		HEART_DETECTION_WIGET_T *detection_widget = &heart_detection_widget_memory;
		gx_utility_rectangle_define(&childsize, 
		parent->gx_widget_size.gx_rectangle_left + 6, 
		parent->gx_widget_size.gx_rectangle_top + 4,
		parent->gx_widget_size.gx_rectangle_right - 210,
		parent->gx_widget_size.gx_rectangle_bottom - 54);
		//18, 62, 98, 142);
		gx_widget_create(&detection_widget->widget, NULL, &test_window, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_fill_color_set(&detection_widget->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

		gx_sprite_create(&sprite, NULL, &detection_widget->widget, heart_screen_sprite_frame_list,
					6, GX_STYLE_BORDER_NONE|GX_STYLE_TRANSPARENT|GX_STYLE_ENABLED|GX_STYLE_SPRITE_AUTO|GX_STYLE_SPRITE_LOOP,
					GX_ID_NONE, &childsize);		
		gx_utility_rectangle_define(&childsize, 
		parent->gx_widget_size.gx_rectangle_left + 104, 
		parent->gx_widget_size.gx_rectangle_top + 29,
		parent->gx_widget_size.gx_rectangle_right,
		parent->gx_widget_size.gx_rectangle_bottom - 79);
		//116, 87, 308, 117);
		gx_prompt_create(&detection_widget->decs1, NULL, &test_window, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&detection_widget->decs1, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&detection_widget->decs1, GX_FONT_ID_SIZE_30);
		app_heart_summary_list_prompt_get(1, &string);	
		gx_prompt_text_set_ext(&detection_widget->decs1, &string);
		
		gx_utility_rectangle_define(&childsize, 
		parent->gx_widget_size.gx_rectangle_left, 
		parent->gx_widget_size.gx_rectangle_top + 102,
		parent->gx_widget_size.gx_rectangle_right,
		parent->gx_widget_size.gx_rectangle_bottom - 10);
		// 12, 160, 308, 186);
		gx_prompt_create(&pre_data_promt, NULL, &test_window, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&pre_data_promt, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
		gx_prompt_font_set(&pre_data_promt, GX_FONT_ID_SIZE_26);
		snprintf(pre_data_buf, 20, "%dBPM(1min ago)", previous_hr%1000);	
		string.gx_string_ptr = pre_data_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&pre_data_promt, &string);
	}
		
}

void detection_heart_icon_widget_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_HEART_LIST_ROW_T *row = (APP_HEART_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 58, 308, 196);
		gx_widget_create(&row->widget, "detection_heart_widget", list, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&row->widget, heart_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);		
		init_test_data_window(&row->widget);					
	}
}

typedef struct{
	GX_PROMPT  decs0;	
	GX_PROMPT  decs6;
	GX_PROMPT  decs12;
	GX_PROMPT  decs18;
	GX_PROMPT  decs220;
	GX_PROMPT  decs40;
}HEART_BAR_WIGET_T;

HEART_BAR_WIGET_T heart_bar_widget_memory;
void get_24hour_hr_data(AllDaysHr_T *data)
{
	Gui_get_24hour_hr(data);
}

void hear_24hour_bar_draw(GX_WIDGET *widget)
{
	GX_RECTANGLE fillrect;	
	int i;
	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);
	GX_COLOR color;
	gx_context_color_get(GX_COLOR_ID_HONBOW_DARK_GRAY, &color);	

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

	gx_context_brush_width_set(1);
	gx_context_color_get(GX_COLOR_ID_HONBOW_DARK_BLACK, &color);	
	gx_brush_define(&current_context->gx_draw_context_brush, 
								color, color, GX_BRUSH_ALIAS);
	for(i = 0 ; i < 18; i ++) {
		gx_canvas_line_draw (widget->gx_widget_size.gx_rectangle_left  + 69, 
		widget->gx_widget_size.gx_rectangle_top + 3 + 6 *i,		
		widget->gx_widget_size.gx_rectangle_left  + 69, 
		widget->gx_widget_size.gx_rectangle_top + 3 + 3 + i*6);
	}
	for(i = 0 ; i < 18; i ++) {
		gx_canvas_line_draw (widget->gx_widget_size.gx_rectangle_left  + 129, 
		widget->gx_widget_size.gx_rectangle_top + 3 + 6 *i,		
		widget->gx_widget_size.gx_rectangle_left  + 129, 
		widget->gx_widget_size.gx_rectangle_top + 3 + 3 + i*6);
	}
	for(i = 0 ; i < 18; i ++) {
		gx_canvas_line_draw (widget->gx_widget_size.gx_rectangle_left + 189, 
		widget->gx_widget_size.gx_rectangle_top + 3 + 6 *i,		
		widget->gx_widget_size.gx_rectangle_left  + 189, 
		widget->gx_widget_size.gx_rectangle_top + 3 + 3 + i*6);
	}
	for(i = 0 ; i < 18; i ++) {
		gx_canvas_line_draw (widget->gx_widget_size.gx_rectangle_left  + 250, 
		widget->gx_widget_size.gx_rectangle_top + 3 + 6 *i,		
		widget->gx_widget_size.gx_rectangle_left  + 250, 
		widget->gx_widget_size.gx_rectangle_top + 3 + 3 + i*6);
	}	
	gx_context_brush_width_set(4);
	gx_context_color_get(GX_COLOR_ID_HONBOW_DARK_GRAY, &color);	
	gx_brush_define(&current_context->gx_draw_context_brush, 
								color, color, 0);
	for (i = 0; i < 24; i++) {	
		gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + 16 + i * 10,  
		widget->gx_widget_size.gx_rectangle_top + 7 , 
		widget->gx_widget_size.gx_rectangle_left  + 16 + i * 10, 
		widget->gx_widget_size.gx_rectangle_top + 67);	
	}

	gx_context_color_get(GX_COLOR_ID_HONBOW_RED, &color);	
	gx_brush_define(&current_context->gx_draw_context_brush, 
								color, color, 0);
	for (i = 0; i < 24; i++) {
		if (((day_hr.hour_hr[i].max_hr >= 40) && (day_hr.hour_hr[i].max_hr <= 220)) &&  
			((day_hr.hour_hr[i].min_hr >= 40) && (day_hr.hour_hr[i].min_hr <= 220))){
		gx_canvas_line_draw(
		widget->gx_widget_size.gx_rectangle_left  + 16 + i * 10, 
		(220 - day_hr.hour_hr[i].max_hr)/3 + widget->gx_widget_size.gx_rectangle_top + 7, 
		widget->gx_widget_size.gx_rectangle_left  + 16 + i * 10,  
		widget->gx_widget_size.gx_rectangle_top + 67 - (day_hr.hour_hr[i].min_hr - 40)/3);
		}
				
	}
	gx_widget_children_draw(widget);

}


void hear_24hour_bar_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_HEART_LIST_ROW_T *row = (APP_HEART_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 196, 308, 303);
		gx_widget_create(&row->widget, "detection_heart_bar", list, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&row->widget, heart_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_draw_set(&row->widget, hear_24hour_bar_draw);
		
		HEART_BAR_WIGET_T *heart_bar_widget = &heart_bar_widget_memory;	
		gx_utility_rectangle_define(&childsize, 23, 282, 36, 302);
		gx_prompt_create(&heart_bar_widget->decs0, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&heart_bar_widget->decs0, GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&heart_bar_widget->decs0, GX_FONT_ID_SIZE_20);	
		string.gx_string_ptr = "0";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&heart_bar_widget->decs0, &string);

		gx_utility_rectangle_define(&childsize, 85, 282, 108, 302);
		gx_prompt_create(&heart_bar_widget->decs6, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&heart_bar_widget->decs6, GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&heart_bar_widget->decs6, GX_FONT_ID_SIZE_20);	
		string.gx_string_ptr = "6";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&heart_bar_widget->decs6, &string);

		gx_utility_rectangle_define(&childsize, 143, 282, 164, 302);
		gx_prompt_create(&heart_bar_widget->decs12, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&heart_bar_widget->decs12, GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&heart_bar_widget->decs12, GX_FONT_ID_SIZE_20);	
		string.gx_string_ptr = "12";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&heart_bar_widget->decs12, &string);

		gx_utility_rectangle_define(&childsize, 206, 282, 227, 302);
		gx_prompt_create(&heart_bar_widget->decs18, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&heart_bar_widget->decs18, GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&heart_bar_widget->decs18, GX_FONT_ID_SIZE_20);	
		string.gx_string_ptr = "18";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&heart_bar_widget->decs18, &string);

		gx_utility_rectangle_define(&childsize, 269, 263, 294, 283);
		gx_prompt_create(&heart_bar_widget->decs40, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&heart_bar_widget->decs40, GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&heart_bar_widget->decs40, GX_FONT_ID_SIZE_20);	
		string.gx_string_ptr = "40";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&heart_bar_widget->decs40, &string);

		gx_utility_rectangle_define(&childsize, 269, 198, 305, 218);
		gx_prompt_create(&heart_bar_widget->decs220, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&heart_bar_widget->decs220, GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&heart_bar_widget->decs220, GX_FONT_ID_SIZE_20);	
		string.gx_string_ptr = "220";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&heart_bar_widget->decs220, &string);
	}
}
MaxMinHr_Widget_T max_hr_widget_memory, min_hr_widget_memory;

void max_min_info_widget_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_HEART_LIST_ROW_T *row = (APP_HEART_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;

	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 303, 308, 361);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
			GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&row->widget, heart_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);

		MaxMinHr_Widget_T *max_hr_widget = &max_hr_widget_memory;

		gx_utility_rectangle_define(&childsize, 40, 313, 70, 343);
		gx_widget_create(&max_hr_widget->hr_widget, NULL, &row->widget, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);		
		gx_widget_fill_color_set(&max_hr_widget->hr_widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
		max_hr_widget->icon = GX_PIXELMAP_ID_APP_HEART_IC_UP;
		gx_widget_draw_set(&max_hr_widget->hr_widget, heart_icon_widget_draw);
		gx_utility_rectangle_define(&childsize, 80, 313, 132, 343);
		gx_prompt_create(&max_promt, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
						
		gx_prompt_text_color_set(&max_promt, GX_COLOR_ID_HONBOW_RED, 
			GX_COLOR_ID_HONBOW_RED, GX_COLOR_ID_HONBOW_RED);	
		gx_prompt_font_set(&max_promt, GX_FONT_ID_SIZE_30);
		snprintf(max_buf, 5, "%s", "--");
		string.gx_string_ptr = max_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&max_promt, &string);

		MaxMinHr_Widget_T *min_hr_widget = &min_hr_widget_memory;
		gx_utility_rectangle_define(&childsize, 190, 313, 220, 343);
		gx_widget_create(&min_hr_widget->hr_widget, NULL, &row->widget, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);		
		gx_widget_fill_color_set(&min_hr_widget->hr_widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
		min_hr_widget->icon = GX_PIXELMAP_ID_APP_HEART_IC_DOWN;
		gx_widget_draw_set(&min_hr_widget->hr_widget, heart_icon_widget_draw);

		gx_utility_rectangle_define(&childsize, 230, 313, 282, 343);
		gx_prompt_create(&min_promt, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&min_promt, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&min_promt, GX_FONT_ID_SIZE_30);
		snprintf(min_buf, 5, "%s", "--");
		string.gx_string_ptr = min_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&min_promt, &string);
	}
}

typedef struct {
	GX_PROMPT title;
	GX_PROMPT util;
}RESET_PROMT_WIDGET_T;



RESET_PROMT_WIDGET_T resting_promt_memory;

void resting_hr_widget_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_HEART_LIST_ROW_T *row = (APP_HEART_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 361, 308, 450);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								GX_COLOR_ID_HONBOW_BLACK);

		RESET_PROMT_WIDGET_T *resting = &resting_promt_memory;		
		gx_utility_rectangle_define(&childsize, 12, 361, 308, 387);		
		gx_prompt_create(&resting->title, NULL, &row->widget, 0,
			GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
			0, &childsize);
		gx_prompt_text_color_set(&resting->title, GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK,GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&resting->title, GX_FONT_ID_SIZE_26);
		app_heart_summary_list_prompt_get(2, &string);	
		gx_prompt_text_set_ext(&resting->title, &string);
		
		if (!resting_hr) {
			snprintf(resting_buf, 5, "%s", "--");
		}else {
			snprintf(resting_buf, 5, "%3d", resting_hr);
		}		
		gx_utility_rectangle_define(&childsize, 15, 397, 95, 443);	
		gx_prompt_create(&resting_prompt, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&resting_prompt, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&resting_prompt, GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = resting_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&resting_prompt, &string);		
				
		gx_utility_rectangle_define(&childsize, 96, 407, 308, 437);		
		gx_prompt_create(&resting->util, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&resting->util, GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK,GX_COLOR_ID_HONBOW_DARK_BLACK);
		gx_prompt_font_set(&resting->util, GX_FONT_ID_SIZE_30);
		string.gx_string_ptr = "BPM";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&resting->util, &string);
	}
}


void heart_list_elempent_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_BOOL result;
	APP_HEART_LIST_ROW_T *row = CONTAINER_OF(widget, APP_HEART_LIST_ROW_T, widget);
	gx_widget_created_test(&row->widget, &result);
	if(!result) {
		switch (index) {
		case 0:
			detection_heart_icon_widget_create(list, widget, index);
		break;
		case 1:
			hear_24hour_bar_create(list, widget, index);
		break;
		case 2:
			max_min_info_widget_create(list, widget, index);
		break;
		case 3:
			resting_hr_widget_create(list, widget, index);
		break;
	
		default:
		break;
		}			
	}
}

static VOID app_heart_list_child_creat(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < 4; count++) {
		heart_list_elempent_create(list, 
			(GX_WIDGET *)&app_heart_list_row_memory[count], count);
	}
}

void sport_heart_list_create(GX_WIDGET *widget)
{
	GX_RECTANGLE child_size;
	gx_utility_rectangle_define(&child_size, 0, 58, 319, 359);
	gx_vertical_list_create(&app_heart_list, NULL, widget, 3, NULL,
		GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child_size);
	gx_widget_fill_color_set(&app_heart_list, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
	app_heart_list_child_creat(&app_heart_list);
}


void app_please_wear_window_init(GX_WINDOW *window);
void app_heart_window_init(GX_WINDOW *window)
{
	gx_widget_create(&heart_main_page, "heart_main_page", window, 
					GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
					&window->gx_widget_size);
	gx_widget_event_process_set(&heart_main_page, app_heart_main_page_event);
	GX_WIDGET *parent = &heart_main_page;

	custom_window_title_create(parent, NULL, &heart_prompt, &time_prompt);
	GX_STRING string;
	string.gx_string_ptr = get_language_heart_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&heart_prompt, &string);

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	time_t now = ts.tv_sec;
	struct tm *tm_now = gmtime(&now);
	snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
	time_prompt_buff[7] = 0;
	string.gx_string_ptr = time_prompt_buff;
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&time_prompt, &string);

	sport_heart_list_create(parent);

	app_please_wear_window_init(NULL);
}

GUI_APP_DEFINE_WITH_INIT(today, APP_ID_03_HEARTRATE, (GX_WIDGET *)&app_heart_window,
	GX_PIXELMAP_ID_APP_LIST_ICON_HEART_90_90, GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY,
	app_heart_window_init);