#include "gx_api.h"
#include "gx_context.h"
#include "gx_canvas.h"
#include "gx_system.h"

#include "ux/_ux_action.h"
#include "ux/_ux_utils.h"
#include "ux/_ux_move.h"

static UINT __ux_screen_switch(GX_WIDGET *parent, GX_WIDGET *curr, GX_WIDGET *next)
{
    UINT ret;

    if (parent) {
        gx_widget_detach(curr);
        ret = gx_widget_attach(parent, next);
        if (curr->gx_widget_status & GX_STATUS_STUDIO_CREATED)
            gx_widget_delete(curr);
        return ret;
    }
    return GX_FAILURE;
}

UINT __ux_screen_move(GX_WIDGET *parent, GX_WIDGET *from, GX_WIDGET *to,
    const GX_ANIMATION_INFO *info)
{
    return (info == NULL)?
        __ux_screen_switch(parent, from, to):
        __ux_animation_run(parent, from, to, info);
}

UINT ux_screen_push_and_switch(GX_WIDGET *curr, GX_WIDGET *next,
    const GX_ANIMATION_INFO *info)
{
    GX_WIDGET *parent = curr->gx_widget_parent;

    if (gx_system_screen_stack_push(curr) == GX_SUCCESS)
        return __ux_screen_move(parent, curr, next, info);
    return GX_FAILURE;
}

UINT ux_screen_pop_and_switch(GX_WIDGET *curr, const GX_ANIMATION_INFO *info)
{
    GX_WIDGET *target;
    
    if (gx_system_screen_stack_get(NULL, &target) == GX_SUCCESS)
        return __ux_screen_move(curr->gx_widget_parent, curr, target, info);
    return GX_FAILURE;
}

void ux_input_capture_stack_release(GX_WIDGET *notifer)
{
    GX_WIDGET **stackptr = _gx_system_input_capture_stack;
    GX_EVENT release_event;

    memset(&release_event, 0, sizeof(GX_EVENT));
    release_event.gx_event_type = GX_EVENT_INPUT_RELEASE;
    while (*stackptr) {
        if (*stackptr != notifer) {
            release_event.gx_event_target = *stackptr;
            _gx_system_event_send(&release_event);
        }
        stackptr++;
    }
}

UINT ux_rectangle_background_render(const GX_RECTANGLE *client, GX_RESOURCE_ID color, 
    UINT style, INT delta, GX_COLOR (*color_preprocess)(GX_COLOR))
{
    GX_DRAW_CONTEXT *context;
    GX_COLOR vcolor;
    GX_VALUE height;

    gx_system_draw_context_get(&context);
    if (gx_context_color_get(color, &vcolor) == GX_SUCCESS) {
        if (color_preprocess)
            vcolor = color_preprocess(vcolor);
        gx_brush_define(&context->gx_draw_context_brush, vcolor, 0, style);
        height = client->gx_rectangle_bottom - client->gx_rectangle_top + 1;
        gx_context_brush_width_set(height);
        height = (height >> 1) + delta;
        gx_canvas_line_draw(client->gx_rectangle_left + height,
            client->gx_rectangle_top + height,
            client->gx_rectangle_right - height,
            client->gx_rectangle_top + height);
        return GX_SUCCESS;
    }
    return GX_FAILURE;
}

UINT __ux_rectangle_pixelmap_draw(const GX_RECTANGLE *client, GX_RESOURCE_ID pixelmap,
    UINT style, GX_UBYTE alpha)
{
    GX_PIXELMAP *map;

    _gx_context_pixelmap_get(pixelmap, &map);
    if (map) {
        INT xpos, ypos;
        switch (style & GX_PIXELMAP_HALIGN_MASK) {
        case GX_STYLE_HALIGN_CENTER:
            xpos = client->gx_rectangle_right - client->gx_rectangle_left -
                map->gx_pixelmap_width + 1;
            xpos = xpos / 2 + client->gx_rectangle_left;
            break;
        case GX_STYLE_HALIGN_RIGHT:
            xpos = client->gx_rectangle_right - map->gx_pixelmap_width + 1;
            break;
        default:
            xpos = client->gx_rectangle_left;
            break;
        }
        switch (style & GX_PIXELMAP_VALIGN_MASK) {
        case GX_STYLE_VALIGN_CENTER:
            ypos = client->gx_rectangle_bottom - client->gx_rectangle_top -
                map->gx_pixelmap_height + 1;
            ypos = ypos / 2 + client->gx_rectangle_top;
            break;
        case GX_STYLE_VALIGN_BOTTOM:
            ypos = client->gx_rectangle_bottom - map->gx_pixelmap_height + 1;
            break;
        default:
            ypos = client->gx_rectangle_top;
            break;
        }
        if (alpha == 255)
            _gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
        else
            _gxe_canvas_pixelmap_blend((GX_VALUE)xpos, (GX_VALUE)ypos, map, alpha);
        return GX_SUCCESS;
    }
    return GX_FAILURE;
}

UINT ux_rectangle_pixelmap_draw(const GX_RECTANGLE *client, GX_RESOURCE_ID pixelmap, 
    UINT style)
{
    return __ux_rectangle_pixelmap_draw(client, pixelmap, style, 255);
}

