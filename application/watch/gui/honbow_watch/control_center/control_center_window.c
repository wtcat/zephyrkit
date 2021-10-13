#include "control_center_window.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "custom_icon_utils.h"
#include <stdint.h>
#include <stdio.h>
#include "zephyr.h"
#include "sys/timeutil.h"
#include "time.h"
#include <posix/time.h>
#include "posix/sys/time.h"
#include "custom_animation.h"
#include "windows_manager.h"
typedef struct children_module {
	Custom_Icon_TypeDef night_mode;
	bool night_mode_on;
	Custom_Icon_TypeDef bright_screen_switch;
	bool auto_bright_screen;
	Custom_Icon_TypeDef disturb_mode;
	Custom_Icon_TypeDef bright_ness;
	uint8_t bright_level;
	Custom_Icon_TypeDef set_up;

	GX_ICON bt_link_status;
	bool bt_conn;
	GX_PROMPT date;
	char date_buff[10];
	GX_PROMPT time;
	char time_buff[10];

	GX_PROMPT operation_tips;
	char tips_buff[20];

	GX_PROMPT battery_tips;
	char batt_tips_buff[5];

	Custom_Icon_TypeDef line_tips;
} CONTROL_CENTER_CHILD_T;

static CONTROL_CENTER_CHILD_T children_module;

