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
#include "custom_radiobox.h"

LOG_MODULE_DECLARE(guix_log);

static GX_WIDGET alarm_rpt_sel_page;
static Custom_title_icon_t icon_tips = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
};
static GX_PROMPT prompt_rpt_select;
static GX_VERTICAL_LIST rpt_sel_list;
static GX_SCROLLBAR_APPEARANCE rpt_sel_list_scrollbar_properties = {
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
static GX_SCROLLBAR rpt_sel_list_scrollbar;
typedef struct APP_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	Custom_RoundedButton_TypeDef round_button;
	CUSTOM_RADIOBOX radio_box;
	uint8_t rpt_index;
} RPT_SEL_LIST_ROW;

#define RPT_SEL_LIST_VISIBLE_ROWS 3
static RPT_SEL_LIST_ROW rpt_sel_list_row_memory[RPT_SEL_LIST_VISIBLE_ROWS + 1];

static Custom_RoundedButton_TypeDef next_finish_button;

static REMINDERS_EDIT_TYPE edit_type;
static REMINDER_INFO_T *alarm_info;
#define NEXT_OR_FINISH_BUTTON_ID 1
static int alarm_rpt_sel_page_info_update(REMINDERS_EDIT_TYPE type, REMINDER_INFO_T *info);

extern GX_WINDOW_ROOT *root;

static const GX_CHAR *lauguage_ch_element[8] = {"仅一次", "周日", "周一", "周二", "周三", "周四", "周五", "周六"};
static const GX_CHAR *lauguage_en_element[8] = {"Only Once", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static const GX_CHAR **list_string[] = {
	[LANGUAGE_CHINESE] = lauguage_ch_element, [LANGUAGE_ENGLISH] = lauguage_en_element};
static const GX_CHAR **get_list_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return list_string[id];
}

static const GX_CHAR *get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "重复";
	case LANGUAGE_ENGLISH:
		return "Repeat";
	default:
		return "Repeat";
	}
}

static UINT alarm_rpt_sel_page_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
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
			app_reminders_page_jump(REMINDERS_PAGE_RPT_SEL, REMINDERS_OP_CANCEL);
		}
		break;
	}
	case GX_EVENT_LANGUAGE_CHANGE:
		break;
	case GX_SIGNAL(NEXT_OR_FINISH_BUTTON_ID, GX_EVENT_CLICKED): {
		LOG_INF("button next clicked!\n");
		if (alarm_info != NULL) {
		}
		app_reminders_page_jump(REMINDERS_PAGE_RPT_SEL, REMINDERS_OP_NEXT_OR_FINISH);
		break;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

static void update_radiobox_status(CUSTOM_RADIOBOX *radiobox, uint8_t rpt_list_index, REMINDER_INFO_T *alarm_info)
{
	if (alarm_info != NULL) {
		if (rpt_list_index == 0) {
			if (alarm_info->repeat_flags & (1 << 7)) {
				custom_radiobox_status_change(radiobox, GX_TRUE);
			} else {
				custom_radiobox_status_change(radiobox, GX_FALSE);
			}
		} else {
			if (alarm_info->repeat_flags & (1 << (rpt_list_index - 1))) {
				custom_radiobox_status_change(radiobox, GX_TRUE);
			} else {
				custom_radiobox_status_change(radiobox, GX_FALSE);
			}
		}
	}
}

static void update_alarm_info_rpt_flag(REMINDER_INFO_T *alarm_info, uint8_t rpt_list_index)
{
	if (alarm_info != NULL) {
		uint8_t *rpt_flags = &alarm_info->repeat_flags;

		if (rpt_list_index == 0) {
			if (!(*rpt_flags & (1 << 7))) {
				*rpt_flags = 0x80;
			}
		} else {

			if (*rpt_flags & (1 << 7)) {
				*rpt_flags &= 0x7f;
			}

			if (*rpt_flags & (1 << (rpt_list_index - 1))) {
				if (*rpt_flags != 1 << (rpt_list_index - 1)) {
					*rpt_flags &= ~(1 << (rpt_list_index - 1));
				}
			} else {
				*rpt_flags |= (1 << (rpt_list_index - 1));
			}
		}

		uint8_t index;
		for (uint8_t i = 0; i < RPT_SEL_LIST_VISIBLE_ROWS + 1; i++) {

			index = rpt_sel_list_row_memory[i].rpt_index;
			update_radiobox_status(&rpt_sel_list_row_memory[i].radio_box, index, alarm_info);
		}
	}
}

static UINT setting_main_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(1, GX_EVENT_CLICKED): {
		RPT_SEL_LIST_ROW *row = CONTAINER_OF(widget, RPT_SEL_LIST_ROW, widget);
		update_alarm_info_rpt_flag(alarm_info, row->rpt_index);
		return 0;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

static VOID rpt_sel_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	RPT_SEL_LIST_ROW *row = (RPT_SEL_LIST_ROW *)widget;
	GX_BOOL result;

	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 290, 77);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								 GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_event_process_set(&row->widget, setting_main_processing_function);

		childsize.gx_rectangle_bottom -= 10;
		GX_STRING init_string;
		init_string.gx_string_ptr = get_list_string()[index];
		init_string.gx_string_length = strlen(init_string.gx_string_ptr);
		custom_rounded_button_create2(&row->round_button, 1, &row->widget, &childsize, GX_COLOR_ID_HONBOW_DARK_GRAY,
									  GX_PIXELMAP_NULL, &init_string, GX_FONT_ID_SIZE_30,
									  CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
		gx_utility_rectangle_define(&childsize, 246, 24, 246 + 30 - 1, 24 + 30 - 1);
		row->radio_box.color_id_select = GX_COLOR_ID_HONBOW_GREEN;
		row->radio_box.color_id_deselect = GX_COLOR_ID_HONBOW_GRAY;
		row->radio_box.color_id_background = GX_COLOR_ID_HONBOW_DARK_GRAY;
		custom_radiobox_create(&row->radio_box, &row->widget, &childsize);

		row->rpt_index = index;
	}
}

