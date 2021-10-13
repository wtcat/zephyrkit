#include "gx_api.h"
#include "custom_rounded_button.h"
#include "custom_window_title.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "app_reminders_page_mgr.h"
#include "stdio.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include "guix_language_resources_custom.h"
#include "logging/log.h"

LOG_MODULE_DECLARE(guix_log);
#define NEXT_OR_FINISH_BUTTON_ID 3

static GX_WIDGET alarm_vib_duration_select_page;
static REMINDERS_EDIT_TYPE edit_type;
static REMINDER_INFO_T *alarm_info;

static Custom_title_icon_t icon_tips = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
};
static GX_PROMPT prompt_vib_select;
static GX_TEXT_SCROLL_WHEEL vib_duration_scroll_wheel;
static GX_PROMPT background;
static Custom_RoundedButton_TypeDef next_finish_button;
static int alarm_vib_sel_page_info_update(REMINDERS_EDIT_TYPE type, REMINDER_INFO_T *info);
extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "震动时长";
	case LANGUAGE_ENGLISH:
		return "VIB DU";
	default:
		return "VIB DU";
	}
}

static UINT alarm_time_sel_page_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static GX_VALUE gx_point_x_first;
	static GX_VALUE delta_x;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_HIDE: {
	} break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}
	case GX_EVENT_PEN_UP: {
		delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x >= 100) {
			app_reminders_page_jump(REMINDERS_PAGE_VIB_DURATION, REMINDERS_OP_CANCEL);
		}
		break;
	}
	case GX_EVENT_LANGUAGE_CHANGE:
		break;
	case GX_SIGNAL(NEXT_OR_FINISH_BUTTON_ID, GX_EVENT_CLICKED): {
		LOG_INF("button next clicked!\n");
		if (alarm_info != NULL) {
			INT index;
			gx_scroll_wheel_selected_get(&vib_duration_scroll_wheel, &index);

			if (index == 0) {
				alarm_info->vib_duration = 30;
			} else if (index == 1) {
				alarm_info->vib_duration = 60;
			} else {
				alarm_info->vib_duration = 90;
			}
		}
		app_reminders_page_jump(REMINDERS_PAGE_VIB_DURATION, REMINDERS_OP_NEXT_OR_FINISH);
		break;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

const static GX_CHAR *string_list[] = {"30", "60", "90"};

static UINT scroll_wheel_callback(GX_TEXT_SCROLL_WHEEL *wheel, INT id, GX_STRING *string)
{
	string->gx_string_ptr = string_list[id];
	string->gx_string_length = strlen(string->gx_string_ptr);
	return 0;
}
void reminders_vib_duration_sel_page_init(void)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);

	gx_widget_create(&alarm_vib_duration_select_page, NULL, NULL, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
					 &childSize);
	gx_widget_event_process_set(&alarm_vib_duration_select_page, alarm_time_sel_page_event_process);

	GX_WIDGET *parent = (GX_WIDGET *)&alarm_vib_duration_select_page;
	custom_window_title_create((GX_WIDGET *)parent, &icon_tips, &prompt_vib_select, NULL);

	GX_STRING string;
	string.gx_string_ptr = get_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&prompt_vib_select, &string);

	gx_utility_rectangle_define(&childSize, 106, 60, 106 + 109 - 1, 269);
	gx_text_scroll_wheel_create(&vib_duration_scroll_wheel, GX_NULL, parent, 3,
								GX_STYLE_BORDER_NONE | GX_STYLE_TRANSPARENT | GX_STYLE_ENABLED | GX_STYLE_TEXT_CENTER,
								GX_ID_NONE, &childSize);
	gx_text_scroll_wheel_font_set(&vib_duration_scroll_wheel, GX_FONT_ID_SIZE_60, GX_FONT_ID_SIZE_60);
	gx_text_scroll_wheel_text_color_set(&vib_duration_scroll_wheel, GX_COLOR_ID_DISABLED_FILL, GX_COLOR_ID_HONBOW_WHITE,
										GX_COLOR_ID_HONBOW_WHITE);
	gx_text_scroll_wheel_callback_set_ext(&vib_duration_scroll_wheel, scroll_wheel_callback);
	gx_scroll_wheel_row_height_set(&vib_duration_scroll_wheel, 70);
	if (parent) {
		gx_widget_attach(parent, &vib_duration_scroll_wheel);
	}

	gx_utility_rectangle_define(&childSize, 219, 158, 300, 158 + 30 - 1);

	gx_prompt_create(&background, NULL, parent, 0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 0,
					 &childSize);
	gx_prompt_text_color_set(&background, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&background, GX_FONT_ID_SIZE_30);
	string.gx_string_ptr = "s";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&background, &string);

	gx_utility_rectangle_define(&childSize, 12, 283, 12 + 296 - 1, 283 + 65 - 1);
	custom_rounded_button_create(&next_finish_button, NEXT_OR_FINISH_BUTTON_ID, parent, &childSize,
								 GX_COLOR_ID_HONBOW_GREEN, GX_PIXELMAP_ID_APP_ALARM_CHECK_BOX_SELECTED, NULL,
								 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
	app_reminders_page_reg(REMINDERS_PAGE_VIB_DURATION, &alarm_vib_duration_select_page,
						   alarm_vib_sel_page_info_update);
}

static int alarm_vib_sel_page_info_update(REMINDERS_EDIT_TYPE type, REMINDER_INFO_T *info)
{
	edit_type = type;
	alarm_info = info;

	return 0;
}