#include "gx_api.h"
#include "timer_widget.h"
#include "custom_widget_scrollable.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include "custom_window_title.h"
#include "app_timer_language.h"
#include "stdio.h"
#include "windows_manager.h"
#include "guix_simple_specifications.h"
#include "custom_rounded_button.h"
#include "guix_simple_resources.h"
#include "app_timer.h"
#include "custom_rounded_button.h"

typedef enum {
	TIMER_STATUS_RUNNING = 0,
	TIMER_STATUS_PAUSE = 1,
	TIMER_STATUS_COMPLETE = 2,
} TIMER_STATUS_T;

static GX_BOOL jump_from_ext = GX_FALSE;
static GX_WIDGET *page_record = GX_NULL;
static TIMER_PARA_TRANS_T para_dst;
static TIMER_PARA_TRANS_T para_now;
static TIMER_STATUS_T timer_status = TIMER_STATUS_COMPLETE;
static GX_WIDGET curr_widget;
static GX_PROMPT app_prompt;
static GX_PROMPT time_prompt;
static char time_prompt_buff[8];
static GX_PROMPT timer_prompt;
static char timer_prompt_buff[16];

static Custom_RoundedButton_TypeDef cancel_resume_button;
static Custom_RoundedButton_TypeDef pause_start_button;
#define CANCEL_RESUME_BUTTON_ID 1
#define PAUSE_START_BUTTON_ID 2

static GX_BOOL time_dec_one_sec(TIMER_PARA_TRANS_T *para)
{
	if (para->tenth_of_second >= 10) {
		para->tenth_of_second = 9;
	}
	if (para->tenth_of_second > 0) {
		para->tenth_of_second--;
	} else {
		if (para->sec > 0) {
			para->sec--;
			para->tenth_of_second = 9;
		} else if (para->sec == 0) {
			if (para->min > 0) {
				para->min--;
				para->sec = 59;
				para->tenth_of_second = 9;
			} else {
				if (para->hour > 0) {
					para->hour--;
					para->min = 59;
					para->sec = 59;
					para->tenth_of_second = 9;
				} else {
					return GX_FALSE;
				}
			}
		}
	}
	return GX_TRUE;
}

