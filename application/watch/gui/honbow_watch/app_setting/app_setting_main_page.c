#include "gx_api.h"
#include "custom_window_title.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "custom_rounded_button.h"
#include "sys/util.h"
#include "custom_window_title.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include "string.h"
#include "stdio.h"
#include "app_list_define.h"
#include "app_setting.h"
#include <logging/log.h>
#include "guix_language_resources_custom.h"
#include "custom_vertical_scroll_limit.h"
#include "windows_manager.h"

LOG_MODULE_DECLARE(guix_log);

GX_WIDGET setting_main_page;
GX_WIDGET *setting_child[7] = {0};
static GX_PROMPT setting_prompt;
static GX_PROMPT time_prompt;
static char time_prompt_buff[8];

// for main page
static GX_VERTICAL_LIST app_setting_list;
typedef struct APP_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	Custom_RoundedButton_TypeDef child;
	SETTING_CHILD_PAGE_T id;
} APP_SETTING_LIST_ROW;

GX_SCROLLBAR_APPEARANCE app_setting_list_scrollbar_properties = {
	9,						  /* scroll width                   */
	9,						  /* thumb width                    */
	0,						  /* thumb travel min               */
	0,						  /* thumb travel max               */
	4,						  /* thumb border style             */
	0,						  /* scroll fill pixelmap           */
	0,						  /* scroll thumb pixelmap          */
	0,						  /* scroll up pixelmap             */
	0,						  /* scroll down pixelmap           */
	GX_COLOR_ID_HONBOW_GREEN, /* scroll thumb color             */
	GX_COLOR_ID_HONBOW_GREEN, /* scroll thumb border color      */
	GX_COLOR_ID_HONBOW_GREEN, /* scroll button color            */
};
static GX_SCROLLBAR app_setting_list_scrollbar;
#define APP_SETTING_LIST_VISIBLE_ROWS 4
static APP_SETTING_LIST_ROW app_setting_list_row_memory[APP_SETTING_LIST_VISIBLE_ROWS + 1];
extern GX_WINDOW_ROOT *root;

static const GX_CHAR *lauguage_ch_setting_element[7] = {"显示",			"APP视图",	"勿扰模式", "振动强度",
														"自动运动识别", "音乐控制", "系统"};
static const GX_CHAR *lauguage_en_setting_element[7] = {"Display",			"App View",		 "DND",	  "Vibration Tuner",
														"Activity Detect ", "Music Control", "System"};

static const GX_CHAR **setting_string[] = {
	[LANGUAGE_CHINESE] = lauguage_ch_setting_element, [LANGUAGE_ENGLISH] = lauguage_en_setting_element};
static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return setting_string[id];
}

static const GX_CHAR *get_language_setting_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "设置";
	case LANGUAGE_ENGLISH:
		return "Setting";
	default:
		return "Setting";
	}
}

static void app_setting_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

void setting_main_page_jump_reg(GX_WIDGET *dst, SETTING_CHILD_PAGE_T id)
{
	setting_child[id] = dst;
}

static GX_WIDGET *setting_menu_screen_list[] = {
	&setting_main_page,
	GX_NULL,
	GX_NULL,
};
#include "custom_animation.h"
void jump_setting_main_page(GX_WIDGET *now)
{
#if 0
	gx_widget_detach(now);
	gx_widget_attach((GX_WIDGET *)&app_setting_window, &setting_main_page);
#else
	setting_menu_screen_list[1] = now;
	custom_animation_start(setting_menu_screen_list, (GX_WIDGET *)&app_setting_window, SCREEN_SLIDE_RIGHT);
#endif
}

