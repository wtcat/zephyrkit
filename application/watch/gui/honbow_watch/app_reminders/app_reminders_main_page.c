#include "gx_api.h"
#include "gx_system.h"
#include "gx_widget.h"
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
#include <logging/log.h>
#include "guix_language_resources_custom.h"
#include "custom_widget_scrollable.h"
#include "custom_slide_delete_widget.h"
#include "custom_checkbox_basic.h"
#include "gx_system.h"
#include "view_service_reminders.h"
#include "data_service_custom.h"
#include "view_service_custom.h"
#include "app_reminders_main_page.h"
#include "custom_animation.h"
#include "app_reminders_page_mgr.h"
#include "windows_manager.h"

LOG_MODULE_DECLARE(guix_log);

static REMINDER_INFO_T *alarm_info;
static uint8_t alarm_info_size;

#define REMINDERS_INFO_WIDGET_MAX_SIZE 9
typedef struct {
	REMINDER_INFO_T *alarm_info;
	GX_WIDGET *widget_binding;
} REMINDERS_INFO_MGR_T;
static REMINDERS_INFO_MGR_T reminders_info_mgr[REMINDERS_INFO_WIDGET_MAX_SIZE];

static GX_WIDGET curr_page;
static CUSTOM_SCROLLABLE_WIDGET scrollable_alarm_page;
static GX_SCROLLBAR scrollbar;
static GX_SCROLLBAR_APPEARANCE scrollbar_properties = {
	8,						  /* scroll width                   */
	8,						  /* thumb width                    */
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
static GX_PROMPT title_prompt;
static GX_PROMPT time_prompt;

static CUSTOM_SLIDE_DEL_WIDGET alarm_widget_list[REMINDERS_INFO_WIDGET_MAX_SIZE];
static Custom_RoundedButton_TypeDef reminder_add_button;
static GX_PIXELMAP_BUTTON no_alarm_pixelmap;
static GX_PROMPT no_alarm_tips;
#define ALARM_ADD_BUTTON_ID 1
typedef struct {
	Custom_RoundedButton_TypeDef child1;
	GX_PROMPT alarm_name;
	GX_PROMPT alarm_time;
	GX_CHAR time_disp_buff[8];
	GX_PROMPT repeat[7];
	GX_PROMPT everyday_or_once_tips;
	CUSTOM_CHECKBOX_BASIC checkbox;
	Custom_RoundedButton_TypeDef child2;
} ALARM_CHILD_T;
static ALARM_CHILD_T alarm_child_list[REMINDERS_INFO_WIDGET_MAX_SIZE];
#define ALARM_CHILD_BUTTON_0_ID 1
#define ALARM_CHILD_BUTTON_1_ID 2

static void child_pos_reset(GX_POINT *point)
{
	for (uint8_t i = 0; i < sizeof(alarm_widget_list) / sizeof(CUSTOM_SLIDE_DEL_WIDGET); i++) {
		custom_deletable_slide_widget_reset(&alarm_widget_list[i], point, GX_FALSE);
	}
}
extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "提醒事项";
	case LANGUAGE_ENGLISH:
		return "reminders";
	default:
		return "reminders";
	}
}
static void alarm_info_mgr_sort(REMINDERS_INFO_MGR_T *alarm_info_mgr, uint8_t size);
static void alarm_info_widget_update(void);

