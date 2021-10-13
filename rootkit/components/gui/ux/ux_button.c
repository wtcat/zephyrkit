#include "gx_api.h"
#include "gx_canvas.h"
#include "gx_context.h"
#include "gx_widget.h"
#include "gx_button.h"

#include "ux/_ux_button.h"
#include "ux/_ux_utils.h"


static void pixelmap_button_color_set(GX_WIDGET *widget, struct pixelmap_button *pb)
{
    if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED)
        ux_rectangle_background_render(&widget->gx_widget_size, pb->selected_fill_color,
            GX_BRUSH_ALIAS | GX_BRUSH_ROUND, 0, NULL);
    else
        ux_rectangle_background_render(&widget->gx_widget_size, pb->normal_fill_color,
            GX_BRUSH_ALIAS | GX_BRUSH_ROUND, 0, NULL);
}

static void pixelmap_button_draw(GX_PIXELMAP_BUTTON *button)
{
    struct pixelmap_button *pb = CONTAINER_OF(button, struct pixelmap_button, button);
    GX_WIDGET *widget = (GX_WIDGET *)button;

    pb->color_set(widget, pb);
    ux_rectangle_pixelmap_draw(&button->gx_widget_size, button->gx_pixelmap_button_normal_id, 
        button->gx_widget_style);
    _gx_widget_children_draw(widget);
}

UINT ux_pixelmap_button_create(UX_PIXELMAP_BUTTON *button,
    const char *name,
    GX_WIDGET *parent,
    GX_RESOURCE_ID pixelmap_id,
    GX_RESOURCE_ID normal_color_id,
    GX_RESOURCE_ID selected_color_id,
    USHORT button_id,
    ULONG style,
    GX_CONST GX_RECTANGLE *size,
    void (*color_set)(GX_WIDGET *, struct pixelmap_button *))
{
    gx_pixelmap_button_create(&button->button, name, parent, pixelmap_id, pixelmap_id, pixelmap_id,
        style, button_id, size);
    gx_widget_draw_set(&button->button, pixelmap_button_draw);
    button->selected_fill_color = selected_color_id;
    button->normal_fill_color = normal_color_id;
    button->color_set = (color_set == NULL) ? pixelmap_button_color_set : color_set;
    return GX_SUCCESS;
}

static void text_button_color_set(GX_WIDGET *widget, struct ux_text_button *tb)
{
    if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED)
        ux_round_rectangle_render(&widget->gx_widget_size, tb->selected_fill_color, 6);
    else
        ux_round_rectangle_render(&widget->gx_widget_size, tb->normal_fill_color, 6);
}

static void text_button_draw(GX_TEXT_BUTTON *button)
{
    struct ux_text_button *tb = CONTAINER_OF(button, struct ux_text_button, button);
    GX_WIDGET *widget = (GX_WIDGET *)button;

    tb->color_set(widget, tb);
    _gx_text_button_text_draw(button);
    _gx_widget_children_draw(widget);
    if (!(button->gx_widget_style & GX_STYLE_ENABLED))
        _gx_monochrome_driver_disabled_button_line_draw((GX_BUTTON *)button);
}

UINT ux_text_button_create(UX_TEXT_BUTTON *button,
    const char *name,
    GX_WIDGET *parent,
    GX_RESOURCE_ID text_id,
    GX_RESOURCE_ID normal_bg_color,
    GX_RESOURCE_ID selected_bg_color,
    USHORT button_id,
    ULONG style,
    GX_CONST GX_RECTANGLE *size,
    void (*color_set)(GX_WIDGET *, struct ux_text_button *))
{
    gx_text_button_create(&button->button, name, parent, text_id, style, button_id, size);
    gx_widget_draw_set(&button->button, text_button_draw);
    button->selected_fill_color = selected_bg_color;
    button->normal_fill_color = normal_bg_color;
    button->color_set = (color_set == NULL) ? text_button_color_set : color_set;
    return GX_SUCCESS;
}