static GX_PROMPT prompt_for_brightness;
void brightness_adjust_draw(GX_WIDGET *widget)
{
	custom_icon_draw(widget);

	static char disp[4];
	snprintf(disp, 4, "%d", children_module.bright_level);
	GX_STRING string;
	string.gx_string_ptr = disp;
	string.gx_string_length = strlen(disp);
	gx_prompt_text_set_ext(&prompt_for_brightness, &string);

	gx_widget_children_draw(widget);
}
void control_center_window_init(GX_WIDGET *parent) // add child module for control center window
{
	children_module.bright_level = 5;
	extern int lcd_brightness_adjust(uint8_t value);
	lcd_brightness_adjust(children_module.bright_level * 50);

	GX_RECTANGLE child;

	gx_utility_rectangle_define(&child, 12, 89, 140 + 12 - 1, 89 + 80 - 1);
	if (children_module.night_mode_on == true) {
		custom_icon_create(&children_module.night_mode, NIGHT_MODE_ICON_ID, parent, &child, GX_COLOR_ID_HONBOW_BLUE,
						   GX_PIXELMAP_ID_CONTROL_CENTER_NIGHT);
	} else {
		custom_icon_create(&children_module.night_mode, NIGHT_MODE_ICON_ID, parent, &child,
						   GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_ID_CONTROL_CENTER_NIGHT);
	}

	gx_utility_rectangle_define(&child, 168, 89, 168 + 140 - 1, 89 + 80 - 1);
	children_module.auto_bright_screen = true;
	custom_icon_create(&children_module.bright_screen_switch, BRIGHT_SCREEN_ON_ICON_ID, parent, &child,
					   GX_COLOR_ID_HONBOW_GREEN, GX_PIXELMAP_ID_CONTROL_CENTER_BRIGHT_SCREEN_ON);

	gx_utility_rectangle_define(&child, 12, 179, 12 + 140 - 1, 179 + 80 - 1);
	custom_icon_create(&children_module.disturb_mode, DISTURB_MODE_ICON_ID, parent, &child, GX_COLOR_ID_HONBOW_RED,
					   GX_PIXELMAP_ID_CONTROL_CENTER_DO_NOT_DISTURB);

	gx_utility_rectangle_define(&child, 168, 179, 168 + 140 - 1, 179 + 80 - 1);
	custom_icon_create(&children_module.bright_ness, BRIGHTNESS_ICON_ID, parent, &child, GX_COLOR_ID_HONBOW_YELLOW,
					   GX_PIXELMAP_ID_CONTROL_CENTER_BRIGHTNESS);
	gx_prompt_create(&prompt_for_brightness, NULL, &children_module.bright_ness.widget, 0,
					 GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, GX_ID_NONE, &child);
	gx_prompt_text_color_set(&prompt_for_brightness, GX_COLOR_ID_BLUE, GX_COLOR_ID_BLUE, GX_COLOR_ID_HONBOW_YELLOW);
	gx_prompt_font_set(&prompt_for_brightness, GX_FONT_ID_SIZE_30);

	gx_widget_draw_set(&children_module.bright_ness.widget, brightness_adjust_draw);

	gx_utility_rectangle_define(&child, 12, 269, 12 + 296 - 1, 269 + 80 - 1);
	custom_icon_create(&children_module.set_up, SET_UP_ICON_ID, parent, &child, GX_COLOR_ID_HONBOW_DARK_GRAY,
					   GX_PIXELMAP_ID_CONTROL_CENTER_SET_UP);

	gx_utility_rectangle_define(&child, 12, 47, 12 + 147 - 1, 30 + 47 - 1);
	gx_prompt_create(&children_module.date, NULL, parent, 0,
					 GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, GX_ID_NONE, &child);
	gx_prompt_text_color_set(&children_module.date, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&children_module.date, GX_FONT_ID_SIZE_30);

	gx_utility_rectangle_define(&child, 228, 47, 230 + 82 - 1, 30 + 47 - 1);
	gx_prompt_create(&children_module.time, NULL, parent, 0,
					 GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, GX_ID_NONE, &child);
	gx_prompt_text_color_set(&children_module.time, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&children_module.time, GX_FONT_ID_SIZE_30);

	// battey prompt
	gx_utility_rectangle_define(&child, 230, 16, 236 + 74 - 1, 16 + 28 - 1);
	gx_prompt_create(&children_module.battery_tips, NULL, parent, 0,
					 GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, GX_ID_NONE, &child);
	gx_prompt_text_color_set(&children_module.battery_tips, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&children_module.battery_tips, GX_FONT_ID_SIZE_26);
	// should add get battery capacity itf
	snprintf(children_module.batt_tips_buff, sizeof(children_module.batt_tips_buff), "%d%%", 100);
	GX_STRING string;
	string.gx_string_ptr = children_module.batt_tips_buff;
	string.gx_string_length = strlen(children_module.batt_tips_buff);
	gx_prompt_text_set_ext(&children_module.battery_tips, &string);

	// bt status icon
	children_module.bt_conn = true;
	gx_icon_create(&children_module.bt_link_status, NULL, parent, GX_PIXELMAP_ID_CONTROL_CENTER_LINK,
				   GX_STYLE_BORDER_NONE, GX_ID_NONE, 16, 20);
	if (children_module.bt_conn == true) {
		gx_icon_pixelmap_set(&children_module.bt_link_status, GX_PIXELMAP_ID_CONTROL_CENTER_LINK, 0);
	} else {
		gx_icon_pixelmap_set(&children_module.bt_link_status, GX_PIXELMAP_ID_CONTROL_CENTER_LINK_OFF, 0);
	}

	// gx_utility_rectangle_define(&child, 130, 10, 130 + 60 - 1, 15);
	// custom_icon_create(&children_module.line_tips, GX_ID_NONE, parent, &child, GX_COLOR_ID_HONBOW_WHITE, 0);

	// operation tips
	gx_utility_rectangle_define(&child, 0, 0, 319, 87);
	gx_prompt_create(&children_module.operation_tips, NULL, parent, 0, GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE,
					 GX_ID_NONE, &child);
	gx_prompt_text_color_set(&children_module.operation_tips, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&children_module.operation_tips, GX_FONT_ID_SIZE_30);
	gx_widget_fill_color_set(&children_module.operation_tips, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
							 GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_hide(&children_module.operation_tips);
}

static GX_WIDGET *anim_screen_list[] = {
	(GX_WIDGET *)&control_center_window,
	(GX_WIDGET *)&root_window.root_window_wf_window,
	GX_NULL,
};
#define CONTROL_CENTER_TIMER_ID 3
#define CONTROL_CENTER_TIPS_TIMER_ID 4
extern uint8_t lcd_brightness_get(void);
extern VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
UINT control_center_window_event(GX_WINDOW *window, GX_EVENT *event_ptr)
{
	UINT rc = 0;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		gx_system_timer_start(window, CONTROL_CENTER_TIMER_ID, 50, 50);
		uint8_t brightness = lcd_brightness_get();
		children_module.bright_level = brightness / 50;
		break;
	}
	case GX_EVENT_HIDE:
		gx_system_timer_stop(window, CONTROL_CENTER_TIMER_ID);
		break;
	case GX_SIGNAL(BRIGHTNESS_ICON_ID, GX_EVENT_CLICKED): {
		extern int lcd_brightness_adjust(uint8_t value);
		if (children_module.bright_level <= 4) {
			children_module.bright_level++;
		} else {
			children_module.bright_level = 1;
		}
		lcd_brightness_adjust(children_module.bright_level * 50);
		gx_system_dirty_mark(&children_module.bright_ness);
		return 0;
	}
	case GX_SIGNAL(NIGHT_MODE_ICON_ID, GX_EVENT_CLICKED): {
		if (children_module.night_mode_on == true) {
			custom_icon_change_bg_color(&children_module.night_mode, GX_COLOR_ID_HONBOW_DARK_GRAY);
			children_module.night_mode_on = false;
			snprintf(children_module.tips_buff, sizeof(children_module.tips_buff), "night mode off");
		} else {
			custom_icon_change_bg_color(&children_module.night_mode, GX_COLOR_ID_HONBOW_BLUE);
			children_module.night_mode_on = true;
			snprintf(children_module.tips_buff, sizeof(children_module.tips_buff), "night mode on");
		}
		GX_STRING string;
		string.gx_string_ptr = children_module.tips_buff;
		string.gx_string_length = strlen(children_module.tips_buff);
		gx_prompt_text_set_ext(&children_module.operation_tips, &string);
		gx_widget_show(&children_module.operation_tips);
		gx_system_timer_start(window, CONTROL_CENTER_TIPS_TIMER_ID, 75, 0);
		return 0;
	}
	case GX_SIGNAL(BRIGHT_SCREEN_ON_ICON_ID, GX_EVENT_CLICKED): {
		if (children_module.auto_bright_screen == true) {
			custom_icon_change_bg_color(&children_module.bright_screen_switch, GX_COLOR_ID_HONBOW_DARK_GRAY);
			custom_icon_change_pixelmap(&children_module.bright_screen_switch,
										GX_PIXELMAP_ID_CONTROL_CENTER_BRIGHT_SCREEN_OFF);
			children_module.auto_bright_screen = false;
		} else {
			custom_icon_change_bg_color(&children_module.bright_screen_switch, GX_COLOR_ID_HONBOW_GREEN);
			custom_icon_change_pixelmap(&children_module.bright_screen_switch,
										GX_PIXELMAP_ID_CONTROL_CENTER_BRIGHT_SCREEN_ON);
			children_module.auto_bright_screen = true;
		}
		return 0;
	}
	case GX_SIGNAL(DISTURB_MODE_ICON_ID, GX_EVENT_CLICKED): {
		return 0;
	}
	case GX_SIGNAL(SET_UP_ICON_ID, GX_EVENT_CLICKED): {
		// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&app_setting_window);
		windows_mgr_page_jump((GX_WINDOW *)&root_window.root_window_home_window, (GX_WINDOW *)&app_setting_window,
							  WINDOWS_OP_SWITCH_NEW);
		return 0;
	}
	case GX_EVENT_TIMER:
		if (event_ptr->gx_event_payload.gx_event_timer_id == CONTROL_CENTER_TIMER_ID) {
			struct timeval tv;
			gettimeofday(&tv, NULL);
			time_t now = tv.tv_sec;
			struct tm *tm_now = gmtime(&now);
			static int tm_mday = 0xff;
			if (tm_mday != tm_now->tm_mday) {
				tm_mday = tm_now->tm_mday;
				snprintf(children_module.date_buff, sizeof(children_module.date_buff), "%d/%d ", tm_now->tm_mon + 1,
						 tm_now->tm_mday);
				char weekday[7][5] = {
					"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT",
				};
				strncat(children_module.date_buff, weekday[tm_now->tm_wday], 3);
				GX_STRING string;
				string.gx_string_ptr = children_module.date_buff;
				string.gx_string_length = strlen(children_module.date_buff);
				gx_prompt_text_set_ext(&children_module.date, &string);
			}
			static int tm_min = 0xff;
			if (tm_min != tm_now->tm_min) {
				tm_min = tm_now->tm_min;
				snprintf(children_module.time_buff, sizeof(children_module.time_buff), "%02d:%02d", tm_now->tm_hour,
						 tm_now->tm_min);
				GX_STRING string;
				string.gx_string_ptr = children_module.time_buff;
				string.gx_string_length = strlen(children_module.time_buff);
				gx_prompt_text_set_ext(&children_module.time, &string);
			}
			return 0;
		} else if (event_ptr->gx_event_payload.gx_event_timer_id == CONTROL_CENTER_TIPS_TIMER_ID) {
			gx_widget_hide(&children_module.operation_tips);
			gx_system_timer_stop(window, CONTROL_CENTER_TIPS_TIMER_ID);
			return 0;
		}
		break;
	default:
		break;
	}
	rc = gx_widget_event_process(window, event_ptr);
	return rc;
}