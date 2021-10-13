#include <stdlib.h>

#include "ux/ux_api.h"
#include "gx_widget.h"

#include "popup/widget.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"

#define HR_DELTA_ALPHA 10

struct hrov_widget {
    GX_WIDGET widget;
    GX_WIDGET pixelmap;
    GX_RESOURCE_ID icon;
    GX_PROMPT value;
    GX_PROMPT unit;
    GX_PROMPT tips;
    GX_UBYTE alpha;
    INT alpha_dir;
    INT angle;
};

static struct hrov_widget popup_widget;
USTR_START(hrov)
    _USTR(LANGUAGE_ENGLISH, "times/min", "Heartrate Overhigh"),
#ifndef _WIN32
    _USTR(LANGUAGE_CHINESE, "次/分", "心率过高")
#endif
USTR_END(hrov)

static void hrov_widget_draw(GX_WIDGET *widget)
{
    struct hrov_widget *hw = CONTAINER_OF(widget, struct hrov_widget, pixelmap);

    _gx_widget_background_draw(&hw->widget);
    __ux_rectangle_pixelmap_draw(&widget->gx_widget_size, hw->icon,
        GX_STYLE_HALIGN_CENTER | GX_STYLE_VALIGN_CENTER, hw->alpha);
    _gx_widget_children_draw(&hw->widget);
}

static UINT hrov_widget_event_process(GX_WIDGET *widget,
    GX_EVENT *event_ptr)
{
    struct hrov_widget *hw = CONTAINER_OF(widget, struct hrov_widget, widget);

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_TIMER:
        if (event_ptr->gx_event_payload.gx_event_timer_id == UX_TIMER_ID(widget)) {
            GX_FIXED_VAL sinv;
            hw->angle += HR_DELTA_ALPHA;
            if (hw->angle > 360)
                hw->angle = 0;
            sinv = _gx_utility_math_sin(GX_FIXED_VAL_MAKE(hw->angle));
            hw->alpha = GX_FIXED_VAL_TO_INT(sinv * 255);

            //INT alpha = hw->alpha;
            //alpha += hw->alpha_dir * HR_DELTA_ALPHA;
            //if (alpha > 255) {
            //    alpha = 255;
            //    hw->alpha_dir = -1;
            //} else if (alpha < HR_DELTA_ALPHA) {
            //    alpha = HR_DELTA_ALPHA;
            //    hw->alpha_dir = 1;
            //}
            //hw->alpha = alpha;
            gx_system_dirty_partial_add(widget, &hw->pixelmap.gx_widget_size);
            return GX_SUCCESS;
        } else {
            gx_system_timer_stop(widget, UX_TIMER_ID(widget));
        }
        break;
    case GX_EVENT_ANIMATION_COMPLETE:
        hw->angle = 0;
        hw->alpha_dir = 1;
        hw->alpha = HR_DELTA_ALPHA;
        gx_system_timer_start(widget, UX_TIMER_ID(widget), 1, 5);
        break;
    default:
        break;
    }
    return pop_widget_event_process_wrapper(&hw->widget, event_ptr,
        500, _gx_widget_event_process, popup_hide_animation);
}

GX_WIDGET *hrov_widget_get(INT value)
{
    struct hrov_widget *hw = &popup_widget;
    static char buffer[5];
    GX_STRING str;
    int i = 4;

    while (value && i > 0) {
        buffer[--i] = value % 10 + '0';
        value /= 10;
    }
    buffer[4] = '\0';
    str.gx_string_ptr = &buffer[i];
    str.gx_string_length = 4 - i;
    gx_prompt_text_set_ext(&hw->value, &str);
    return &hw->widget;
}

UINT hrov_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
    struct hrov_widget *hw = &popup_widget;
    GX_RECTANGLE client;

    /* Setup widget base class */
    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_widget_create(&hw->widget, "charging", parent,
        GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&hw->widget, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_event_process_set(&hw->widget, hrov_widget_event_process);

    /* Set icon */
    gx_utility_rectangle_define(&client, 120, 65, 120 + 80, 65 + 80);
    gx_widget_create(&hw->pixelmap, "pixelmap", &hw->widget,
        GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&hw->pixelmap, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_draw_set(&hw->pixelmap, hrov_widget_draw);
    hw->icon = GX_PIXELMAP_ID_HEART_RATE_REMIND;

    /* Heartrate value */
    gx_utility_rectangle_define(&client, 66, 168, 66+108, 168+60);
    gx_prompt_create(&hw->value, "value", &hw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(&hw->value, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&hw->value, GX_FONT_ID_SIZE_60);

    /* Heartrate unit */
    gx_utility_rectangle_define(&client, 180, 187, 180+90, 187+30);
    gx_prompt_create(&hw->unit, "unit", &hw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(&hw->unit, GX_COLOR_ID_HONBOW_GRAY,
        GX_COLOR_ID_HONBOW_GRAY, GX_COLOR_ID_HONBOW_GRAY);
    gx_prompt_font_set(&hw->unit, GX_FONT_ID_SIZE_26);
    ux_widget_string_set(hrov, 0, &hw->unit, gx_prompt_text_set_ext);

    /* Heartrate tips */
    gx_utility_rectangle_define(&client, 20, 232, 320-20, 232 + 45);
    gx_prompt_create(&hw->tips, "unit", &hw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(&hw->tips, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&hw->tips, GX_FONT_ID_SIZE_26);
    ux_widget_string_set(hrov, 1, &hw->tips, gx_prompt_text_set_ext);
    if (ptr)
        *ptr = &hw->widget;
    return GX_SUCCESS;
}
