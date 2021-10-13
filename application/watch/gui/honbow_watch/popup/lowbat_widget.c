#include "ux/ux_api.h"
#include "gx_widget.h"

#include "popup/widget.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"

struct lowbat_widget {
    GX_WIDGET widget;
    GX_PROMPT battery_lvl;
    GX_PROMPT tips;
    GX_RESOURCE_ID icon;
    GX_RESOURCE_ID color;
    GX_VALUE delta_angle;
};

static struct lowbat_widget low_battery_widget;
USTR_START(lowbat)
    _USTR(LANGUAGE_ENGLISH,
        "Please charge now",
        "Battery less than 5%",
        "Battery less than 10%"
    )
USTR_END(lowbat)

static void lowbat_widget_draw(struct lowbat_widget *lb)
{
    GX_RECTANGLE *prect = &lb->widget.gx_widget_size;
    GX_RECTANGLE client;

    client.gx_rectangle_left = prect->gx_rectangle_left + 112;
    client.gx_rectangle_top = prect->gx_rectangle_top + 68;
    client.gx_rectangle_right = client.gx_rectangle_left + 91;
    client.gx_rectangle_bottom = client.gx_rectangle_top + 91;
    _gx_widget_background_draw(&lb->widget);
    ux_arc_progress_render(client.gx_rectangle_left + 45, 
        client.gx_rectangle_top + 45, 45, 10,
        0, lb->delta_angle, lb->color);
    ux_arc_progress_render(client.gx_rectangle_left + 45, 
        client.gx_rectangle_top + 45, 45, 10,
        lb->delta_angle, 359, GX_COLOR_ID_HONBOW_DARK_GRAY);
    ux_rectangle_pixelmap_draw(&client, lb->icon,
        GX_STYLE_HALIGN_CENTER | GX_STYLE_VALIGN_CENTER);
    _gx_widget_children_draw(&lb->widget);
}

static UINT lowbat_widget_event_process(struct lowbat_widget *lb, GX_EVENT *event_ptr)
{
    return pop_widget_event_process_wrapper(&lb->widget, event_ptr,
        200, _gx_widget_event_process, popup_hide_animation);
}

GX_WIDGET *lowbat_widget_get(INT power_lvl)
{
    struct lowbat_widget *lb = &low_battery_widget;

    if (power_lvl <= 5) {
        ux_widget_string_set(lowbat, 1, &lb->battery_lvl, gx_prompt_text_set_ext);
        lb->icon = GX_PIXELMAP_ID_ICON_RED;
        lb->color = GX_COLOR_RED;
        lb->delta_angle = 10;
    } else {
        ux_widget_string_set(lowbat, 2, &lb->battery_lvl, gx_prompt_text_set_ext);
        lb->icon = GX_PIXELMAP_ID_ICON_ORANGE_;
        lb->color = GX_COLOR_ID_HONBOW_ORANGE;
        lb->delta_angle = 40;
    }
    ux_widget_string_set(lowbat, 0, &lb->tips, gx_prompt_text_set_ext);
    return &lb->widget;
}

UINT lowbat_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
    struct lowbat_widget *lb = &low_battery_widget;
    GX_RECTANGLE client;

    /* Setup widget base class */
    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_widget_create(&lb->widget, "ota", parent,
        GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&lb->widget, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_draw_set(&lb->widget, lowbat_widget_draw);
    gx_widget_event_process_set(&lb->widget, lowbat_widget_event_process);

    /* Battery power level */
    gx_utility_rectangle_define(&client, 15, 185, 15 + 291, 185 + 26);
    gx_prompt_create(&lb->battery_lvl, "bat-lvl", &lb->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(&lb->battery_lvl, GX_COLOR_ID_HONBOW_ORANGE,
        GX_COLOR_ID_HONBOW_ORANGE, GX_COLOR_ID_HONBOW_ORANGE);
    gx_prompt_font_set(&lb->battery_lvl, GX_FONT_ID_SIZE_26);

    /* Remind charing */
    gx_utility_rectangle_define(&client, 12, 219, 12 + 291, 219 + 39);
    gx_prompt_create(&lb->tips, "bat-tips", &lb->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(&lb->tips, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&lb->tips, GX_FONT_ID_SIZE_30);
    if (ptr)
        *ptr = &lb->widget;
    return GX_SUCCESS;
}
