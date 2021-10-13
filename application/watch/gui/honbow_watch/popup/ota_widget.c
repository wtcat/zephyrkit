#include "ux/ux_api.h"
#include "gx_widget.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"

struct ota_widget {
    GX_WIDGET widget;
    GX_PROGRESS_BAR progress;
    GX_RECTANGLE client;
    GX_RESOURCE_ID pixelmap;
};

static struct ota_widget popup_widget;
static const GX_PROGRESS_BAR_INFO ota_progress_bar = {
    .gx_progress_bar_info_min_val = 0,
    .gx_progress_bar_info_max_val = 100,
    .gx_progress_bar_info_current_val = 0, 
    .gx_progress_bar_font_id = GX_FONT_ID_SIZE_26,
    .gx_progress_bar_normal_text_color = GX_COLOR_ID_HONBOW_WHITE,
    .gx_progress_bar_selected_text_color = GX_COLOR_ID_HONBOW_WHITE,
    .gx_progress_bar_disabled_text_color = GX_COLOR_ID_HONBOW_WHITE,
    .gx_progress_bar_fill_pixelmap = 0
};

static void ota_widget_draw(struct ota_widget *ota)
{
    _gx_widget_background_draw(&ota->widget);
    ux_rectangle_pixelmap_draw(&ota->client, ota->pixelmap, 0);
    _gx_widget_children_draw(&ota->widget);
}

void ota_progress_set(GX_WIDGET *ota, INT value)
{
    struct ota_widget *owidget = CONTAINER_OF(ota, 
        struct ota_widget, widget);
    gx_progress_bar_value_set(&owidget->progress, value);
}

GX_WIDGET *ota_widget_get(void)
{
    return &popup_widget.widget;
}

UINT ota_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
    struct ota_widget *owidget = &popup_widget;
    GX_RECTANGLE client;

    /* Setup widget base class */
    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_widget_create(&owidget->widget, "ota", parent,
        GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&owidget->widget, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_draw_set(&owidget->widget, ota_widget_draw);

    /* Set icon */
    gx_utility_rectangle_define(&owidget->client, 96, 67, 129, 146);
    owidget->pixelmap = GX_PIXELMAP_ID_OAT_UPDATE;

    /* Create progress bar */
    gx_utility_rectangle_define(&client, 47, 265, 320-45, 360-94);
    gx_progress_bar_create(&owidget->progress, "progress-bar", 
        &owidget->widget, (void *)&ota_progress_bar, 
        GX_STYLE_ENABLED, 0, &client);
        
    gx_widget_fill_color_set(&owidget->progress, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_GREEN, GX_COLOR_ID_HONBOW_BLACK);

    ota_progress_set(&owidget->widget, 20); //TODO: remove (only for test)
    return GX_SUCCESS;
}

