#include "gx_api.h"
#include "custom_rounded_button.h"
#include "custom_window_title.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "app_alarm_page_mgr.h"
#include "stdio.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include "guix_language_resources_custom.h"
#include "logging/log.h"
#include "custom_radiobox.h"
#include "custom_widget_scrollable.h"
#include "custom_checkbox_basic.h"

LOG_MODULE_DECLARE(guix_log);

#define TIME_EDIT_BUTTON_ID 1
#define RPT_EDIT_BUTTON_ID 2
#define VIB_DU_EDIT_BUTTON_ID 3
#define REMIND_LATER_BUTTON_ID 4
#define SMART_AWAKE_BUTTON_ID 5

static ALARM_EDIT_TYPE edit_type;
static ALARM_INFO_T *alarm_info;

static GX_WIDGET curr_page;
static Custom_title_icon_t icon_tips = {
	.bg_color = GX_COLOR_ID_HONBOW_BLUE,
	.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
};
static GX_PROMPT prompt_edit_page;
static CUSTOM_SCROLLABLE_WIDGET scrollable_alarm_page;
static GX_SCROLLBAR_APPEARANCE scrollbar_properties = {
	8,						 /* scroll width                   */
	8,						 /* thumb width                    */
	0,						 /* thumb travel min               */
	0,						 /* thumb travel max               */
	4,						 /* thumb border style             */
	0,						 /* scroll fill pixelmap           */
	0,						 /* scroll thumb pixelmap          */
	0,						 /* scroll up pixelmap             */
	0,						 /* scroll down pixelmap           */
	GX_COLOR_ID_HONBOW_BLUE, /* scroll thumb color             */
	GX_COLOR_ID_HONBOW_BLUE, /* scroll thumb border color      */
	GX_COLOR_ID_HONBOW_BLUE, /* scroll button color            */
};
static GX_SCROLLBAR scrollbar;

// time edit button
static Custom_RoundedButton_TypeDef time_edit_button;
static GX_PROMPT time_edit_button_info_tips;
static GX_CHAR time_prompt_buff[10];
static GX_ICON time_edit_button_icon;

// rpt edit button
static Custom_RoundedButton_TypeDef rpt_edit_button;
static GX_PROMPT rpt_edit_button_info_tips;
static GX_CHAR rpt_prpmpt_buff[20];
static GX_ICON rpt_edit_button_icon;

// VIB DU edit button
static Custom_RoundedButton_TypeDef vib_du_edit_button;
static GX_PROMPT vib_du_edit_button_info_tips;
static GX_CHAR vib_du_prpmpt_buff[10];
static GX_ICON vib_du_edit_button_icon;

// reminder later switch button
static Custom_RoundedButton_TypeDef remind_late_switch_button;
static CUSTOM_CHECKBOX_BASIC remind_late_switch_checkbox;

// smart awake switch button
static Custom_RoundedButton_TypeDef smart_awake_switch_button;
static CUSTOM_CHECKBOX_BASIC smart_awake_switch_checkbox;