#define CLOCK_TIMER_ID 1
#define TIMER_FOR_APP_ID 2
static UINT timer_page_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT timer_id;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		gx_system_timer_start(widget, CLOCK_TIMER_ID, 50, 50);
		GX_STRING value;
		if (timer_status != TIMER_STATUS_COMPLETE) {
			if (para_now.hour > 0) {
				snprintf(timer_prompt_buff, sizeof(timer_prompt_buff), "%02d:%02d:%02d.%d", para_now.hour, para_now.min,
						 para_now.sec, para_now.tenth_of_second);
			} else {
				snprintf(timer_prompt_buff, sizeof(timer_prompt_buff), "%02d:%02d.%d", para_now.min, para_now.sec,
						 para_now.tenth_of_second);
			}
		} else {
			snprintf(timer_prompt_buff, sizeof(timer_prompt_buff), "complete");
		}
		value.gx_string_ptr = timer_prompt_buff;
		value.gx_string_length = strlen(value.gx_string_ptr);
		gx_prompt_text_set_ext(&timer_prompt, &value);
		gx_system_dirty_mark(&timer_prompt);
		break;
	}
	case GX_EVENT_HIDE: {
		gx_system_timer_stop(widget, CLOCK_TIMER_ID);
		break;
	}
	case GX_EVENT_TIMER:
		timer_id = event_ptr->gx_event_payload.gx_event_timer_id;
		if (((timer_id == CLOCK_TIMER_ID) || (timer_id == TIMER_FOR_APP_ID)) &&
			(event_ptr->gx_event_target == widget)) {
			if (timer_id == CLOCK_TIMER_ID) {
				struct timespec ts;
				clock_gettime(CLOCK_REALTIME, &ts);
				time_t now = ts.tv_sec;
				struct tm *tm_now = gmtime(&now);
				static int hour_old, min_old;
				if ((hour_old != tm_now->tm_hour) || (min_old != tm_now->tm_min)) {
					snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
					time_prompt_buff[7] = 0;
					GX_STRING string;
					string.gx_string_ptr = time_prompt_buff;
					string.gx_string_length = strlen(string.gx_string_ptr);
					gx_prompt_text_set_ext(&time_prompt, &string);
					hour_old = tm_now->tm_hour;
					min_old = tm_now->tm_min;
				}
			} else if (timer_id == TIMER_FOR_APP_ID) {
				if (timer_status == TIMER_STATUS_RUNNING) {
					GX_STRING value;
					time_dec_one_sec(&para_now);
					if ((para_now.hour == 0) && (para_now.min == 0) && (para_now.sec == 0) &&
						(para_now.tenth_of_second == 0)) {
						timer_status = TIMER_STATUS_COMPLETE;
						cancel_resume_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_S_REFRESH;
						pause_start_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_S_OK;
						gx_system_dirty_mark(&cancel_resume_button);
						gx_system_dirty_mark(&pause_start_button);
						value.gx_string_ptr = "complete";
						value.gx_string_length = strlen(value.gx_string_ptr);
						gx_prompt_text_set_ext(&timer_prompt, &value);
						gx_system_dirty_mark(&timer_prompt);
						if (!(curr_widget.gx_widget_status & GX_STATUS_VISIBLE)) {
							GX_WIDGET *parent = (GX_WIDGET *)&app_timer_window;
							GX_WIDGET *child = parent->gx_widget_first_child;
							while (child) {
								gx_widget_detach(child);
								child = parent->gx_widget_first_child;
							}
							gx_widget_resize(&curr_widget, &parent->gx_widget_size);
							gx_widget_attach(parent, &curr_widget);
							if (!(app_timer_window.gx_widget_status & GX_STATUS_VISIBLE)) {
								windows_mgr_page_jump((GX_WINDOW *)root_window.gx_widget_first_child,
													  (GX_WINDOW *)&app_timer_window, WINDOWS_OP_SWITCH_NEW);
							}
						}
					} else {
						if (para_now.hour > 0) {
							snprintf(timer_prompt_buff, sizeof(timer_prompt_buff), "%02d:%02d:%02d.%d", para_now.hour,
									 para_now.min, para_now.sec, para_now.tenth_of_second);
						} else {
							snprintf(timer_prompt_buff, sizeof(timer_prompt_buff), "%02d:%02d.%d", para_now.min,
									 para_now.sec, para_now.tenth_of_second);
						}
						value.gx_string_ptr = timer_prompt_buff;
						value.gx_string_length = strlen(value.gx_string_ptr);
						gx_prompt_text_set_ext(&timer_prompt, &value);
						gx_system_dirty_mark(&timer_prompt);
					}
				} else {
					gx_system_timer_stop(widget, TIMER_FOR_APP_ID);
				}
			}
			return 0;
		}
		break;
	case GX_SIGNAL(CANCEL_RESUME_BUTTON_ID, GX_EVENT_CLICKED):
		if (timer_status != TIMER_STATUS_COMPLETE) {
			timer_status = TIMER_STATUS_COMPLETE;
			cancel_resume_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_S_REFRESH;
			pause_start_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_S_OK;
			gx_system_dirty_mark(&cancel_resume_button);
			gx_system_dirty_mark(&pause_start_button);
			if (jump_from_ext == GX_FALSE) {
				app_timer_page_jump_internal(&curr_widget, APP_TIMER_PAGE_JUMP_BACK);
			} else {
				windows_mgr_page_jump((GX_WINDOW *)&app_timer_window, NULL, WINDOWS_OP_BACK);
				jump_from_ext = GX_FALSE;
			}
		} else {
			cancel_resume_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_S_CANCEL;
			pause_start_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_S_PAUSE;
			gx_system_dirty_mark(&cancel_resume_button);
			gx_system_dirty_mark(&pause_start_button);
			para_now = para_dst;
			timer_status = TIMER_STATUS_RUNNING;
			if (para_now.hour > 0) {
				snprintf(timer_prompt_buff, sizeof(timer_prompt_buff), "%02d:%02d:%02d", para_now.hour, para_now.min,
						 para_now.sec);
			} else {
				snprintf(timer_prompt_buff, sizeof(timer_prompt_buff), "%02d:%02d", para_now.min, para_now.sec);
			}
			GX_STRING value;
			value.gx_string_ptr = timer_prompt_buff;
			value.gx_string_length = strlen(value.gx_string_ptr);
			gx_prompt_text_set_ext(&timer_prompt, &value);
			gx_system_dirty_mark(&timer_prompt);
			gx_system_timer_start(&curr_widget, TIMER_FOR_APP_ID, 5, 5);
		}
		break;
	case GX_SIGNAL(PAUSE_START_BUTTON_ID, GX_EVENT_CLICKED):
		if (timer_status == TIMER_STATUS_RUNNING) {
			timer_status = TIMER_STATUS_PAUSE;
			pause_start_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_L_START;
		} else if (timer_status == TIMER_STATUS_PAUSE) {
			timer_status = TIMER_STATUS_RUNNING;
			pause_start_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_S_PAUSE;
			gx_system_timer_start(&curr_widget, TIMER_FOR_APP_ID, 5, 5);
		} else {
			if (jump_from_ext == GX_FALSE) {
				app_timer_page_jump_internal(&curr_widget, APP_TIMER_PAGE_JUMP_BACK);
			} else {
				windows_mgr_page_jump((GX_WINDOW *)&app_timer_window, NULL, WINDOWS_OP_BACK);
				jump_from_ext = GX_FALSE;
			}
		}
		gx_system_dirty_mark(&pause_start_button);
		break;
