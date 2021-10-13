#include <stdlib.h>

#include "ux/ux_api.h"
#include "gx_widget.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"

#define LATER_ON_BUTTON_ID 0x1001
#define STOP_BUTTON_ID     0x1002

struct alarm_remind_widget {
    GX_WIDGET widget;
    GX_PROMPT time;
    GX_PROMPT time_ext;
    GX_PROMPT alarm;
    UX_TEXT_BUTTON later_on;
    UX_TEXT_BUTTON stop;
};

static struct alarm_remind_widget alarm_remind_widget;

USTR_START(alarm)
    _USTR(LANGUAGE_ENGLISH, "Alarm", "Later On", "Stop"),
    _USTR(LANGUAGE_CHINESE, "闹钟", "稍后", "停止")
USTR_END(alarm)

static UINT alarm_remind_widget_event_process(struct alarm_remind_widget *aw,
    GX_EVENT *event_ptr)
{
    switch (event_ptr->gx_event_type) {
    case GX_SIGNAL(LATER_ON_BUTTON_ID, GX_EVENT_CLICKED):

        break;
    case GX_SIGNAL(STOP_BUTTON_ID, GX_EVENT_CLICKED):
        event_ptr->gx_event_type = GX_EVENT_KEY_UP;
        event_ptr->gx_event_payload.gx_event_ushortdata[0] = GX_KEY_HOME;
        break;
    default:
        break;
    }
    return pop_widget_event_process_wrapper(&aw->widget, event_ptr,
        400, _gx_widget_event_process, _ux_move_up_animation);
}

GX_WIDGET *alarm_remind_widget_get(GX_VALUE hour, GX_VALUE min, GX_BOOL use_hour12)
{
    struct alarm_remind_widget *aw = &alarm_remind_widget;
    static char textbuffer[5];
    GX_STRING time_str;
    GX_STRING str;

    textbuffer[0] = hour / 10 + '0';
    textbuffer[1] = hour % 10 + '0';
    textbuffer[2] = ':';
    textbuffer[3] = min / 10 + '0';
    textbuffer[4] = min / 10 + '0';
    time_str.gx_string_ptr = textbuffer;
    time_str.gx_string_length = 5;
    gx_prompt_text_set_ext(&aw->time, &time_str);
    if (use_hour12) {
        if (hour >= 0 && hour < 12) {
            str.gx_string_ptr = "AM";
            str.gx_string_length = 2;
        } else {
            str.gx_string_ptr = "PM";
            str.gx_string_length = 2;
        }
        gx_prompt_text_set_ext(&aw->time_ext, &str);
    } else {
        _gx_widget_hide((GX_WIDGET *)&aw->time_ext);
    }
    return &aw->widget;
}

UINT alarm_remind_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
    struct alarm_remind_widget *aw = &alarm_remind_widget;
    GX_RECTANGLE client;

    /* Setup widget base class */
    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_widget_create(&aw->widget, "charging", parent,
        GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&aw->widget, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_event_process_set(&aw->widget, alarm_remind_widget_event_process);

    /* time */
    gx_utility_rectangle_define(&client, 78, 75, 320 - 78, 75 + 81);
    gx_prompt_create(&aw->time, "time", &aw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(&aw->time, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&aw->time, GX_FONT_ID_SIZE_60);

    /* time extern */
    gx_utility_rectangle_define(&client, 245, 108, 245 + 40, 108 + 30);
    gx_prompt_create(&aw->time_ext, "time-ext", &aw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT, 0, &client);
    gx_prompt_text_color_set(&aw->time_ext, GX_COLOR_ID_HONBOW_GRAY,
        GX_COLOR_ID_HONBOW_GRAY, GX_COLOR_ID_HONBOW_GRAY);
    gx_prompt_font_set(&aw->time_ext, GX_FONT_ID_SIZE_26);

    /* alarm */
    gx_utility_rectangle_define(&client, 122, 135, 320-122, 135+45);
    gx_prompt_create(&aw->alarm, "alarm", &aw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT, 0, &client);
    gx_prompt_text_color_set(&aw->alarm, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&aw->alarm, GX_FONT_ID_SIZE_26);
    ux_widget_string_set(alarm, 0, &aw->alarm, gx_prompt_text_set_ext);
    
    /* Later-on button*/
    gx_utility_rectangle_define(&client, 13, 208, 13 + 296, 208 + 65);
    ux_text_button_create(&aw->later_on, "later-on", &aw->widget, 0,
        GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_GREEN, LATER_ON_BUTTON_ID,
        GX_STYLE_ENABLED, &client, NULL);
    gx_text_button_font_set((GX_TEXT_BUTTON *)&aw->later_on, GX_FONT_ID_SIZE_26);
    gx_text_button_text_color_set((GX_TEXT_BUTTON *)&aw->later_on,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    ux_widget_string_set(alarm, 1, &aw->later_on, gx_text_button_text_set_ext);
 
    /* Stop button*/
    gx_utility_rectangle_define(&client, 13, 283, 13 + 296, 283 + 65);
    ux_text_button_create(&aw->stop, "stop", &aw->widget, 0,
        GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_GREEN, STOP_BUTTON_ID,
        GX_STYLE_ENABLED, &client, NULL);
    gx_text_button_font_set((GX_TEXT_BUTTON *)&aw->stop, GX_FONT_ID_SIZE_26);
    gx_text_button_text_color_set((GX_TEXT_BUTTON *)&aw->stop,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    ux_widget_string_set(alarm, 2, &aw->stop, gx_text_button_text_set_ext);
    if (ptr)
        *ptr = &aw->widget;
    return GX_SUCCESS;
}