static int alarm_edit_page_info_update(ALARM_EDIT_TYPE type, ALARM_INFO_T *info);
extern VOID custom_scrollbar_vertical_draw(GX_SCROLLBAR *scrollbar);
static void alarm_edit_page_sync(void);
extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "编辑";
	case LANGUAGE_ENGLISH:
		return "Edit";
	default:
		return "Edit";
	}
}
static UINT scroll_page_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	CUSTOM_SCROLLABLE_WIDGET *custom_widget = CONTAINER_OF(widget, CUSTOM_SCROLLABLE_WIDGET, widget);
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		if (alarm_info != NULL) {
			alarm_edit_page_sync();
		}
		return custom_scrollable_widget_event_process(custom_widget, event_ptr);
	case GX_SIGNAL(TIME_EDIT_BUTTON_ID, GX_EVENT_CLICKED):
		LOG_INF("time edit button clicked!");
		app_alarm_page_jump(ALARM_PAGE_EDIT, ALARM_OP_TIME_SEL);
		break;
	case GX_SIGNAL(RPT_EDIT_BUTTON_ID, GX_EVENT_CLICKED):
		app_alarm_page_jump(ALARM_PAGE_EDIT, ALARM_OP_RPT_SEL);
		LOG_INF("rpt edit button clicked!");
		break;
	case GX_SIGNAL(VIB_DU_EDIT_BUTTON_ID, GX_EVENT_CLICKED):
		app_alarm_page_jump(ALARM_PAGE_EDIT, ALARM_OP_VIB_DURATION);
		LOG_INF("Vib Du edit button clicked!");
		break;
	case GX_SIGNAL(REMIND_LATER_BUTTON_ID, GX_EVENT_CLICKED):
		LOG_INF("remind later button clicked!");
		if (alarm_info->alarm_flag & (1 << REMIND_LATER_FALG_BIT)) {
			alarm_info->alarm_flag &= ~(1 << REMIND_LATER_FALG_BIT);
			if (remind_late_switch_checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
				custom_checkbox_basic_toggle(&remind_late_switch_checkbox);
			}
		} else {
			alarm_info->alarm_flag |= (1 << REMIND_LATER_FALG_BIT);
			if (!(remind_late_switch_checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED)) {
				custom_checkbox_basic_toggle(&remind_late_switch_checkbox);
			}
		}

		break;
	case GX_SIGNAL(SMART_AWAKE_BUTTON_ID, GX_EVENT_CLICKED):
		LOG_INF("smart awake button clicked!");
		if (alarm_info->alarm_flag & (1 << SMART_AWAKE_FLAG_BIT)) {
			alarm_info->alarm_flag &= ~(1 << SMART_AWAKE_FLAG_BIT);
			if (smart_awake_switch_checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
				custom_checkbox_basic_toggle(&smart_awake_switch_checkbox);
			}
		} else {
			alarm_info->alarm_flag |= (1 << SMART_AWAKE_FLAG_BIT);
			if (!(smart_awake_switch_checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED)) {
				custom_checkbox_basic_toggle(&smart_awake_switch_checkbox);
			}
		}
		break;
	default:
		return custom_scrollable_widget_event_process(custom_widget, event_ptr);
	}
	return status;
}
static UINT alarm_edit_page_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static GX_VALUE gx_point_x_first;
	static GX_VALUE delta_x;
	static GX_BOOL pen_down_valid = GX_FALSE;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_HIDE: {
	} break;
	case GX_EVENT_PEN_DOWN: {
		pen_down_valid = GX_TRUE;
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}
	case GX_EVENT_PEN_UP:
		if (GX_TRUE == pen_down_valid) {
			delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
			if (delta_x >= 100) {
				app_alarm_page_jump(ALARM_PAGE_EDIT, ALARM_OP_CANCEL);
			}
			pen_down_valid = GX_FALSE;
		}
		break;
	case GX_EVENT_LANGUAGE_CHANGE:
		break;
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