static VOID rpt_sel_list_children_create(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < RPT_SEL_LIST_VISIBLE_ROWS + 1; count++) {
		rpt_sel_list_row_create(list, (GX_WIDGET *)&rpt_sel_list_row_memory[count], count);
	}
}
static VOID rpt_sel_list_callback(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	RPT_SEL_LIST_ROW *row = CONTAINER_OF(widget, RPT_SEL_LIST_ROW, widget);
	GX_STRING init_string;
	init_string.gx_string_ptr = get_list_string()[index];
	init_string.gx_string_length = strlen(init_string.gx_string_ptr);
	gx_prompt_text_set_ext(&row->round_button.desc, &init_string);

	row->rpt_index = index;
	update_radiobox_status(&row->radio_box, index, alarm_info);
}

void app_reminders_rpt_sel_page_init(void)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);

	gx_widget_create(&alarm_rpt_sel_page, NULL, NULL, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_widget_event_process_set(&alarm_rpt_sel_page, alarm_rpt_sel_page_event_process);

	GX_WIDGET *parent = (GX_WIDGET *)&alarm_rpt_sel_page;
	custom_window_title_create((GX_WIDGET *)parent, &icon_tips, &prompt_rpt_select, NULL);
	GX_STRING string;
	string.gx_string_ptr = get_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&prompt_rpt_select, &string);

	// vertical_list create
	gx_utility_rectangle_define(&childSize, 12, 60, 12 + 302 - 1, 283 - 1);
	gx_vertical_list_create(&rpt_sel_list, NULL, parent, 8, rpt_sel_list_callback,
							GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &childSize);
	gx_widget_fill_color_set(&rpt_sel_list, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
							 GX_COLOR_ID_HONBOW_BLACK);
	extern UINT custom_vertical_event_processing_function(GX_WIDGET * widget, GX_EVENT * event_ptr);
	gx_widget_event_process_set(&rpt_sel_list, custom_vertical_event_processing_function);

	rpt_sel_list_children_create(&rpt_sel_list);

	gx_vertical_scrollbar_create(&rpt_sel_list_scrollbar, NULL, &rpt_sel_list, &rpt_sel_list_scrollbar_properties,
								 GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB |
									 GX_SCROLLBAR_VERTICAL);
	gx_widget_fill_color_set(&rpt_sel_list_scrollbar, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
							 GX_COLOR_ID_HONBOW_DARK_GRAY);
	extern VOID custom_scrollbar_vertical_draw(GX_SCROLLBAR * scrollbar);
	gx_widget_draw_set(&rpt_sel_list_scrollbar, custom_scrollbar_vertical_draw);

	gx_utility_rectangle_define(&childSize, 12, 283, 12 + 296 - 1, 283 + 65 - 1);
	custom_rounded_button_create(&next_finish_button, NEXT_OR_FINISH_BUTTON_ID, parent, &childSize,
								 GX_COLOR_ID_HONBOW_GREEN, GX_PIXELMAP_ID_APP_ALARM__NEXT, NULL,
								 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
	app_reminders_page_reg(REMINDERS_PAGE_RPT_SEL, &alarm_rpt_sel_page, alarm_rpt_sel_page_info_update);
}

static int alarm_rpt_sel_page_info_update(REMINDERS_EDIT_TYPE type, REMINDER_INFO_T *info)
{
	edit_type = type;
	alarm_info = info;
	uint8_t index;

	for (uint8_t i = 0; i < RPT_SEL_LIST_VISIBLE_ROWS + 1; i++) {

		index = rpt_sel_list_row_memory[i].rpt_index;
		update_radiobox_status(&rpt_sel_list_row_memory[i].radio_box, index, alarm_info);
	}

	if (edit_type == REMINDERS_EDIT_TYPE_SINGLE) {
		next_finish_button.icon = GX_PIXELMAP_ID_APP_ALARM_CHECK_BOX_SELECTED;
	} else {
		next_finish_button.icon = GX_PIXELMAP_ID_APP_ALARM__NEXT;
	}
	gx_system_dirty_mark(&next_finish_button.widget);

	return 0;
}