static UINT scroll_page_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	CUSTOM_SCROLLABLE_WIDGET *custom_widget = CONTAINER_OF(widget, CUSTOM_SCROLLABLE_WIDGET, widget);
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		status = custom_scrollable_widget_event_process(custom_widget, event_ptr);
	}
	case DATA_SERVICE_EVT_REMINDERS_INFO_CHG: {
		GX_BOOL child_finded = GX_FALSE;
		GX_SCROLLBAR *pScroll;
		GX_VALUE top = 0;
		alarm_info_mgr_sort(reminders_info_mgr, sizeof(reminders_info_mgr) / sizeof(REMINDERS_INFO_MGR_T));
		GX_WIDGET *child = _gx_widget_first_client_child_get((GX_WIDGET *)custom_widget);
		if (child != NULL) {
			child_finded = GX_TRUE;
			top = child->gx_widget_size.gx_rectangle_top;
			while (child) {
				child = _gx_widget_next_client_child_get(child);
				if (child != NULL) {
					if (child->gx_widget_size.gx_rectangle_top < top) {
						top = child->gx_widget_size.gx_rectangle_top;
					}
				}
			}
		}
		alarm_info_widget_update();
		gx_window_scrollbar_find((GX_WINDOW *)custom_widget, GX_TYPE_VERTICAL_SCROLL, &pScroll);
		if (pScroll) {
			gx_scrollbar_reset(pScroll, GX_NULL);
		}

		if (child_finded == GX_TRUE) {
			gx_window_client_scroll((GX_WINDOW *)custom_widget, 0, top - widget->gx_widget_size.gx_rectangle_top);
		}

		return 0;
	}
	case GX_SIGNAL(ALARM_ADD_BUTTON_ID, GX_EVENT_CLICKED):
		LOG_INF("alarm add button clicked!");
		app_reminders_page_jump(REMINDERS_PAGE_MAIN, REMINDERS_OP_ADD_NEW);
		return status;
	case GX_EVENT_PEN_DOWN:
		child_pos_reset(&event_ptr->gx_event_payload.gx_event_pointdata);
		return custom_scrollable_widget_event_process(custom_widget, event_ptr);
	default:
		return custom_scrollable_widget_event_process(custom_widget, event_ptr);
	}
	return status;
}
static UINT custom_slide_widget_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	CUSTOM_SLIDE_DEL_WIDGET *custom_widget = CONTAINER_OF(widget, CUSTOM_SLIDE_DEL_WIDGET, widget_main);
	ALARM_CHILD_T *alarm_child = CONTAINER_OF(custom_widget->child1, ALARM_CHILD_T, child1);
	static GX_POINT point_record;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_UP:
		point_record = event_ptr->gx_event_payload.gx_event_pointdata;
		status = custom_deletable_slide_widget_event_process(widget, event_ptr);
		return status;
	case GX_SIGNAL(ALARM_CHILD_BUTTON_0_ID, GX_EVENT_CLICKED): {
		// TODO: 这里需要判断一下按键范围，区分进入alarm修改界面或者是进行checkbox的状态改变？
		LOG_INF("button 0 clicked!");
		GX_RECTANGLE *child1_rect = &custom_widget->child1->gx_widget_size;
		GX_VALUE half_width = (child1_rect->gx_rectangle_right - child1_rect->gx_rectangle_left + 1) >> 1;
		if ((point_record.gx_point_x > child1_rect->gx_rectangle_left) &&
			(point_record.gx_point_x < child1_rect->gx_rectangle_left + half_width)) {
			LOG_INF("left 1/2 clicked!");
			for (uint8_t i = 0; i < REMINDERS_INFO_WIDGET_MAX_SIZE; i++) {
				if (reminders_info_mgr[i].widget_binding == (GX_WIDGET *)custom_widget) {
					app_reminders_instance_record(reminders_info_mgr[i].alarm_info);
					app_reminders_page_jump(REMINDERS_PAGE_MAIN, REMINDERS_OP_EDIT);
					break;
				}
			}

		} else {
			LOG_INF("right 1/2 clicked!");
			for (uint8_t i = 0; i < REMINDERS_INFO_WIDGET_MAX_SIZE; i++) {
				if (reminders_info_mgr[i].widget_binding == (GX_WIDGET *)custom_widget) {
					view_service_event_submit(VIEW_EVENT_TYPE_REMINDERS_EDITED, NULL, 0);
					if (reminders_info_mgr[i].alarm_info->alarm_flag & 0x01) {
						reminders_info_mgr[i].alarm_info->alarm_flag &= 0xfe;
						if (alarm_child->checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
							custom_checkbox_basic_toggle(&alarm_child->checkbox);
						}
					} else {
						reminders_info_mgr[i].alarm_info->alarm_flag |= 0x01;
						if (!(alarm_child->checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED)) {
							custom_checkbox_basic_toggle(&alarm_child->checkbox);
						}
					}
					break;
				}
			}
		}
		return 0;
	}
	case GX_SIGNAL(ALARM_CHILD_BUTTON_1_ID, GX_EVENT_CLICKED):
		LOG_INF("button 1 clicked!");
		for (uint8_t i = 0; i < REMINDERS_INFO_WIDGET_MAX_SIZE; i++) {
			if (reminders_info_mgr[i].widget_binding == (GX_WIDGET *)custom_widget) {
				view_service_event_submit(VIEW_EVENT_TYPE_REMINDERS_DEL, &reminders_info_mgr[i].alarm_info, 4);
				// gx_widget_hide(custom_widget);
				custom_deletable_slide_widget_reset(custom_widget, NULL, GX_TRUE);
				break;
			}
		}
		return 0;
	default:
		break;
	}
	return custom_deletable_slide_widget_event_process(widget, event_ptr);
}

