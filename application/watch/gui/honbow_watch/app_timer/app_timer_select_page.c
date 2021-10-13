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
#include "logging/log.h"
LOG_MODULE_DECLARE(guix_log);

static GX_WIDGET curr_widget;
static GX_PROMPT app_prompt;
static GX_PROMPT time_prompt;
static char time_prompt_buff[8];
static CUSTOM_SCROLLABLE_WIDGET scrollable_timer_page;
#define BUTTON_ID_TIMER_FIRST 1
#define BUTTON_ID_TIMER_SECOND 2
#define BUTTON_ID_TIMER_THIRD 3
#define BUTTON_ID_TIMER_FORTH 4
#define BUTTON_ID_TIMER_FIFTH 5
#define BUTTON_ID_TIMER_SIXTH 6
#define BUTTON_ID_TIMER_SEVENTH 7
#define BUTTON_ID_TIMER_EIGHTH 8

const static TIMER_PARA_TRANS_T timer_info[8] = {
	[0] = {.hour = 0, .min = 1, .sec = 0, .tenth_of_second = 0},
	[1] = {.hour = 0, .min = 3, .sec = 0, .tenth_of_second = 0},
	[2] = {.hour = 0, .min = 5, .sec = 0, .tenth_of_second = 0},
	[3] = {.hour = 0, .min = 10, .sec = 0, .tenth_of_second = 0},
	[4] = {.hour = 0, .min = 15, .sec = 0, .tenth_of_second = 0},
	[5] = {.hour = 0, .min = 30, .sec = 0, .tenth_of_second = 0},
	[6] = {.hour = 1, .min = 0, .sec = 0, .tenth_of_second = 0},
	[7] = {.hour = 2, .min = 0, .sec = 0, .tenth_of_second = 0},
};
static TIMER_WIDGET_TYPE child_timer_widgets[8] = {
	[0] =
		{
			.button_id = BUTTON_ID_TIMER_FIRST,
			.counter_value = "1",
			.unit_value = "min",
		},
	[1] =
		{
			.button_id = BUTTON_ID_TIMER_SECOND,
			.counter_value = "3",
			.unit_value = "min",
		},
	[2] =
		{
			.button_id = BUTTON_ID_TIMER_THIRD,
			.counter_value = "5",
			.unit_value = "min",
		},
	[3] =
		{
			.button_id = BUTTON_ID_TIMER_FORTH,
			.counter_value = "10",
			.unit_value = "min",
		},
	[4] =
		{
			.button_id = BUTTON_ID_TIMER_FIFTH,
			.counter_value = "15",
			.unit_value = "min",
		},
	[5] =
		{
			.button_id = BUTTON_ID_TIMER_SIXTH,
			.counter_value = "30",
			.unit_value = "min",
		},
	[6] =
		{
			.button_id = BUTTON_ID_TIMER_SEVENTH,
			.counter_value = "1",
			.unit_value = "hour",
		},
	[7] =
		{
			.button_id = BUTTON_ID_TIMER_EIGHTH,
			.counter_value = "2",
			.unit_value = "hour",
		},
};
typedef struct {
	Custom_RoundedButton_TypeDef button;
} TIMER_CUSTOM_BUTTON_T;

typedef struct {
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t tenth_of_second;
	GX_BOOL valid_flag;
} TIMER_CUSTOM_T;

static TIMER_CUSTOM_BUTTON_T timer_custom_widget[3];
static TIMER_CUSTOM_T timer_custom[3] = {
	[0] = {.valid_flag = GX_FALSE},
	[1] = {.valid_flag = GX_FALSE},
	[2] = {.valid_flag = GX_FALSE},
};
static GX_CHAR timer_custom_button_buff[3][12];

#define BUTTON_ID_TIMER_CUSTOM_1 9
#define BUTTON_ID_TIMER_CUSTOM_2 10
#define BUTTON_ID_TIMER_CUSTOM_3 11
#define BUTTON_ID_TIMER_ADD 12

static Custom_RoundedButton_TypeDef timer_add_button;
extern void timer_widget_create_helper(TIMER_WIDGET_TYPE *child, GX_RECTANGLE *rect, GX_WIDGET *parent);

