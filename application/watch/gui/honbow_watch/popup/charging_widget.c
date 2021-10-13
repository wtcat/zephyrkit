#include "ux/ux_api.h"
#include "gx_widget.h"

#include "base/ev_types.h"
#include "popup/widget.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"

struct charging_widget {
    GX_WIDGET widget;
    GX_PROMPT level;
    GX_RESOURCE_ID icon;
    INT value;
};

static struct charging_widget charging_widget;
static const GX_RESOURCE_ID icons_table[] = {
    GX_PIXELMAP_ID_CHARGE_FIVE_PERCENT,
    GX_PIXELMAP_ID_CHARGE_TEN_PERCENT,
    GX_PIXELMAP_ID_CHARGE_TWENTY_PERCENT,
    GX_PIXELMAP_ID_CHARGE_THIRTY_PERCENT,
    GX_PIXELMAP_ID_CHARGE_FORTY_PERCENT,
    GX_PIXELMAP_ID_CHARGE_FIFTY_PERCENT,
    GX_PIXELMAP_ID_CHARGE_SIXTY_PERCENT,
    GX_PIXELMAP_ID_CHARGE_SEVENTY_PERCENT,
    GX_PIXELMAP_ID_CHARGE_EIGHTY_PERCENT,
    GX_PIXELMAP_ID_CHARGE_NINETY_PERCENT,
    GX_PIXELMAP_ID_CHARGE_HUNDRED_PERCENT
};

static void charging_widget_draw(struct charging_widget *cw)
{
    GX_RECTANGLE *prect = &cw->widget.gx_widget_size;
    GX_RECTANGLE client;

    client.gx_rectangle_left = prect->gx_rectangle_left + 63;
    client.gx_rectangle_top = prect->gx_rectangle_top + 83;
    client.gx_rectangle_right = client.gx_rectangle_left + 195;
    client.gx_rectangle_bottom = client.gx_rectangle_top + 195;
    _gx_widget_background_draw(&cw->widget);
    ux_rectangle_pixelmap_draw(&client, cw->icon,
        GX_STYLE_HALIGN_CENTER| GX_STYLE_VALIGN_CENTER);
    _gx_widget_children_draw(&cw->widget);
}

GX_WIDGET *charging_widget_get(INT level)
{
    struct charging_widget *cw = &charging_widget;
    static char percent[5];
    GX_STRING str;
    INT v;

    if (level > 100)
        return &cw->widget;
    if (level < 0) {
        v = level;
        level = cw->value;
        cw->value = v;
    } else {
        cw->value = level;
    }
    if (level < 10) {
        cw->icon = icons_table[0];
        v = 5;
        percent[0] = v + '0';
        percent[1] = '%';
        str.gx_string_length = 2;
    } else if (level >= 100) {
        cw->icon = icons_table[10];
        percent[0] = 1 + '0';
        percent[1] = '0';
        percent[2] = '0';
        percent[3] = '%';
        str.gx_string_length = 4;
    } else {
        v = level / 10;
        cw->icon = icons_table[v];
        percent[0] = v + '0';
        percent[1] = '0';
        percent[2] = '%';
        str.gx_string_length = 3;
    }
    str.gx_string_ptr = percent;
    gx_prompt_text_set_ext(&cw->level, &str);
    return &cw->widget;
}

static UINT charging_widget_event_process(struct charging_widget *cw, 
    GX_EVENT *event_ptr)
{
    if (event_ptr->gx_event_type == USER_EVENT(EV_CHARGING)) {
        struct ev_struct *e;
        e = (struct ev_struct *)event_ptr->gx_event_payload.gx_event_ulongdata;
        charging_widget_get((int)e->data);
        if (cw->value < 0)
            event_ptr = NULL;
    }
    return pop_widget_event_process_wrapper(&cw->widget, event_ptr,
        0, _gx_widget_event_process, popup_hide_animation);
}

UINT charging_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
    struct charging_widget *cw = &charging_widget;
    GX_RECTANGLE client;

    /* Setup widget base class */
    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_widget_create(&cw->widget, "charging", parent,
        GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&cw->widget, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_draw_set(&cw->widget, charging_widget_draw);
    gx_widget_event_process_set(&cw->widget, charging_widget_event_process);

    /* Battery power level */
    gx_utility_rectangle_define(&client, 63, 83, 320 - 62, 360 - 82);
    client.gx_rectangle_left = client.gx_rectangle_left + 56;
    client.gx_rectangle_top = client.gx_rectangle_top + 103;
    client.gx_rectangle_right = client.gx_rectangle_right - 52;
    client.gx_rectangle_bottom = client.gx_rectangle_bottom - 47;
    gx_prompt_create(&cw->level, "charging", &cw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(&cw->level, GX_COLOR_ID_HONBOW_GREEN,
        GX_COLOR_ID_HONBOW_GREEN, GX_COLOR_ID_HONBOW_GREEN);
    gx_prompt_font_set(&cw->level, GX_FONT_ID_SIZE_30);
    cw->icon = 0;
    if (ptr)
        *ptr = &cw->widget;
    return GX_SUCCESS;
}
