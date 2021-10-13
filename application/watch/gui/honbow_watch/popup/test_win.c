#include "ux/ux_api.h"
#include "gx_widget.h"
#include "gx_window.h"
#include "gx_system.h"
#include "gx_scrollbar.h"

#include "popup/widget.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"

#include "app_list/app_list_define.h"

#define BTN_EVENT(id) GX_SIGNAL(id+1, GX_EVENT_CLICKED)

enum button_id {
	BTN_NEW_MESSAGE,
	BTN_OTA_START,
	BTN_LOW_BATTERY,
	BTN_LOW_BATTERY_1,
	BTN_CHARGING,
	BTN_GOAL_ARRIVED,
	BTN_SIT_TIMEOUT,
	BTN_OVERHIGH_HEARTRATE,
	BTN_ALARM_ARRIVED,
	BTN_ALARM_ARRIVED1,
	BTN_ALARM_ARRIVED2,
	BTN_ALARM_ARRIVED3,
	BTN_SHUTDOWN_ANI,
	BTN_SPORT,
	BTN_CALLING,
	BTN_MAX
};

struct pop_pannel {
	GX_WIDGET parent;
	GX_WIDGET widget;
	GX_VERTICAL_LIST list;
	UX_TEXT_BUTTON buttons[BTN_MAX];
};

struct button_info {
	const char *name;
};

static struct pop_pannel pop_pannel;
static const struct button_info buttons_info[] = {
	[BTN_NEW_MESSAGE]          = {"new-message"},
	[BTN_OTA_START]            = {"ota"},
	[BTN_LOW_BATTERY]          = {"low-battery5"},
	[BTN_LOW_BATTERY_1]        = {"low-battery10"},
	[BTN_CHARGING]             = {"charging"},
	[BTN_GOAL_ARRIVED]         = {"goal-1"},
	[BTN_SIT_TIMEOUT]          = {"5"},
	[BTN_OVERHIGH_HEARTRATE]   = {"heart-rate"},
	[BTN_ALARM_ARRIVED]        = {"alarm"},
	[BTN_ALARM_ARRIVED1]       = {"goal-2"},
	[BTN_ALARM_ARRIVED2]       = {"goal-3"},
	[BTN_ALARM_ARRIVED3]       = {"goal-4"},
	[BTN_SHUTDOWN_ANI]         = {"shutdown-ani"},
	[BTN_SPORT]                = {"sport"},
	[BTN_CALLING]			   = {"calling"}
};


static UINT pop_widget_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	extern GX_WIDGET *message_pop_widget_get(void);
	GX_ANIMATION_INFO info;

	move_animation_setup(&info, 1, GX_ANIMATION_ELASTIC_EASE_IN_OUT,
		_ux_dir_down, 10);

	switch (event_ptr->gx_event_type) {
	case BTN_EVENT(BTN_NEW_MESSAGE):
		pop_widget_fire(widget, message_pop_widget_get(), &info);
		break;
	case BTN_EVENT(BTN_OTA_START):
		pop_widget_fire(widget, ota_widget_get(), &info);
		break;
	case BTN_EVENT(BTN_LOW_BATTERY):
		pop_widget_fire(widget, lowbat_widget_get(5), &info);
		break;
	case BTN_EVENT(BTN_LOW_BATTERY_1):
		pop_widget_fire(widget, lowbat_widget_get(10), &info);
		break;
	case BTN_EVENT(BTN_CHARGING):
		pop_widget_fire(widget, charging_widget_get(60), &info);
		break;
	case BTN_EVENT(BTN_GOAL_ARRIVED):
		pop_widget_fire(widget, goal_arrived_widget_get(GOAL_1, 10000), &info);
		break;
	case BTN_EVENT(BTN_SIT_TIMEOUT):
		pop_widget_fire(widget, sedentary_widget_get(8), &info);
		break;
	case BTN_EVENT(BTN_OVERHIGH_HEARTRATE):
		pop_widget_fire(widget, hrov_widget_get(220), &info);
		break;
	case BTN_EVENT(BTN_ALARM_ARRIVED):
		pop_widget_fire(widget, alarm_remind_widget_get(7, 0, GX_TRUE), &info);
		break;
	case BTN_EVENT(BTN_ALARM_ARRIVED1):
		pop_widget_fire(widget, goal_arrived_widget_get(GOAL_2, 30), &info);
		break;
	case BTN_EVENT(BTN_ALARM_ARRIVED2):
		pop_widget_fire(widget, goal_arrived_widget_get(GOAL_3, 5), &info);
		break;
	case BTN_EVENT(BTN_ALARM_ARRIVED3):
		pop_widget_fire(widget, goal_arrived_widget_get(GOAL_4, 0), &info);
		break;
	case BTN_EVENT(BTN_SHUTDOWN_ANI):
		pop_widget_fire(widget, shutdown_ani_widget_get(), &info);
		break;
	case BTN_EVENT(BTN_SPORT):
		pop_widget_fire(widget, sport_widget_get(), &info);
		break;
	case BTN_EVENT(BTN_CALLING):
		pop_widget_fire(widget, ringing_widget_get(), &info);
	default:
		break;
	}
	return _gx_widget_event_process(widget, event_ptr);
}

