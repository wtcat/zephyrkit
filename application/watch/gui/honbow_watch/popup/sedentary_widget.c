#include <stdlib.h>

#include "ux/ux_api.h"
#include "gx_widget.h"

#include "popup/widget.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"

struct sedentary_widget {
    GX_WIDGET widget;
    GX_RECTANGLE client;
    GX_RESOURCE_ID icon;
    GX_PROMPT time;
    GX_PROMPT unit;
    GX_PROMPT tips;
};

static struct sedentary_widget sedentary_widget;
USTR_START(sedentary)
    _USTR(LANGUAGE_ENGLISH, "Hour", "Get up and move!"),
#ifndef _WIN32
    _USTR(LANGUAGE_CHINESE, "小时", "起来活动一下!")
#endif
USTR_END(sedentary)

static void sedentary_widget_draw(struct sedentary_widget *sw)
{
    _gx_widget_background_draw(&sw->widget);
    ux_rectangle_pixelmap_draw(&sw->client, sw->icon,
        GX_STYLE_HALIGN_CENTER | GX_STYLE_VALIGN_CENTER);
    _gx_widget_children_draw(&sw->widget);
}

static UINT sedentary_widget_event_process(struct sedentary_widget *sw,
    GX_EVENT *event_ptr)
{
    return pop_widget_event_process_wrapper(&sw->widget, event_ptr,
        200, _gx_widget_event_process, popup_hide_animation);
}

GX_WIDGET *sedentary_widget_get(INT value)
{
    struct sedentary_widget *sw = &sedentary_widget;
    static char buffer[5] = {0, 0, '/', '1', '2'};
    GX_STRING str;
    int i = 2;

    while (value) {
        buffer[--i] = value % 10 + '0';
        value /= 10;
    }
    str.gx_string_ptr = &buffer[i];
    str.gx_string_length = 2 - i + 3;
    gx_prompt_text_set_ext(&sw->time, &str);
    return &sw->widget;
}

UINT sedentary_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
    struct sedentary_widget *sw = &sedentary_widget;
    GX_RECTANGLE client;

    /* Setup widget base class */
    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_widget_create(&sw->widget, "charging", parent,
        GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&sw->widget, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_draw_set(&sw->widget, sedentary_widget_draw);
    gx_widget_event_process_set(&sw->widget, sedentary_widget_event_process);

    /* Set icon */
    gx_utility_rectangle_define(&sw->client, 99, 63, 320-127, 360-189);
    sw->icon = GX_PIXELMAP_ID_SEDENTARY_REMIND;

    /* time */
    gx_utility_rectangle_define(&client, 89, 189, 89+97, 189+46);
    gx_prompt_create(&sw->time, "time", &sw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(&sw->time, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&sw->time, GX_FONT_ID_SIZE_30);

    /* Heartrate unit */
    gx_utility_rectangle_define(&client, 190, 199, 190 + 61, 199 + 30);
    gx_prompt_create(&sw->unit, "unit", &sw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(&sw->unit, GX_COLOR_ID_HONBOW_GRAY,
        GX_COLOR_ID_HONBOW_GRAY, GX_COLOR_ID_HONBOW_GRAY);
    gx_prompt_font_set(&sw->unit, GX_FONT_ID_SIZE_26);
    ux_widget_string_set(sedentary, 0, &sw->unit, gx_prompt_text_set_ext);

    /* Remind tips */
    gx_utility_rectangle_define(&client, 55, 251, 55+210, 251 + 45);
    gx_prompt_create(&sw->tips, "tips", &sw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(&sw->tips, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&sw->tips, GX_FONT_ID_SIZE_26);
    ux_widget_string_set(sedentary, 1, &sw->tips, gx_prompt_text_set_ext);
    if (ptr)
        *ptr = &sw->widget;
    return GX_SUCCESS;
}
