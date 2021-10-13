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

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

GX_WIDGET spo2_measuring_window;
static GX_WIDGET animation_window;
static GX_PROMPT stay_prompt;

static GX_PROMPT spo2_prompt;
static GX_PROMPT time_prompt;
static GX_PROMPT count_down_promt;
static char count_down_data_buf[3];
static char count_down_sec = 15;
static GX_PROMPT sec_util_promt;

extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];

static const GX_CHAR *lauguage_en_sport_summary_element[] = { NULL, "Stay still", "Try to stay still"};
static const GX_CHAR **spo2_stay_string[] = {
	[LANGUAGE_ENGLISH] = lauguage_en_sport_summary_element
};

static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return spo2_stay_string[id];
}

void app_spo2_stay_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 3

UINT spo2_measuring_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{	
	GX_STRING string;
	bool has_motion = false;
	static bool pre_motion = false;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {		
		gx_system_timer_start(window, TIME_FRESH_TIMER_ID, 50, 50);	
		count_down_sec = 15;	
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
	case GX_EVENT_KEY_DOWN:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			return 0;
		}
		break;
	// case GX_EVENT_KEY_UP:
	// 	if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
	// 		windows_mgr_page_jump((GX_WINDOW *)&app_spo2_window, NULL, WINDOWS_OP_BACK);
	// 		printf("spo2_measuring_window_event GX_EVENT_KEY_UP\n");		
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

			snprintf(count_down_data_buf, 3, "%d", count_down_sec%60);	
			string.gx_string_ptr = count_down_data_buf;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&count_down_promt, &string);			
			if(count_down_sec == 0) {
				jump_spo2_page(&measuring_fail_window, &spo2_measuring_window, SCREEN_SLIDE_RIGHT);
				gx_widget_delete(&spo2_measuring_window);
			}
			count_down_sec--;
			

			has_motion = watch_has_motion();
			if (has_motion) {
				pre_motion = has_motion;
				app_spo2_stay_prompt_get(2, &string);	
				gx_prompt_text_set_ext(&stay_prompt, &string);
			} else {
				if (pre_motion) {
					pre_motion = false;
				} else {
					app_spo2_stay_prompt_get(1, &string);	
					gx_prompt_text_set_ext(&stay_prompt, &string);
				}				
			}
			uint8_t spo2_data = Gui_get_spo2();
			if (spo2_data) {
				app_spo2_result_window_init(NULL, spo2_data);
				jump_spo2_page(&spo2_result_window, &spo2_measuring_window, SCREEN_SLIDE_RIGHT);
				gx_widget_delete(&spo2_measuring_window);
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

static GX_SPRITE spo2_measuring_sprite;
GX_SPRITE_FRAME spo2_measurint_sprite_frame_list[] =
{
    {
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_1,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        20,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_2,   /* pixelmap id                    */
        0,                                   /* x offset                       */
        0,                                   /* y offset                       */
        20,                                 /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_3,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        20,                                 /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_4,   /* pixelmap id                    */
        0,                                   /* x offset                       */
        0,                                   /* y offset                       */
        20,                                 /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_5,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        20,                                 /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_6,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        20,                                 /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
	{
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_7,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        20,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_8,   /* pixelmap id                    */
        0,                                   /* x offset                       */
        0,                                   /* y offset                       */
        20,                                 /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_9,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        20,                                 /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_10,   /* pixelmap id                    */
        0,                                   /* x offset                       */
        0,                                   /* y offset                       */
       	20,                                 /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_11,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        20,                                 /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
    {
        GX_PIXELMAP_ID_APP_SPO2_ANIMATION_12,   /* pixelmap id                    */
        0,                                  /* x offset                       */
        0,                                  /* y offset                       */
        5,                                  /* frame delay                    */
        GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
        255                                  /* alpha value                    */
    },
	{
		GX_PIXELMAP_ID_APP_SPO2_ANIMATION_13,   /* pixelmap id                    */
		0,                                  /* x offset                       */
		0,                                  /* y offset                       */
		20,                                 /* frame delay                    */
		GX_SPRITE_BACKGROUND_NO_ACTION,      /* background operation           */
		255                                  /* alpha value                    */
    }
};

void app_spo2_animation_widget_create(GX_WIDGET *parent)
{
	GX_RECTANGLE childsize;
	GX_STRING string;
	GX_BOOL result;
	gx_widget_created_test(&animation_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize,
		parent->gx_widget_size.gx_rectangle_left + 118, 
		parent->gx_widget_size.gx_rectangle_top + 67,
		parent->gx_widget_size.gx_rectangle_left + 118 + 83,
		parent->gx_widget_size.gx_rectangle_top + 197);
		//  12, 58, 308, 196);
		gx_widget_create(&animation_window, "animation_window", parent, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&animation_window, spo2_child_widget_common_event);
		gx_widget_fill_color_set(&animation_window, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);		
	
		gx_sprite_create(&spo2_measuring_sprite, NULL, &animation_window, spo2_measurint_sprite_frame_list,
					6, GX_STYLE_BORDER_NONE|GX_STYLE_TRANSPARENT|GX_STYLE_ENABLED|GX_STYLE_SPRITE_AUTO|GX_STYLE_SPRITE_LOOP,
					GX_ID_NONE, &childsize);

		gx_utility_rectangle_define(&childsize, 
		parent->gx_widget_size.gx_rectangle_left + 12, 
		parent->gx_widget_size.gx_rectangle_top + 197,
		parent->gx_widget_size.gx_rectangle_left + 308,
		parent->gx_widget_size.gx_rectangle_top + 197 + 30);
		//116, 87, 308, 117);
		gx_prompt_create(&stay_prompt, NULL, parent, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&stay_prompt, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&stay_prompt, GX_FONT_ID_SIZE_30);
		app_spo2_stay_prompt_get(1, &string);	
		gx_prompt_text_set_ext(&stay_prompt, &string);
		
		gx_utility_rectangle_define(&childsize, 
		parent->gx_widget_size.gx_rectangle_left + 126, 
		parent->gx_widget_size.gx_rectangle_top + 253,
		parent->gx_widget_size.gx_rectangle_left + 126 + 69,
		parent->gx_widget_size.gx_rectangle_top + 253 + 60);
		// 12, 160, 308, 186);
		gx_prompt_create(&count_down_promt, NULL, parent, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&count_down_promt, GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&count_down_promt, GX_FONT_ID_SIZE_60);
		count_down_sec = 15;
		snprintf(count_down_data_buf, 3, "%d", count_down_sec%60);	
		string.gx_string_ptr = count_down_data_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&count_down_promt, &string);

		gx_utility_rectangle_define(&childsize, 
		parent->gx_widget_size.gx_rectangle_left + 199, 
		parent->gx_widget_size.gx_rectangle_top + 279,
		parent->gx_widget_size.gx_rectangle_left + 199 + 16,
		parent->gx_widget_size.gx_rectangle_top + 279 + 30);
		gx_prompt_create(&sec_util_promt, NULL, parent, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&sec_util_promt, GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&sec_util_promt, GX_FONT_ID_SIZE_30);		
		string.gx_string_ptr = "s";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&sec_util_promt, &string);
	}
		
}


void app_spo2_measuring_window_init(GX_WINDOW *window)
{
	GX_RECTANGLE child_size;
	GX_BOOL result;
	gx_widget_created_test(&spo2_measuring_window, &result);
	if (!result) {

		gx_utility_rectangle_define(&child_size, 0, 0, 319, 359);
		gx_widget_create(&spo2_measuring_window, "spo2_measuring_wind", window, 
						GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
						&child_size);
		
		GX_WIDGET *parent = &spo2_measuring_window;

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

		app_spo2_animation_widget_create(parent);
		gx_widget_event_process_set(&spo2_measuring_window, spo2_measuring_window_event);
	}
	

}