UINT ux_round_rectangle_render(const GX_RECTANGLE *client, GX_RESOURCE_ID fill_color,
    GX_VALUE boardwidth)
{
    GX_DRAW_CONTEXT *context;
    GX_COLOR vcolor;
    GX_VALUE height;
    GX_VALUE width;

    gx_system_draw_context_get(&context);
    if (gx_context_color_get(fill_color, &vcolor) == GX_SUCCESS) {
        gx_brush_define(&context->gx_draw_context_brush, vcolor, 0, 
            GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
        gx_context_brush_width_set(boardwidth << 1);
        width = boardwidth;
        gx_canvas_line_draw(client->gx_rectangle_left + width,
            client->gx_rectangle_top + width,
            client->gx_rectangle_right - width,
            client->gx_rectangle_top + width);
        gx_canvas_line_draw(client->gx_rectangle_left + width,
            client->gx_rectangle_bottom - width,
            client->gx_rectangle_right - width,
            client->gx_rectangle_bottom - width);

        height = client->gx_rectangle_bottom - client->gx_rectangle_top + 1;
        gx_context_brush_width_set(height - (boardwidth << 1));
        height = (height >> 1);
        gx_brush_define(&context->gx_draw_context_brush, vcolor, 0, 0);
        gx_canvas_line_draw(client->gx_rectangle_left,
            client->gx_rectangle_top + height,
            client->gx_rectangle_right,
            client->gx_rectangle_top + height);
        return GX_SUCCESS;
    }
    return GX_FAILURE;
}

UINT ux_round_rectangle_render2(const GX_RECTANGLE *client,
    GX_VALUE boardwidth,
    GX_VALUE upper_width,
    GX_RESOURCE_ID color_upper,
    GX_RESOURCE_ID color_bottom)
{
    GX_COLOR vcolor_upper, vcolor_bottom;
    GX_DRAW_CONTEXT *context;
    GX_VALUE width;
    GX_VALUE h;
    UINT ret;

    gx_system_draw_context_get(&context);
    ret = gx_context_color_get(color_upper, &vcolor_upper);
    if (ret != GX_SUCCESS)
        goto _out;
    ret = gx_context_color_get(color_bottom, &vcolor_bottom);
    if (ret != GX_SUCCESS)
        goto _out;
    gx_brush_define(&context->gx_draw_context_brush, vcolor_upper, 0,
        GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
    gx_context_brush_width_set(boardwidth << 1);
    width = boardwidth;
    gx_canvas_line_draw(client->gx_rectangle_left + width,
        client->gx_rectangle_top + width,
        client->gx_rectangle_right - width,
        client->gx_rectangle_top + width);
    gx_brush_define(&context->gx_draw_context_brush, vcolor_bottom, 0,
        GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
    gx_canvas_line_draw(client->gx_rectangle_left + width,
        client->gx_rectangle_bottom - width,
        client->gx_rectangle_right - width,
        client->gx_rectangle_bottom - width);

    width = upper_width - boardwidth;
    gx_brush_define(&context->gx_draw_context_brush, vcolor_upper, 0, 0);
    gx_context_brush_width_set(width);
    width = (width >> 1) + boardwidth;
    gx_canvas_line_draw(client->gx_rectangle_left,
        client->gx_rectangle_top + width,
        client->gx_rectangle_right,
        client->gx_rectangle_top + width);

    h = client->gx_rectangle_bottom - client->gx_rectangle_top + 1;
    width = h - upper_width - boardwidth;
    gx_context_brush_width_set(width);
    width = (width >> 1) + upper_width;
    gx_brush_define(&context->gx_draw_context_brush, vcolor_bottom, 0, 0);
    gx_canvas_line_draw(client->gx_rectangle_left,
        client->gx_rectangle_top + width,
        client->gx_rectangle_right,
        client->gx_rectangle_top + width);
_out:
    return ret;
}

UINT ux_arc_progress_render(INT xorg, INT yorg, UINT r, UINT width, 
    INT start_angle, INT end_angle, GX_RESOURCE_ID color)
{
    GX_DRAW_CONTEXT *context;
    GX_COLOR vcolor;

    gx_system_draw_context_get(&context);
    if (gx_context_color_get(color, &vcolor) == GX_SUCCESS) {
        gx_brush_define(&context->gx_draw_context_brush, vcolor, 0,
            GX_BRUSH_ALIAS);
        gx_context_brush_width_set(width);
        return _gx_canvas_arc_draw(xorg, yorg, r, start_angle, end_angle);
    }
    return GX_FAILURE;
}

UINT ux_circle_render(INT xorg, INT yorg, UINT r, GX_RESOURCE_ID color)
{
    GX_DRAW_CONTEXT *context;
    GX_COLOR vcolor;

    gx_system_draw_context_get(&context);
    if (gx_context_color_get(color, &vcolor) == GX_SUCCESS) {
        gx_brush_define(&context->gx_draw_context_brush, vcolor, vcolor,
            GX_BRUSH_ALIAS| GX_BRUSH_SOLID_FILL);
        gx_context_brush_width_set(1);
        return _gx_canvas_circle_draw(xorg, yorg, r);
    }
    return GX_FAILURE;
}