void app_timer_custom_add(uint8_t hour, uint8_t min, uint8_t sec)
{
	uint8_t i = 0;
	for (i = 0; i < sizeof(timer_custom) / sizeof(TIMER_CUSTOM_T); i++) {
		if (timer_custom[i].valid_flag == GX_FALSE) {
			timer_custom[i].valid_flag = GX_TRUE;
			timer_custom[i].hour = hour;
			timer_custom[i].min = min;
			timer_custom[i].sec = sec;
			break;
		}
	}
	if (i >= 3) {
		timer_custom[0] = timer_custom[1];
		timer_custom[1] = timer_custom[2];
		timer_custom[2].hour = hour;
		timer_custom[2].min = min;
		timer_custom[2].sec = sec;
		timer_custom[2].tenth_of_second = 0;
		timer_custom[2].valid_flag = GX_TRUE;
	}
}
void app_timer_children_repos(void)
{
	GX_RECTANGLE childSize;
	GX_STRING init_value;

	GX_VALUE left = scrollable_timer_page.widget.gx_widget_size.gx_rectangle_left;
	GX_VALUE top = child_timer_widgets[0].counter_down_widget.gx_widget_size.gx_rectangle_top;

	for (uint8_t i = 0; i < sizeof(timer_custom) / sizeof(TIMER_CUSTOM_T); i++) {
		gx_widget_detach(&timer_custom_widget[i].button);
	}
	gx_widget_detach(&timer_add_button);

	uint8_t custom_child_cnt = 0;
	for (uint8_t i = 0; i < sizeof(timer_custom) / sizeof(TIMER_CUSTOM_T); i++) {
		if (timer_custom[i].valid_flag == GX_TRUE) {
			custom_child_cnt++;
			gx_utility_rectangle_define(&childSize, left + 12, top + 600 + i * (65 + 12), left + 12 + 296 - 1,
										top + 600 + i * (65 + 12) + 65 - 1);
			snprintf(timer_custom_button_buff[i], sizeof(timer_custom_button_buff[i]), "%02d:%02d:%02d",
					 timer_custom[i].hour, timer_custom[i].min, timer_custom[i].sec);
			init_value.gx_string_ptr = timer_custom_button_buff[i];
			init_value.gx_string_length = strlen(init_value.gx_string_ptr);
			gx_prompt_text_set_ext(&timer_custom_widget[i].button.desc, &init_value);
			gx_widget_resize(&timer_custom_widget[i].button, &childSize);
			gx_widget_attach(&scrollable_timer_page, &timer_custom_widget[i].button);
		}
	}

	gx_utility_rectangle_define(&childSize, left + 12, top + 600 + custom_child_cnt * (65 + 12), left + 12 + 296 - 1,
								top + 600 + custom_child_cnt * (65 + 12) + 65 - 1);
	gx_widget_attach(&scrollable_timer_page, &timer_add_button);
	gx_widget_resize(&timer_add_button, &childSize);
}

#define CLOCK_TIMER_ID 1
static UINT timer_main_page_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static WM_QUIT_JUDGE_T pen_info_judge;

	if ((event_ptr->gx_event_type & 0x00ff) == GX_EVENT_CLICKED) {
		uint8_t key_id = (event_ptr->gx_event_type & 0xff00) >> 8;
		LOG_INF("key pressed, key id = %d", key_id);
		if ((key_id >= BUTTON_ID_TIMER_FIRST) && (key_id <= BUTTON_ID_TIMER_EIGHTH)) {
			app_timer_main_page_show(GX_FALSE, (GX_WINDOW *)&app_timer_window, &curr_widget, timer_info[key_id - 1]);
		} else {
			// custom timer button or custom timer recently used
			TIMER_PARA_TRANS_T para;
			if ((key_id >= BUTTON_ID_TIMER_CUSTOM_1) && (key_id <= BUTTON_ID_TIMER_CUSTOM_3)) {
				if (timer_custom[key_id - BUTTON_ID_TIMER_CUSTOM_1].valid_flag == GX_TRUE) {
					para.hour = timer_custom[key_id - BUTTON_ID_TIMER_CUSTOM_1].hour;
					para.min = timer_custom[key_id - BUTTON_ID_TIMER_CUSTOM_1].min;
					para.sec = timer_custom[key_id - BUTTON_ID_TIMER_CUSTOM_1].sec;
					para.tenth_of_second = timer_custom[key_id - BUTTON_ID_TIMER_CUSTOM_1].tenth_of_second;
					app_timer_main_page_show(GX_FALSE, (GX_WINDOW *)&app_timer_window, &curr_widget, para);
				}
			} else if (key_id == BUTTON_ID_TIMER_ADD) {
				// enter custom page
				app_timer_page_jump_internal(&curr_widget, APP_TIMER_PAGE_JUMP_CUSTOM);
			}
		}
		return 0;
	}

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		gx_widget_event_process(widget, event_ptr);
		gx_system_timer_start(widget, CLOCK_TIMER_ID, 50, 50);
		app_timer_children_repos();
		return 0;
	}
	case GX_EVENT_HIDE: {
		gx_system_timer_stop(widget, CLOCK_TIMER_ID);
		break;
	}
	case GX_EVENT_TIMER:
		if ((event_ptr->gx_event_payload.gx_event_timer_id == CLOCK_TIMER_ID) &&
			(event_ptr->gx_event_target == widget)) {
			struct timespec ts;
			GX_STRING string;
			clock_gettime(CLOCK_REALTIME, &ts);
			time_t now = ts.tv_sec;
			struct tm *tm_now = gmtime(&now);
			static int hour_old, min_old;
			if ((hour_old != tm_now->tm_hour) || (min_old != tm_now->tm_min)) {
				snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
				time_prompt_buff[7] = 0;
				string.gx_string_ptr = time_prompt_buff;
				string.gx_string_length = strlen(string.gx_string_ptr);
				gx_prompt_text_set_ext(&time_prompt, &string);
				hour_old = tm_now->tm_hour;
				min_old = tm_now->tm_min;
			}
			return 0;
		}
		break;
	case GX_EVENT_PEN_DOWN:
		wm_point_down_record(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata);
		break;
	case GX_EVENT_PEN_UP:
		if (wm_quit_judge(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata) == GX_TRUE) {
			windows_mgr_page_jump((GX_WINDOW *)&app_timer_window, NULL, WINDOWS_OP_BACK);
		}
		break;
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