#if 0
	case GX_EVENT_PEN_DOWN:
		wm_point_down_record(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata);
		break;
	case GX_EVENT_PEN_UP:
		if (wm_quit_judge(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata) == GX_TRUE) {
		}
		break;
#endif
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

// TODO: NEED implement!!!!!!!!!!!!!!!!
void app_timer_main_page_show(GX_BOOL ext_flag, GX_WINDOW *curr, GX_WIDGET *curr_child_page, TIMER_PARA_TRANS_T para)
{
	jump_from_ext = ext_flag;
	para_dst = para;
	para_now = para;

	timer_status = TIMER_STATUS_RUNNING;
	cancel_resume_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_S_CANCEL;
	pause_start_button.icon = GX_PIXELMAP_ID_APP_TIMER_BTN_S_PAUSE;

	if (ext_flag == GX_TRUE) {
		GX_WIDGET *parent = (GX_WIDGET *)&app_timer_window;
		GX_WIDGET *child = parent->gx_widget_first_child;

		while (child) {
			gx_widget_detach(child);
			child = parent->gx_widget_first_child;
		}
		gx_widget_resize(&curr_widget, &parent->gx_widget_size);
		gx_widget_attach(parent, &curr_widget);

		windows_mgr_page_jump(curr, (GX_WINDOW *)&app_timer_window, WINDOWS_OP_SWITCH_NEW);
	} else {
		page_record = curr_child_page;
		app_timer_page_jump_internal(curr_child_page, APP_TIMER_PAGE_JUMP_MAIN);
	}

	gx_system_timer_start(&curr_widget, TIMER_FOR_APP_ID, 5, 5);
}

void app_timer_main_page_init(void)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_widget, NULL, NULL, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_widget_event_process_set(&curr_widget, timer_page_event_process);

	GX_WIDGET *parent = &curr_widget;
	custom_window_title_create(parent, NULL, &app_prompt, &time_prompt);
	GX_STRING string;
	string.gx_string_ptr = app_timer_get_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&app_prompt, &string);

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	time_t now = ts.tv_sec;
	struct tm *tm_now = gmtime(&now);
	snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
	time_prompt_buff[7] = 0;

	string.gx_string_ptr = time_prompt_buff;
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&time_prompt, &string);

	gx_utility_rectangle_define(&childSize, 0, 130, 319, 130 + 60 - 1);
	gx_prompt_create(&timer_prompt, NULL, parent, GX_ID_NONE,
					 GX_STYLE_ENABLED | GX_STYLE_TEXT_CENTER | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_prompt_font_set(&timer_prompt, GX_FONT_ID_SIZE_60);
	gx_prompt_text_color_set(&timer_prompt, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	string.gx_string_ptr = "00:00";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&timer_prompt, &string);

	gx_utility_rectangle_define(&childSize, 12, 282, 12 + 143 - 1, 282 + 65 - 1);
	custom_rounded_button_create(&cancel_resume_button, CANCEL_RESUME_BUTTON_ID, parent, &childSize,
								 GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_ID_APP_TIMER_BTN_S_CANCEL, NULL,
								 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	gx_utility_rectangle_define(&childSize, 165, 282, 165 + 143 - 1, 282 + 65 - 1);
	custom_rounded_button_create(&pause_start_button, PAUSE_START_BUTTON_ID, parent, &childSize, GX_COLOR_ID_HONBOW_RED,
								 GX_PIXELMAP_ID_APP_TIMER_BTN_S_PAUSE, NULL, CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	app_timer_page_reg(APP_TIMER_PAGE_MAIN, &curr_widget);
}