static GX_CHAR *day_tips[7] = {"S", "M", "T", "W", "T", "F", "S"};
static void alarm_info_widget_create(Custom_RoundedButton_TypeDef *button)
{
	GX_WIDGET *parent = (GX_WIDGET *)button;
	ALARM_CHILD_T *alarm_child = CONTAINER_OF(button, ALARM_CHILD_T, child1);
	CUSTOM_CHECKBOX_BASIC_INFO info;
	info.background_checked = GX_COLOR_ID_HONBOW_GREEN;
	info.background_uchecked = GX_COLOR_ID_HONBOW_GRAY;
	info.rolling_ball_color = GX_COLOR_ID_HONBOW_WHITE;
	info.widget_id = 1;
	info.pos_offset = 5;
	GX_RECTANGLE size;
	gx_utility_rectangle_define(
		&size, parent->gx_widget_size.gx_rectangle_left + 210, parent->gx_widget_size.gx_rectangle_top + 48,
		parent->gx_widget_size.gx_rectangle_left + 210 + 66 - 1, parent->gx_widget_size.gx_rectangle_top + 48 + 36 - 1);
	custom_checkbox_basic_create(&alarm_child->checkbox, parent, &info, &size);

	GX_VALUE left = parent->gx_widget_size.gx_rectangle_left + 10;
	GX_VALUE top = parent->gx_widget_size.gx_rectangle_top + 12;
	gx_utility_rectangle_define(&size, left, top, left + 286 - 1, top + 25 - 1);
	gx_prompt_create(&alarm_child->alarm_name, "alarm_name", parent, GX_ID_NONE,
					 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_BORDER_NONE | GX_STYLE_TEXT_LEFT, GX_ID_NONE,
					 &size);
	gx_prompt_text_color_set(&alarm_child->alarm_name, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&alarm_child->alarm_name, GX_FONT_ID_SIZE_26);
	GX_STRING value;
	value.gx_string_ptr = "alarm";
	value.gx_string_length = strlen(value.gx_string_ptr);
	gx_prompt_text_set_ext(&alarm_child->alarm_name, &value);

	left = parent->gx_widget_size.gx_rectangle_left + 10;
	top = parent->gx_widget_size.gx_rectangle_top + 43;
	gx_utility_rectangle_define(&size, left, top, left + 117 - 1, top + 46 - 1);
	gx_prompt_create(&alarm_child->alarm_time, "alarm_time", parent, GX_ID_NONE,
					 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_BORDER_NONE | GX_STYLE_TEXT_LEFT, GX_ID_NONE,
					 &size);
	gx_prompt_text_color_set(&alarm_child->alarm_time, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&alarm_child->alarm_time, GX_FONT_ID_SIZE_46);
	value.gx_string_ptr = "00:00";
	value.gx_string_length = strlen(value.gx_string_ptr);
	gx_prompt_text_set_ext(&alarm_child->alarm_time, &value);

	for (uint8_t i = 0; i < 7; i++) {
		left = parent->gx_widget_size.gx_rectangle_left + 10 + i * (5 + 26);
		top = parent->gx_widget_size.gx_rectangle_top + 96;
		gx_utility_rectangle_define(&size, left, top, left + 30 - 1, top + 26 - 1);
		gx_prompt_create(&alarm_child->repeat[i], "alarm_repeat", parent, GX_ID_NONE,
						 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_BORDER_NONE | GX_STYLE_TEXT_CENTER,
						 GX_ID_NONE, &size);
		gx_prompt_text_color_set(&alarm_child->repeat[i], GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
								 GX_COLOR_ID_HONBOW_WHITE);
		gx_prompt_font_set(&alarm_child->repeat[i], GX_FONT_ID_SIZE_26);
		value.gx_string_ptr = day_tips[i];
		value.gx_string_length = strlen(value.gx_string_ptr);
		gx_prompt_text_set_ext(&alarm_child->repeat[i], &value);
	}

	left = parent->gx_widget_size.gx_rectangle_left + 10;
	top = parent->gx_widget_size.gx_rectangle_top + 96;
	gx_utility_rectangle_define(&size, left, top, left + 120 - 1, top + 26 - 1);
	gx_prompt_create(&alarm_child->everyday_or_once_tips, "alarm_repeat_tips", parent, GX_ID_NONE,
					 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_BORDER_NONE | GX_STYLE_TEXT_LEFT, GX_ID_NONE,
					 &size);
	gx_prompt_text_color_set(&alarm_child->everyday_or_once_tips, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&alarm_child->everyday_or_once_tips, GX_FONT_ID_SIZE_26);
	value.gx_string_ptr = "Only once";
	value.gx_string_length = strlen(value.gx_string_ptr);
	gx_prompt_text_set_ext(&alarm_child->everyday_or_once_tips, &value);
}
static void update_alarm_info_disp_init(REMINDER_INFO_T *alarm_info, uint8_t size);
static WM_QUIT_JUDGE_T pen_info_judge;
static UINT reminders_mian_page_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN:
		wm_point_down_record(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata);
		break;
	case GX_EVENT_PEN_UP:
		if (wm_quit_judge(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata) == GX_TRUE) {
			windows_mgr_page_jump((GX_WINDOW *)&app_reminders_window, NULL, WINDOWS_OP_BACK);
		}
		break;
	case GX_EVENT_LANGUAGE_CHANGE:
		break;
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