static void app_timer_child_widget_create(GX_WIDGET *widget)
{
	GX_RECTANGLE childSize;
	GX_STRING init_value;
	GX_VALUE left = widget->gx_widget_size.gx_rectangle_left;
	GX_VALUE top = widget->gx_widget_size.gx_rectangle_top;
	// first, create eight timer widget
	for (uint8_t i = 0; i < 8; i++) {
		gx_utility_rectangle_define(&childSize, left + 27 + i % 2 * (120 + 25), top + i / 2 * (120 + 33),
									left + 27 + i % 2 * (120 + 25) + 120 - 1, top + i / 2 * (120 + 33) + 120 - 1);
		timer_widget_create_helper(&child_timer_widgets[i], &childSize, (GX_WIDGET *)&scrollable_timer_page);
	}
	// second, create custom timer widget
	for (uint8_t i = 0; i < 3; i++) {
		gx_utility_rectangle_define(&childSize, left + 12, top + 600 + i * (65 + 12), left + 12 + 296 - 1,
									top + 600 + i * (65 + 12) + 65 - 1);
		snprintf(timer_custom_button_buff[i], sizeof(timer_custom_button_buff[i]), "00:00:00");
		init_value.gx_string_ptr = timer_custom_button_buff[i];
		init_value.gx_string_length = strlen(init_value.gx_string_ptr);
		custom_rounded_button_create2(&timer_custom_widget[i].button, BUTTON_ID_TIMER_CUSTOM_1 + i,
									  (GX_WIDGET *)&scrollable_timer_page, &childSize, GX_COLOR_ID_HONBOW_DARK_GRAY,
									  GX_ID_NONE, &init_value, GX_FONT_ID_SIZE_46, CUSTOM_ROUNDED_BUTTON_TEXT_MIDDLE);
	}
	// last, create custom timer widget button
	gx_utility_rectangle_define(&childSize, left + 12, top + 600 + 3 * (65 + 12), left + 12 + 296 - 1,
								top + 600 + 3 * (65 + 12) + 65 - 1);
	custom_rounded_button_create(&timer_add_button, BUTTON_ID_TIMER_ADD, (GX_WIDGET *)&scrollable_timer_page,
								 &childSize, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_ID_APP_ALARM_ADD, NULL,
								 CUSTOM_ROUNDED_BUTTON_TEXT_MIDDLE);
}

void app_timer_select_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 356);
	gx_widget_create(&curr_widget, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_widget_event_process_set(&curr_widget, timer_main_page_event_process);

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

	gx_utility_rectangle_define(&childSize, 0, 58, 319, 356);
	custom_scrollable_widget_create(&scrollable_timer_page, &curr_widget, &childSize);

	app_timer_child_widget_create((GX_WIDGET *)&scrollable_timer_page);
	app_timer_page_reg(APP_TIMER_PAGE_SELECT, &curr_widget);
}