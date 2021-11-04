#include "guix_api.h"

/* Checkbox timer ID */
#define GUIX_CHECKBOX_TIMER     0x02

/* Checkbox state */
#define CHECKBOX_STATE_NONE     0x01
#define CHECKBOX_STATE_SLIDING  0x02


static VOID guix_checkbox_select(GX_CHECKBOX *checkbox)
{
    checkbox->gx_widget_style ^= GX_STYLE_BUTTON_PUSHED;
}

static VOID guix_checkbox_draw(GUIX_CHECKBOX *checkbox)
{
    GX_RECTANGLE *rect = &checkbox->gx_widget_size;
    GX_PIXELMAP *map;
    INT ypos;

    gx_context_pixelmap_get(checkbox->background_id, &map);
    if (!map)
        return;
    
    gx_canvas_pixelmap_draw(rect->gx_rectangle_left, 
        rect->gx_rectangle_top, map);

    if (checkbox->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
        gx_context_pixelmap_get(checkbox->gx_checkbox_checked_pixelmap_id, &map);
        if (!map)
            return;

        ypos = (rect->gx_rectangle_bottom - rect->gx_rectangle_top + 1);
        ypos = (ypos - map->gx_pixelmap_width) / 2;
        ypos += rect->gx_rectangle_top;
        if (checkbox->state == CHECKBOX_STATE_NONE) {
            gx_canvas_pixelmap_draw(rect->gx_rectangle_right - 
                map->gx_pixelmap_width - checkbox->cur_offset,
                ypos, map);
        } else {
            gx_canvas_pixelmap_draw(rect->gx_rectangle_left + 
                checkbox->cur_offset, ypos, map);  
        }
    } else {
        gx_context_pixelmap_get(checkbox->gx_checkbox_unchecked_pixelmap_id, &map);
        if (!map)
            return;

        ypos = (rect->gx_rectangle_bottom - rect->gx_rectangle_top + 1);
        ypos = (ypos - map->gx_pixelmap_width) / 2;
        ypos += rect->gx_rectangle_top;
        if (checkbox->state == CHECKBOX_STATE_NONE) {
            gx_canvas_pixelmap_draw(rect->gx_rectangle_left +
                checkbox->cur_offset, ypos, map);
        } else {
            gx_canvas_pixelmap_draw(rect->gx_rectangle_right - 
                map->gx_pixelmap_width - checkbox->cur_offset,
                ypos, map);
        }
    }
}

static UINT guix_checkbox_event_process(GUIX_CHECKBOX *checkbox, 
    GX_EVENT *event_ptr)
{
    GX_WIDGET *widget = (GX_WIDGET *)checkbox;
    
    switch (event_ptr->gx_event_type) {
    case GX_EVENT_PEN_DOWN:
        if (checkbox->state == CHECKBOX_STATE_NONE) {
            checkbox->gx_button_select_handler(widget);
            checkbox->state = CHECKBOX_STATE_SLIDING;
            gx_system_timer_start(checkbox, GUIX_CHECKBOX_TIMER, 1, 1);
        }
        return gx_widget_event_process(widget, event_ptr);

    case GX_EVENT_TIMER:
        if (event_ptr->gx_event_payload.gx_event_timer_id == GUIX_CHECKBOX_TIMER) {
            checkbox->cur_offset += 2;
            if (checkbox->cur_offset >= checkbox->end_offset) {
                checkbox->cur_offset = checkbox->start_offset;
                gx_system_timer_stop(checkbox, GUIX_CHECKBOX_TIMER);
                checkbox->state = CHECKBOX_STATE_NONE;
                if (checkbox->gx_widget_style & GX_STYLE_BUTTON_PUSHED)
                    gx_widget_event_generate(widget, GX_EVENT_TOGGLE_ON, 0);
                else
                    gx_widget_event_generate(widget, GX_EVENT_TOGGLE_OFF, 0);
            }
            gx_system_dirty_mark(checkbox);
        }
        break;

    default:
        return gx_widget_event_process(widget, event_ptr);
    }

    return 0;
}

VOID guix_checkbox_create(GUIX_CHECKBOX *checkbox, GX_WIDGET *parent, 
    GUIX_CHECKBOX_INFO *info, GX_RECTANGLE *size)
{
    gx_checkbox_create((GX_CHECKBOX *)checkbox, "guix", parent, GX_ID_NONE, 
        GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, info->widget_id, size);

    checkbox->gx_button_select_handler = 
        (VOID (*)(GX_WIDGET *))guix_checkbox_select;
    checkbox->gx_widget_draw_function = 
        (VOID (*)(GX_WIDGET *))guix_checkbox_draw;
    checkbox->gx_widget_event_process_function = 
        (UINT (*)(GX_WIDGET *, GX_EVENT *))guix_checkbox_event_process;
    
    checkbox->background_id = info->background_id;
    checkbox->cur_offset = info->start_offset;
    checkbox->start_offset = info->start_offset;
    checkbox->end_offset = info->end_offset;
    checkbox->gx_checkbox_checked_pixelmap_id = info->checked_map_id;
    checkbox->gx_checkbox_unchecked_pixelmap_id = info->unchecked_map_id;
    checkbox->state = CHECKBOX_STATE_NONE;
}

