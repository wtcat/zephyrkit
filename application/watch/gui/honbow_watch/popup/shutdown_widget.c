#include "ux/ux_api.h"

#include "popup/widget.h"

#include "utils/custom_icon_utils.h"
#include "utils/custom_slidebutton_basic.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"

#define PIXELMAP_CENTER_OFFSET 52

struct shutdown_widget {
		GX_WIDGET widget;
		Custom_SlideButton_TypeDef shutdown;
		Custom_SlideButton_TypeDef reboot;
		Custom_Icon_TypeDef cancel;
};

#define SHUTDOWN_BUTTON_ID 0xA001
#define REBOOT_BUTTON_ID   0xA002
#define CANCEL_BUTTON_ID   0xA003

static struct shutdown_widget popup_widget;
USTR_START(shutdown)
    _USTR(LANGUAGE_ENGLISH, "Sliding Shutdown", "Sliding Reboot"),
#ifndef _WIN32
    _USTR(LANGUAGE_CHINESE, "滑动关机", "滑动重启")
#endif
USTR_END(shutdown)

static UINT shutdown_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT ret = GX_SUCCESS;

	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(SHUTDOWN_BUTTON_ID, GX_EVENT_CLICKED):
		break;
	case GX_SIGNAL(REBOOT_BUTTON_ID, GX_EVENT_CLICKED):
		break;
	case GX_SIGNAL(CANCEL_BUTTON_ID, GX_EVENT_CLICKED):
		ret = ux_screen_pop_and_switch(widget, popup_hide_animation);
		break;
	default:
		ret = gx_widget_event_process(widget, event_ptr);
			break;
	}
	return ret;
}

GX_WIDGET *shutdown_widget_get(void)
{
	return &popup_widget.widget;
}

UINT shutdown_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
	struct shutdown_widget *swidget = &popup_widget;
	GX_RECTANGLE client;
	GX_STRING str;

	/* Parent widget */
    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_widget_create(&swidget->widget, "shutdown", parent, 
			GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&swidget->widget, GX_COLOR_ID_HONBOW_BLACK,
	GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&swidget->widget, shutdown_event_process);

	/* Shutdown button */
	gx_utility_rectangle_define(&client, 12, 70, 12 + 296, 70 + 80);
	shutdown_language_string_set(0, &str); 
	custom_slide_button_create(&swidget->shutdown, SHUTDOWN_BUTTON_ID, 
			&swidget->widget,
			&client, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
			GX_PIXELMAP_ID_APP_SETTING_BTN_SHUTDOWN, &str, GX_FONT_ID_SIZE_26,
			GX_COLOR_ID_HONBOW_WHITE, 5);

	/* Reboot button */
	gx_utility_rectangle_define(&client, 12, 160, 12 + 296, 240);
	shutdown_language_string_set(1, &str); 
	custom_slide_button_create(&swidget->reboot, REBOOT_BUTTON_ID, 
			&swidget->widget,
			&client, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
			GX_PIXELMAP_ID_APP_SETTING_BTN_RESTART, &str, GX_FONT_ID_SIZE_26,
			GX_COLOR_ID_HONBOW_WHITE, 5);

	/* Cancel button */
	gx_utility_rectangle_define(&client, 12, 268, 12 + 296, 348);
	custom_icon_create(&swidget->cancel, CANCEL_BUTTON_ID, &swidget->widget, 
			&client,
			GX_COLOR_ID_HONBOW_DARK_GRAY,
			GX_PIXELMAP_ID_APP_SETTING_BTN_S_CANCEL);
	if (ptr)
		*ptr = &swidget->widget;
	return GX_SUCCESS;
}