void app_alarm_edit_page_init(void)
{
	GX_RECTANGLE size;
	gx_utility_rectangle_define(&size, 0, 0, 319, 359);
	gx_widget_create(&curr_page, "alarm edit page", NULL, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &size);
	gx_widget_fill_color_set(&curr_page, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);
	gx_widget_event_process_set(&curr_page, alarm_edit_page_event_process);

	custom_window_title_create((GX_WIDGET *)&curr_page, &icon_tips, &prompt_edit_page, NULL);
	GX_STRING string;
	string.gx_string_ptr = get_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&prompt_edit_page, &string);

	gx_utility_rectangle_define(&size, 12, 58, 309, 339);
	custom_scrollable_widget_create(&scrollable_alarm_page, &curr_page, &size);
	gx_widget_event_process_set(&scrollable_alarm_page, scroll_page_event_process);

	gx_vertical_scrollbar_create(&scrollbar, NULL, &scrollable_alarm_page, &scrollbar_properties,
								 GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB |
									 GX_SCROLLBAR_VERTICAL);
	gx_widget_draw_set(&scrollbar, custom_scrollbar_vertical_draw);
	gx_widget_fill_color_set(&scrollbar, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
							 GX_COLOR_ID_HONBOW_DARK_GRAY);

	// show main edit info
	GX_WIDGET *parent = (GX_WIDGET *)&scrollable_alarm_page;

	GX_STRING init_string;
	init_string.gx_string_ptr = "Time";
	init_string.gx_string_length = strlen(init_string.gx_string_ptr);
	gx_utility_rectangle_define(&size, 12, 58, 12 + 292 - 1, 58 + 78 - 1);
	custom_rounded_button_create2(&time_edit_button, TIME_EDIT_BUTTON_ID, parent, &size, GX_COLOR_ID_HONBOW_DARK_GRAY,
								  GX_PIXELMAP_NULL, &init_string, GX_FONT_ID_SIZE_30, CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
	gx_utility_rectangle_define(&size, 195, 58, 195 + 67 - 1, 58 + 78 - 1);
	gx_prompt_create(&time_edit_button_info_tips, NULL, &time_edit_button, GX_ID_NONE,
					 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, GX_ID_NONE, &size);
	gx_prompt_font_set(&time_edit_button_info_tips, GX_FONT_ID_SIZE_26);
	gx_prompt_text_color_set(&time_edit_button_info_tips, GX_COLOR_ID_HONBOW_GRAY_40_PERCENT,
							 GX_COLOR_ID_HONBOW_GRAY_40_PERCENT, GX_COLOR_ID_HONBOW_GRAY_40_PERCENT);
	init_string.gx_string_ptr = "00:00";
	init_string.gx_string_length = strlen(init_string.gx_string_ptr);
	gx_prompt_text_set_ext(&time_edit_button_info_tips, &init_string);
	gx_icon_create(&time_edit_button_icon, NULL, &time_edit_button, GX_PIXELMAP_ID_APP_ALARM_NEXT, GX_STYLE_ENABLED,
				   GX_ID_NONE, 266, 84);

	init_string.gx_string_ptr = "Repeat";
	init_string.gx_string_length = strlen(init_string.gx_string_ptr);
	gx_utility_rectangle_define(&size, 12, 146, 12 + 292 - 1, 146 + 78 - 1);
	custom_rounded_button_create2(&rpt_edit_button, RPT_EDIT_BUTTON_ID, parent, &size, GX_COLOR_ID_HONBOW_DARK_GRAY,
								  GX_PIXELMAP_NULL, &init_string, GX_FONT_ID_SIZE_30, CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
	gx_utility_rectangle_define(&size, 165, 58 + 88, 165 + 97 - 1, 58 + 88 + 78 - 1);
	gx_prompt_create(&rpt_edit_button_info_tips, NULL, &rpt_edit_button, GX_ID_NONE,
					 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_RIGHT, GX_ID_NONE, &size);
	gx_prompt_font_set(&rpt_edit_button_info_tips, GX_FONT_ID_SIZE_26);
	gx_prompt_text_color_set(&rpt_edit_button_info_tips, GX_COLOR_ID_HONBOW_GRAY_40_PERCENT,
							 GX_COLOR_ID_HONBOW_GRAY_40_PERCENT, GX_COLOR_ID_HONBOW_GRAY_40_PERCENT);
	init_string.gx_string_ptr = "00:00";
	init_string.gx_string_length = strlen(init_string.gx_string_ptr);
	gx_prompt_text_set_ext(&rpt_edit_button_info_tips, &init_string);
	gx_icon_create(&rpt_edit_button_icon, NULL, &rpt_edit_button, GX_PIXELMAP_ID_APP_ALARM_NEXT, GX_STYLE_ENABLED,
				   GX_ID_NONE, 266, 84 + 88);

	init_string.gx_string_ptr = "Vib Du";
	init_string.gx_string_length = strlen(init_string.gx_string_ptr);
	gx_utility_rectangle_define(&size, 12, 234, 12 + 292 - 1, 234 + 78 - 1);
	custom_rounded_button_create2(&vib_du_edit_button, VIB_DU_EDIT_BUTTON_ID, parent, &size,
								  GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &init_string, GX_FONT_ID_SIZE_30,
								  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
	gx_utility_rectangle_define(&size, 195, 58 + 88 + 88, 195 + 67 - 1, 58 + 88 + 88 + 78 - 1);
	gx_prompt_create(&vib_du_edit_button_info_tips, NULL, &vib_du_edit_button, GX_ID_NONE,
					 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, GX_ID_NONE, &size);
	gx_prompt_font_set(&vib_du_edit_button_info_tips, GX_FONT_ID_SIZE_26);
	gx_prompt_text_color_set(&vib_du_edit_button_info_tips, GX_COLOR_ID_HONBOW_GRAY_40_PERCENT,
							 GX_COLOR_ID_HONBOW_GRAY_40_PERCENT, GX_COLOR_ID_HONBOW_GRAY_40_PERCENT);
	init_string.gx_string_ptr = "00:00";
	init_string.gx_string_length = strlen(init_string.gx_string_ptr);
	gx_prompt_text_set_ext(&vib_du_edit_button_info_tips, &init_string);
	gx_icon_create(&vib_du_edit_button_icon, NULL, &vib_du_edit_button, GX_PIXELMAP_ID_APP_ALARM_NEXT, GX_STYLE_ENABLED,
				   GX_ID_NONE, 266, 84 + 88 + 88);

	init_string.gx_string_ptr = "Remind later";
	init_string.gx_string_length = strlen(init_string.gx_string_ptr);
	gx_utility_rectangle_define(&size, 12, 322, 12 + 292 - 1, 322 + 78 - 1);
	custom_rounded_button_create2(&remind_late_switch_button, REMIND_LATER_BUTTON_ID, parent, &size,
								  GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &init_string, GX_FONT_ID_SIZE_30,
								  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
	CUSTOM_CHECKBOX_BASIC_INFO info;
	info.background_checked = GX_COLOR_ID_BLUE;
	info.background_uchecked = GX_COLOR_ID_HONBOW_GRAY;
	info.rolling_ball_color = GX_COLOR_ID_HONBOW_WHITE;
	info.widget_id = 1;
	info.pos_offset = 5;
	gx_utility_rectangle_define(&size, 222, 343, 222 + 66 - 1, 343 + 36 - 1);
	custom_checkbox_basic_create(&remind_late_switch_checkbox, (GX_WIDGET *)&remind_late_switch_button, &info, &size);

	init_string.gx_string_ptr = "Smart awake";
	init_string.gx_string_length = strlen(init_string.gx_string_ptr);
	gx_utility_rectangle_define(&size, 12, 412, 12 + 292 - 1, 412 + 78 - 1);
	custom_rounded_button_create2(&smart_awake_switch_button, SMART_AWAKE_BUTTON_ID, parent, &size,
								  GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &init_string, GX_FONT_ID_SIZE_30,
								  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
	gx_utility_rectangle_define(&size, 222, 343 + 88, 222 + 66 - 1, 343 + 88 + 36 - 1);
	custom_checkbox_basic_create(&smart_awake_switch_checkbox, (GX_WIDGET *)&smart_awake_switch_button, &info, &size);

	app_alarm_page_reg(ALARM_PAGE_EDIT, &curr_page, alarm_edit_page_info_update);
}

static GX_CHAR *day_tips[7] = {"S", "M", "T", "W", "T", "F", "S"};
static void alarm_edit_page_sync(void)
{
	GX_STRING value;
	if (alarm_info != NULL) { // start sync info to disp
		snprintf(time_prompt_buff, sizeof(time_prompt_buff), "%02d:%02d", alarm_info->hour, alarm_info->min);
		value.gx_string_ptr = time_prompt_buff;
		value.gx_string_length = strlen(value.gx_string_ptr);
		gx_prompt_text_set_ext(&time_edit_button_info_tips, &value);

		if (alarm_info->repeat_flags == 0x80) {
			snprintf(rpt_prpmpt_buff, sizeof(rpt_prpmpt_buff), "only once");
		} else {
			memset(rpt_prpmpt_buff, 0, sizeof(rpt_prpmpt_buff));
			for (uint8_t repeat_index = 0; repeat_index < 7; repeat_index++) {
				if (alarm_info->repeat_flags & (1 << repeat_index)) {
					if (strlen(rpt_prpmpt_buff) < 6) {
						strcat(rpt_prpmpt_buff, day_tips[repeat_index]);
						strcat(rpt_prpmpt_buff, " ");
					} else {
						strcat(rpt_prpmpt_buff, "...");
						break;
					}
				}
			}
		}
		value.gx_string_ptr = rpt_prpmpt_buff;
		value.gx_string_length = strlen(value.gx_string_ptr);
		gx_prompt_text_set_ext(&rpt_edit_button_info_tips, &value);

		snprintf(vib_du_prpmpt_buff, sizeof(vib_du_prpmpt_buff), "%ds", alarm_info->vib_duration);
		value.gx_string_ptr = vib_du_prpmpt_buff;
		value.gx_string_length = strlen(value.gx_string_ptr);
		gx_prompt_text_set_ext(&vib_du_edit_button_info_tips, &value);

		if (alarm_info->alarm_flag & (1 << REMIND_LATER_FALG_BIT)) {
			if (!(remind_late_switch_checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED)) {
				custom_checkbox_basic_toggle(&remind_late_switch_checkbox);
			}
		} else {
			if (remind_late_switch_checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
				custom_checkbox_basic_toggle(&remind_late_switch_checkbox);
			}
		}

		if (alarm_info->alarm_flag & (1 << SMART_AWAKE_FLAG_BIT)) {
			if (!(smart_awake_switch_checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED)) {
				custom_checkbox_basic_toggle(&smart_awake_switch_checkbox);
			}
		} else {
			if (smart_awake_switch_checkbox.widget.gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
				custom_checkbox_basic_toggle(&smart_awake_switch_checkbox);
			}
		}
	}
}
static int alarm_edit_page_info_update(ALARM_EDIT_TYPE type, ALARM_INFO_T *info)
{
	edit_type = type;
	alarm_info = info;

	alarm_edit_page_sync();
	return 0;
}