void app_reminders_main_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE size;
	gx_utility_rectangle_define(&size, 0, 0, 319, 359);
	gx_widget_create(&curr_page, "alarm main page", window, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &size);
	gx_widget_fill_color_set(&curr_page, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);
	gx_widget_event_process_set(&curr_page, reminders_mian_page_event_process);

	custom_window_title_create(&curr_page, NULL, &title_prompt, &time_prompt);
	GX_STRING string;
	string.gx_string_ptr = get_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&title_prompt, &string);

	gx_utility_rectangle_define(&size, 12, 58, 309, 339);
	custom_scrollable_widget_create(&scrollable_alarm_page, &curr_page, &size);
	gx_widget_event_process_set(&scrollable_alarm_page, scroll_page_event_process);

	gx_vertical_scrollbar_create(&scrollbar, NULL, &scrollable_alarm_page, &scrollbar_properties,
								 GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB |
									 GX_SCROLLBAR_VERTICAL);
	extern VOID custom_scrollbar_vertical_draw(GX_SCROLLBAR * scrollbar);
	gx_widget_draw_set(&scrollbar, custom_scrollbar_vertical_draw);
	gx_widget_fill_color_set(&scrollbar, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
							 GX_COLOR_ID_HONBOW_DARK_GRAY);
	GX_VALUE top = 0;
	GX_VALUE left = 0;
	GX_RECTANGLE *parent_size = &scrollable_alarm_page.widget.gx_widget_size;
	for (uint8_t i = 0; i < sizeof(alarm_widget_list) / sizeof(CUSTOM_SLIDE_DEL_WIDGET); i++) {
		top = parent_size->gx_rectangle_top + i * (134 - 1) + i * 10;
		left = parent_size->gx_rectangle_left;
		gx_utility_rectangle_define(&size, left, top, left + 285 - 1, top + (134 - 1));
		custom_rounded_button_create(&alarm_child_list[i].child1, ALARM_CHILD_BUTTON_0_ID, NULL, &size,
									 GX_COLOR_ID_HONBOW_DARK_GRAY, GX_ID_NONE, NULL, CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
		alarm_info_widget_create(&alarm_child_list[i].child1);
		left = parent_size->gx_rectangle_left - 12 + 320;
		gx_utility_rectangle_define(&size, left, top, left + 90 - 1, top + (134 - 1));
		custom_rounded_button_create(&alarm_child_list[i].child2, ALARM_CHILD_BUTTON_1_ID, NULL, &size,
									 GX_COLOR_ID_HONBOW_RED, GX_PIXELMAP_ID_APP_ALARM_DELETE, NULL,
									 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
		left = parent_size->gx_rectangle_left;
		gx_utility_rectangle_define(&size, left, top, left + 285 - 1, top + (134 - 1));
		custom_deletable_slide_widget_create(&alarm_widget_list[i], GX_ID_NONE, (GX_WIDGET *)&scrollable_alarm_page,
											 &size, (GX_WIDGET *)&alarm_child_list[i].child1,
											 (GX_WIDGET *)&alarm_child_list[i].child2);
		gx_widget_event_process_set(&alarm_widget_list[i].widget_main, custom_slide_widget_event_process);
	}

	// finally add alarm button to page!
	top += 134 - 1 + 10;
	left = parent_size->gx_rectangle_left;
	gx_utility_rectangle_define(&size, left, top, left + 285 - 1, top + (65 - 1));
	custom_rounded_button_create(&reminder_add_button, ALARM_ADD_BUTTON_ID, (GX_WIDGET *)&scrollable_alarm_page, &size,
								 GX_COLOR_ID_HONBOW_GREEN, GX_PIXELMAP_ID_APP_ALARM_ADD, NULL,
								 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

	// no alarm tips
	top = parent_size->gx_rectangle_top + 44;
	left = parent_size->gx_rectangle_left + 108;
	gx_utility_rectangle_define(&size, left, top, left + 80 - 1, top + (80 - 1));
	gx_pixelmap_button_create(&no_alarm_pixelmap, NULL, &scrollable_alarm_page,
							  GX_PIXELMAP_ID_APP_REMINDERS_IC__REMIND_NO, GX_PIXELMAP_ID_APP_REMINDERS_IC__REMIND_NO,
							  GX_PIXELMAP_ID_APP_REMINDERS_IC__REMIND_NO, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED,
							  GX_ID_NONE, &size);

	top = parent_size->gx_rectangle_top + 128;
	left = parent_size->gx_rectangle_left;
	gx_utility_rectangle_define(&size, left, top, parent_size->gx_rectangle_right, top + 30 - 1);
	gx_prompt_create(&no_alarm_tips, NULL, &scrollable_alarm_page, GX_ID_NONE,
					 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_BORDER_NONE | GX_STYLE_TEXT_CENTER, GX_ID_NONE,
					 &size);
	gx_prompt_text_color_set(&no_alarm_tips, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&no_alarm_tips, GX_FONT_ID_SIZE_30);
	string.gx_string_ptr = "NO REMINDER";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&no_alarm_tips, &string);

	// update display by viewservice
	view_service_reminders_info_get(&alarm_info, &alarm_info_size);
	update_alarm_info_disp_init(alarm_info, alarm_info_size);

	app_reminders_page_reg(REMINDERS_PAGE_MAIN, &curr_page, NULL);
}

static void alarm_info_mgr_sort(REMINDERS_INFO_MGR_T *alarm_info_mgr, uint8_t size)
{
	for (uint8_t i = 0; i < size; i++) {
		for (uint8_t j = i + 1; j < size; j++) {
			REMINDERS_INFO_MGR_T tmp;
			if ((alarm_info_mgr[j].alarm_info->info_avalid == REMINDERS_INFO_VALID_FLAG) &&
				(alarm_info_mgr[i].alarm_info->info_avalid == REMINDERS_INFO_VALID_FLAG)) {
				if ((alarm_info_mgr[j].alarm_info->hour < alarm_info_mgr[i].alarm_info->hour) ||
					((alarm_info_mgr[j].alarm_info->hour == alarm_info_mgr[i].alarm_info->hour) &&
					 (alarm_info_mgr[j].alarm_info->min < alarm_info_mgr[i].alarm_info->min))) {
					tmp = alarm_info_mgr[j];
					alarm_info_mgr[j] = alarm_info_mgr[i];
					alarm_info_mgr[i] = tmp;
				}
			} else if (alarm_info_mgr[j].alarm_info->info_avalid == REMINDERS_INFO_VALID_FLAG) {
				tmp = alarm_info_mgr[j];
				alarm_info_mgr[j] = alarm_info_mgr[i];
				alarm_info_mgr[i] = tmp;
			}
		}
	}
}

static void alarm_info_widget_update(void)
{
	GX_WIDGET *child = _gx_widget_first_client_child_get((GX_WIDGET *)&scrollable_alarm_page);
	while (child) {
		gx_widget_detach(child);
		child = _gx_widget_first_client_child_get((GX_WIDGET *)&scrollable_alarm_page);
	}
	GX_RECTANGLE *parent_size = NULL;
	GX_VALUE left = 0;
	parent_size = &scrollable_alarm_page.widget.gx_widget_size;
	GX_VALUE top = 0;
	left = parent_size->gx_rectangle_left;

	GX_RECTANGLE rect_size;
	uint8_t valid_child_cnt = 0;
	GX_STRING init_string;
	for (uint8_t i = 0; i < sizeof(reminders_info_mgr) / sizeof(REMINDERS_INFO_MGR_T); i++) {
		if ((reminders_info_mgr[i].widget_binding) &&
			(reminders_info_mgr[i].alarm_info->info_avalid == REMINDERS_INFO_VALID_FLAG)) {
			top = parent_size->gx_rectangle_top + valid_child_cnt * (134 - 1) + valid_child_cnt * 10;
			gx_utility_rectangle_define(&rect_size, left, top, left + 285 - 1, top + (134 - 1));
			gx_widget_resize(reminders_info_mgr[i].widget_binding, &rect_size);
			gx_widget_attach(&scrollable_alarm_page, reminders_info_mgr[i].widget_binding);

			CUSTOM_SLIDE_DEL_WIDGET *widget_custom = (CUSTOM_SLIDE_DEL_WIDGET *)reminders_info_mgr[i].widget_binding;
			// sync info to widget!
			ALARM_CHILD_T *alarm_child = CONTAINER_OF(widget_custom->child1, ALARM_CHILD_T, child1);

			if (reminders_info_mgr[i].alarm_info->repeat_flags == 0x80) {
				for (uint8_t repeat_index = 0; repeat_index < 7; repeat_index++) {
					gx_widget_hide(&alarm_child->repeat[repeat_index]);
				}
				gx_widget_show(&alarm_child->everyday_or_once_tips);
			} else {
				gx_widget_hide(&alarm_child->everyday_or_once_tips);
				for (uint8_t repeat_index = 0; repeat_index < 7; repeat_index++) {
					gx_widget_show(&alarm_child->repeat[repeat_index]);
					if (reminders_info_mgr[i].alarm_info->repeat_flags & (1 << repeat_index)) {
						gx_prompt_text_color_set(&alarm_child->repeat[repeat_index], GX_COLOR_ID_HONBOW_WHITE,
												 GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
					} else {
						gx_prompt_text_color_set(&alarm_child->repeat[repeat_index], GX_COLOR_ID_HONBOW_GRAY_40_PERCENT,
												 GX_COLOR_ID_HONBOW_GRAY_40_PERCENT,
												 GX_COLOR_ID_HONBOW_GRAY_40_PERCENT);
					}
					init_string.gx_string_ptr = day_tips[repeat_index];
					init_string.gx_string_length = strlen(init_string.gx_string_ptr);
					gx_prompt_text_set_ext(&alarm_child->repeat[repeat_index], &init_string);
				}
			}

			init_string.gx_string_ptr = reminders_info_mgr[i].alarm_info->desc;
			init_string.gx_string_length = strlen(init_string.gx_string_ptr);
			gx_prompt_text_set_ext(&alarm_child->alarm_name, &init_string);

			snprintf(alarm_child->time_disp_buff, sizeof(alarm_child->time_disp_buff), "%02d:%02d",
					 reminders_info_mgr[i].alarm_info->hour, reminders_info_mgr[i].alarm_info->min);
			init_string.gx_string_ptr = alarm_child->time_disp_buff;
			init_string.gx_string_length = strlen(init_string.gx_string_ptr);
			gx_prompt_text_set_ext(&alarm_child->alarm_time, &init_string);

			// rpt everyday or only once!

			// checkbox status sync
			if (reminders_info_mgr[i].alarm_info->alarm_flag & 0x01) {
				if (!(alarm_child->checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED)) {
					custom_checkbox_basic_toggle(&alarm_child->checkbox);
				}
			} else {
				if (alarm_child->checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
					custom_checkbox_basic_toggle(&alarm_child->checkbox);
				}
			}

			valid_child_cnt++;
		}
	}
	// finally add alarm button to page!
	top += 134 - 1 + 10;
	if ((valid_child_cnt <= 1) && (top < 274)) {
		top = 274;
	}
	gx_utility_rectangle_define(&rect_size, left, top, left + 290 - 1, top + (65 - 1));
	gx_widget_attach(&scrollable_alarm_page, &reminder_add_button);
	gx_widget_resize(&reminder_add_button, &rect_size);

	GX_VALUE total_height;
	GX_VALUE widget_height = 0;
	gx_widget_height_get(&scrollable_alarm_page.widget, &widget_height);
	if (reminders_info_mgr[0].alarm_info->info_avalid == REMINDERS_INFO_VALID_FLAG) {
		total_height = top + 65 - 1 - reminders_info_mgr[0].widget_binding->gx_widget_size.gx_rectangle_top;
	} else {
		total_height = top + 65 - 1 - scrollable_alarm_page.widget.gx_widget_size.gx_rectangle_top;
	}

	if (total_height <= widget_height) {
		gx_widget_hide(&scrollbar);
	} else {
		gx_widget_show(&scrollbar);
	}

	if (valid_child_cnt == 0) {
		GX_RECTANGLE size;
		GX_VALUE top = 0;
		GX_VALUE left = 0;
		GX_RECTANGLE *parent_size = &scrollable_alarm_page.widget.gx_widget_size;
		top = parent_size->gx_rectangle_top + 44;
		left = parent_size->gx_rectangle_left + 108;
		gx_utility_rectangle_define(&size, left, top, left + 80 - 1, top + (80 - 1));
		gx_widget_resize(&no_alarm_pixelmap, &size);

		top = parent_size->gx_rectangle_top + 128;
		left = parent_size->gx_rectangle_left;
		gx_utility_rectangle_define(&size, left, top, parent_size->gx_rectangle_right, top + 30 - 1);
		gx_widget_resize(&no_alarm_tips, &size);

		gx_widget_attach(&scrollable_alarm_page, &no_alarm_tips);
		gx_widget_attach(&scrollable_alarm_page, &no_alarm_pixelmap);
	}
}

static void update_alarm_info_disp_init(REMINDER_INFO_T *alarm_info, uint8_t size)
{
	if (size != sizeof(reminders_info_mgr) / sizeof(REMINDERS_INFO_MGR_T)) {
		LOG_ERR("ui widget size not equal to alarm info size!");
	}

	// binding to widget fixed!
	for (uint8_t i = 0; i < sizeof(reminders_info_mgr) / sizeof(REMINDERS_INFO_MGR_T); i++) {
		reminders_info_mgr[i].alarm_info = &alarm_info[i];
		reminders_info_mgr[i].widget_binding = (GX_WIDGET *)&alarm_widget_list[i];
	}

	// step1: sort alarm_info by time
	alarm_info_mgr_sort(reminders_info_mgr, sizeof(reminders_info_mgr) / sizeof(REMINDERS_INFO_MGR_T));

	// step2: display update
	alarm_info_widget_update();
}