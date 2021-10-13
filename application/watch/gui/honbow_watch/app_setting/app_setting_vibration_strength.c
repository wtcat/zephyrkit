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
#include "custom_animation.h"
#include "app_setting_disp_page.h"
#include <logging/log.h>
#include "guix_language_resources_custom.h"
#include "custom_radiobox.h"
#include "view_service_custom.h"
#include "data_service_custom.h"
#include "setting_service_custom.h"

static GX_WIDGET curr_main_page;
static Custom_title_icon_t icon_title = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
};
static GX_PROMPT window_prompt;
static GX_PROMPT time_prompt;
static char time_prompt_buff[8];

struct page_info {
	Custom_RoundedButton_TypeDef button_type1;
	// GX_ICON icon_type1;
	CUSTOM_RADIOBOX radiobox_type1;

	Custom_RoundedButton_TypeDef button_type2;
	// icon_type2;
	CUSTOM_RADIOBOX radiobox_type2;
};

#define BUTTON1_ID 1
#define BUTTON2_ID 2
static struct page_info curr_page_content;

extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "振动强度";
	case LANGUAGE_ENGLISH:
		return "Vibration";
	default:
		return "Vibration";
	}
}

static const GX_CHAR *lauguage_ch_setting_element[2] = {"默认", "强"};
static const GX_CHAR *lauguage_en_setting_element[2] = {"Default", "Strong"};

static const GX_CHAR **setting_string[] = {
	[LANGUAGE_CHINESE] = lauguage_ch_setting_element, [LANGUAGE_ENGLISH] = lauguage_en_setting_element};
static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return setting_string[id];
}
extern void jump_setting_main_page(GX_WIDGET *now);
static UINT widget_event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static GX_VALUE gx_point_x_first;
	static GX_VALUE delta_x;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		custom_radiobox_status_change(&curr_page_content.radiobox_type2, 0);
		custom_radiobox_status_change(&curr_page_content.radiobox_type1, 1);
		break;
	}
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}
	case GX_EVENT_PEN_UP: {
		delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x >= 50) {
			jump_setting_main_page(&curr_main_page);
		}
		break;
	}
	case GX_SIGNAL(BUTTON1_ID, GX_EVENT_CLICKED): {
		custom_radiobox_status_change(&curr_page_content.radiobox_type2, 0);
		custom_radiobox_status_change(&curr_page_content.radiobox_type1, 1);
		break;
	}
	case GX_SIGNAL(BUTTON2_ID, GX_EVENT_CLICKED): {
		custom_radiobox_status_change(&curr_page_content.radiobox_type1, 0);
		custom_radiobox_status_change(&curr_page_content.radiobox_type2, 1);
		break;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}
extern void setting_main_page_jump_reg(GX_WIDGET *dst, SETTING_CHILD_PAGE_T id);
void app_setting_Vibration_strength_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_main_page, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	setting_main_page_jump_reg(&curr_main_page, SETTING_CHILD_PAGE_VIB_TUNER);

	// child page need init

	GX_WIDGET *parent = &curr_main_page;
	custom_window_title_create(parent, &icon_title, &window_prompt, NULL);

	GX_STRING string;
	string.gx_string_ptr = get_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&window_prompt, &string);

	string.gx_string_ptr = get_lauguage_string()[0];
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_utility_rectangle_define(&childSize, 12, 68, 12 + 296 - 1, 68 + 78 - 1);
	custom_rounded_button_create2(&curr_page_content.button_type1, BUTTON1_ID, parent, &childSize,
								  GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string, GX_FONT_ID_SIZE_30,
								  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);

#if 0
	gx_icon_create(&curr_page_content.icon_type1, NULL, &curr_page_content.button_type1.widget,
				   GX_PIXELMAP_ID_APP_SETTING_SWITCH_ACTIVE,
				   GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_HALIGN_CENTER, GX_ID_NONE, left + 246, top + 24);
#endif
	gx_utility_rectangle_define(&childSize, 258, 92, 258 + 30 - 1, 92 + 30 - 1);
	curr_page_content.radiobox_type1.color_id_select = GX_COLOR_ID_HONBOW_GREEN;
	curr_page_content.radiobox_type1.color_id_deselect = GX_COLOR_ID_HONBOW_GRAY;
	curr_page_content.radiobox_type1.color_id_background = GX_COLOR_ID_HONBOW_DARK_GRAY;

	custom_radiobox_create(&curr_page_content.radiobox_type1, &curr_page_content.button_type1.widget, &childSize);

	string.gx_string_ptr = get_lauguage_string()[1];
	string.gx_string_length = strlen(string.gx_string_ptr);

	gx_utility_rectangle_define(&childSize, 12, 156, 12 + 296 - 1, 156 + 78 - 1);
	custom_rounded_button_create2(&curr_page_content.button_type2, BUTTON2_ID, parent, &childSize,
								  GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string, GX_FONT_ID_SIZE_30,
								  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
#if 0
	gx_icon_create(&curr_page_content.icon_type2, NULL, &curr_page_content.button_type2.widget,
				   GX_PIXELMAP_ID_APP_SETTING_SWITCH_ACTIVE,
				   GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT | GX_STYLE_HALIGN_CENTER, GX_ID_NONE, left + 246, top + 24);
#endif
	gx_utility_rectangle_define(&childSize, 258, 181, 258 + 30 - 1, 181 + 30 - 1);
	curr_page_content.radiobox_type2.color_id_select = GX_COLOR_ID_HONBOW_GREEN;
	curr_page_content.radiobox_type2.color_id_deselect = GX_COLOR_ID_HONBOW_GRAY;
	curr_page_content.radiobox_type2.color_id_background = GX_COLOR_ID_HONBOW_DARK_GRAY;

	custom_radiobox_create(&curr_page_content.radiobox_type2, &curr_page_content.button_type2.widget, &childSize);

	gx_widget_event_process_set(parent, widget_event_processing_function);
}