static UINT test_win_vertical_list_children_position(GX_VERTICAL_LIST *vertical_list)
{
	GX_RECTANGLE childsize = vertical_list->gx_window_client;
	GX_WIDGET *child = vertical_list->gx_widget_first_child;
	INT index = vertical_list->gx_vertical_list_top_index;
	GX_VALUE height;
	GX_VALUE client_height;

	vertical_list->gx_vertical_list_child_height = 0;
	vertical_list->gx_vertical_list_child_count = 0;
	while (child) {
		if (!(child->gx_widget_status & GX_STATUS_NONCLIENT)) {
			vertical_list->gx_vertical_list_child_count++;
			if (!(child->gx_widget_id)) {
				child->gx_widget_id = (USHORT)(LIST_CHILD_ID_START +
					vertical_list->gx_vertical_list_child_count);
			}
			if (index == vertical_list->gx_vertical_list_selected)
				child->gx_widget_style |= GX_STYLE_DRAW_SELECTED;
			else
				child->gx_widget_style &= ~GX_STYLE_DRAW_SELECTED;
			index++;
			child->gx_widget_status &= ~GX_STATUS_ACCEPTS_FOCUS;
			_gx_widget_height_get(child, &height);
			if (height > vertical_list->gx_vertical_list_child_height)
				vertical_list->gx_vertical_list_child_height = height;
			/* move this child into position */
			childsize.gx_rectangle_bottom = (GX_VALUE)(childsize.gx_rectangle_top + height - 1);
			_gx_widget_resize(child, &childsize);
			childsize.gx_rectangle_top = (GX_VALUE)(childsize.gx_rectangle_bottom + 10);
		}
		child = child->gx_widget_next;
	}

	/* calculate number of visible rows, needed for scrolling info */
	if (vertical_list->gx_vertical_list_child_height > 0) {
		_gx_window_client_height_get((GX_WINDOW *)vertical_list, &client_height);
		vertical_list->gx_vertical_list_visible_rows =
			(GX_VALUE)((client_height + vertical_list->gx_vertical_list_child_height - 1) /
				vertical_list->gx_vertical_list_child_height);
	}
	else {
		vertical_list->gx_vertical_list_visible_rows = 1;
	}
	return GX_SUCCESS;
}

static UINT test_win_list_event_process(GX_VERTICAL_LIST *list, GX_EVENT *event_ptr)
{
	GX_SCROLLBAR *pScroll;
	UINT ret = GX_SUCCESS;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		ret = _gx_window_event_process((GX_WINDOW *)list, event_ptr);
		if (!list->gx_vertical_list_child_count)
			test_win_vertical_list_children_position(list);
		_gx_window_scrollbar_find((GX_WINDOW *)list, GX_TYPE_VERTICAL_SCROLL, &pScroll);
		if (pScroll)
			_gx_scrollbar_reset(pScroll, GX_NULL);
		break;
	default:
		ret = _gx_vertical_list_event_process(list, event_ptr);
		break;
	}
	return ret;
}

static struct pop_pannel *pop_pannel_create(GX_WIDGET *parent)
{
	struct pop_pannel *pannel = &pop_pannel;
	GX_RECTANGLE client;

	gx_utility_rectangle_define(&client, 0, 0, 320, 360);
	gx_widget_create(&pannel->widget, "pop-test", parent,
		GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
	gx_widget_fill_color_set(&pannel->widget, GX_COLOR_ID_HONBOW_BLACK,
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&pannel->widget, pop_widget_event_process);

	gx_utility_rectangle_resize(&client, -10);
	gx_vertical_list_create(&pannel->list, "test-list", &pannel->widget, 1,
		NULL, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT,
		GX_ID_NONE, &client);
	gx_widget_fill_color_set(&pannel->list, GX_COLOR_ID_HONBOW_BLACK,
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&pannel->list, test_win_list_event_process);

	for (int i = 0; i < BTN_MAX; i++) {
		UX_TEXT_BUTTON *button = pannel->buttons + i;
		GX_STRING str;

		gx_utility_rectangle_define(&client, 20, 10, 320-20, 10+60);
		ux_text_button_create(button, "tst-button", (GX_WIDGET *)&pannel->list, 0,
			GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_GREEN, i + 1,
			GX_STYLE_ENABLED, &client, NULL);
		gx_text_button_font_set(&button->button, GX_FONT_ID_SIZE_26);
		gx_text_button_text_color_set(&button->button,
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
		str.gx_string_ptr = buttons_info[i].name;
		str.gx_string_length = strlen(str.gx_string_ptr);
		gx_text_button_text_set_ext(&button->button, &str);
	}

	return pannel;
}

UINT pop_test_window_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
#ifdef _WIN32
	static GX_WIDGET *screen_stack[2 * 10];
#endif
	struct pop_pannel *pannel = &pop_pannel;
	GX_RECTANGLE client;

	gx_utility_rectangle_define(&client, 0, 0, 320, 360);
	gx_widget_create(&pannel->parent, "test-win", parent,
		GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
	gx_widget_fill_color_set(&pannel->parent, GX_COLOR_ID_HONBOW_BLACK,
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

	pop_pannel_create(&pannel->parent);
#ifdef _WIN32
	gx_system_screen_stack_create(screen_stack, sizeof(screen_stack));
	app_install(APP_ID_05_ALARM_BREATHE,
		(GX_WIDGET *)&pannel->parent,
		GX_PIXELMAP_ID_APP_LIST_ICON_ALARM_90_90,
		GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY,
		NULL);
#endif
	if (ptr)
		*ptr = &pannel->parent;
	return GX_SUCCESS;
}

#ifndef _WIN32
// GUI_APP_DEFINE(today,
// 	APP_ID_05_ALARM_BREATHE,
// 	(GX_WIDGET *)&pop_pannel.parent,
// 	GX_PIXELMAP_ID_APP_LIST_ICON_BREATHE_90_90,
// 	GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
#endif