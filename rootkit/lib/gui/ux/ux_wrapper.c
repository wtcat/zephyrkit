#ifndef _WIN32
#include <sys/__assert.h>
#endif

#include "gx_api.h"
#include "gx_system.h"

#include "ux/_ux_wrapper.h"
#include "ux/_ux_action.h"
#include "ux/_ux_utils.h"


/* Define keyboard style flags */
#define UX_STYLE_KEY_PRESSED 0x00000010UL
#define TIMER_ID(widget)  ((UINT)widget)


UINT key_event_empty_fire(GX_WIDGET *widget)
{
    (void) widget;
    return GX_SUCCESS;
}

UINT __ux_key_event_wrapper(GX_WIDGET *widget, GX_EVENT *event_ptr,
     USHORT key,
     UINT timeout,
     UINT (*click_fire)(GX_WIDGET *widget),
     UINT (*timeout_fire)(GX_WIDGET *widget),
     UINT (*event_fn)(GX_WIDGET *, GX_EVENT *))
{
#ifndef _WIN32
    __ASSERT(click_fire != NULL, "");
    __ASSERT(timeout_fire != NULL, "");
#endif
    switch (event_ptr->gx_event_type) {
    case GX_EVENT_TIMER:
        if (event_ptr->gx_event_payload.gx_event_timer_id == 
            TIMER_ID(widget)) {
            if (widget->gx_widget_style & UX_STYLE_KEY_PRESSED) {
                widget->gx_widget_style &= ~UX_STYLE_KEY_PRESSED;
                return timeout_fire(widget);
            }
        }
        break;
    case GX_EVENT_KEY_DOWN:
        if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == 
            key) {
            if (!(widget->gx_widget_style & UX_STYLE_KEY_PRESSED)) {
                widget->gx_widget_style |= UX_STYLE_KEY_PRESSED;
                gx_system_timer_start(widget, TIMER_ID(widget), 
                    timeout, 0);
            }
        }
        break;
    case GX_EVENT_KEY_UP:
        if (widget->gx_widget_style & UX_STYLE_KEY_PRESSED) {
            gx_system_timer_stop(widget, TIMER_ID(widget));
            click_fire(widget);
        }
        break;
    default:
        break;
    }
    return event_fn(widget, event_ptr);
}

UINT pop_widget_event_process_wrapper(GX_WIDGET *widget,
    GX_EVENT *event_ptr,
    UINT timeout,
    UINT(*event_process)(GX_WIDGET *, GX_EVENT *),
    const GX_ANIMATION_INFO *info)
{
    UINT ret = GX_SUCCESS;

    if (event_ptr == NULL)
        goto _action;
    switch (event_ptr->gx_event_type) {
    case GX_EVENT_TIMER:
        if (event_ptr->gx_event_payload.gx_event_timer_id == POP_WIDGET_TIMER_ID ||
            event_ptr->gx_event_payload.gx_event_timer_id == TIMER_ID(widget)) {
            goto _action;
        }
        break;
    case GX_EVENT_KEY_DOWN:
        if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == 
            GX_KEY_HOME) {
            if (!(widget->gx_widget_style & UX_STYLE_KEY_PRESSED)) {
                widget->gx_widget_style |= UX_STYLE_KEY_PRESSED;
                gx_system_timer_start(widget, TIMER_ID(widget), 
                    UX_MSEC(1000), 0);
            }
        }
        break;
    case GX_EVENT_KEY_UP:
        if (widget->gx_widget_style & UX_STYLE_KEY_PRESSED) 
            goto _action;
        else
            ret = event_process(widget, event_ptr);
        break;
    case GX_EVENT_ANIMATION_COMPLETE:
        if (widget->gx_widget_status & GX_STATUS_VISIBLE) {
            if (timeout)
                gx_system_timer_start(widget, POP_WIDGET_TIMER_ID, timeout, 0);
        }
        break;
    default:
        ret = event_process(widget, event_ptr);
        break;
    }
    return ret;

_action:
    widget->gx_widget_style &= ~UX_STYLE_KEY_PRESSED;
    gx_system_timer_stop(widget, TIMER_ID(widget));
    gx_system_timer_stop(widget, POP_WIDGET_TIMER_ID);
    return ux_screen_pop_and_switch(widget, info);
}