static UINT setting_child_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(1, GX_EVENT_CLICKED): {
		APP_SETTING_LIST_ROW *row = CONTAINER_OF(widget, APP_SETTING_LIST_ROW, widget);
		if (setting_child[row->id] != NULL) {
			setting_menu_screen_list[1] = setting_child[row->id];
			custom_animation_start(setting_menu_screen_list, (GX_WIDGET *)&app_setting_window, SCREEN_SLIDE_LEFT);
		}
		return 0;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

static VOID app_setting_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	APP_SETTING_LIST_ROW *row = (APP_SETTING_LIST_ROW *)widget;
	GX_BOOL result;

	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 295, 75 - 1);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&row->widget, setting_child_processing_function);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								 GX_COLOR_ID_HONBOW_BLACK);

		childsize.gx_rectangle_bottom -= 5;
		GX_STRING string;
		app_setting_list_prompt_get(index, &string);
		custom_rounded_button_create(&row->child, 1, &row->widget, &childsize, GX_COLOR_ID_HONBOW_DARK_GRAY,
									 GX_PIXELMAP_NULL, &string, CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
		row->id = index;
	}
}

static VOID app_setting_list_children_create(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < APP_SETTING_LIST_VISIBLE_ROWS + 1; count++) {
		app_setting_list_row_create(list, (GX_WIDGET *)&app_setting_list_row_memory[count], count);
	}
}

static VOID app_setting_list_callback(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_SETTING_LIST_ROW *row = CONTAINER_OF(widget, APP_SETTING_LIST_ROW, widget);
	GX_STRING string;
	app_setting_list_prompt_get(index, &string);
	gx_prompt_text_set_ext(&row->child.desc, &string);
	row->id = index;
}
UINT app_setting_event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr);

void app_setting_main_page_init(GX_WINDOW *window)
{
	gx_widget_create(&setting_main_page, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
					 &window->gx_widget_size);

	GX_WIDGET *parent = &setting_main_page;

	custom_window_title_create(parent, NULL, &setting_prompt, &time_prompt);

	GX_STRING string;
	string.gx_string_ptr = get_language_setting_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
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

	GX_BOOL result;
	gx_widget_created_test(&app_setting_list, &result);
	if (!result) {
		GX_RECTANGLE child_size;
		gx_utility_rectangle_define(&child_size, 8, 60, 316 - 1, 359);
		gx_vertical_list_create(&app_setting_list, NULL, parent, 7, app_setting_list_callback,
								GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child_size);
		gx_widget_fill_color_set(&app_setting_list, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								 GX_COLOR_ID_HONBOW_BLACK);

		gx_widget_event_process_set(&app_setting_list, custom_vertical_event_processing_function);

		app_setting_list_children_create(&app_setting_list);

		gx_vertical_scrollbar_create(
			&app_setting_list_scrollbar, NULL, &app_setting_list, &app_setting_list_scrollbar_properties,
			GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB | GX_SCROLLBAR_VERTICAL);
		gx_widget_fill_color_set(&app_setting_list_scrollbar, GX_COLOR_ID_HONBOW_DARK_GRAY,
								 GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY);
		extern VOID custom_scrollbar_vertical_draw(GX_SCROLLBAR * scrollbar);
		gx_widget_draw_set(&app_setting_list_scrollbar, custom_scrollbar_vertical_draw);
	}

	gx_widget_event_process_set(parent, app_setting_event_processing_function);
}

#define TIME_FRESH_TIMER_ID 1
UINT app_setting_event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static WM_QUIT_JUDGE_T pen_info_judge;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		gx_system_timer_start(widget, TIME_FRESH_TIMER_ID, 50, 50);
	} break;
	case GX_EVENT_HIDE: {
		gx_system_timer_stop(widget, TIME_FRESH_TIMER_ID);
	} break;
	case GX_EVENT_PEN_DOWN:
		wm_point_down_record(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata);
		break;
	case GX_EVENT_PEN_UP:
		if (wm_quit_judge(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata) == GX_TRUE) {
			windows_mgr_page_jump((GX_WINDOW *)&app_setting_window, NULL, WINDOWS_OP_BACK);
		}
		break;
	case GX_EVENT_TIMER: {
		if (event_ptr->gx_event_payload.gx_event_timer_id == TIME_FRESH_TIMER_ID) {
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
	} break;
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}
