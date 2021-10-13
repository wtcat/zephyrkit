#define GX_SOURCE_CODE

#include "gx_api.h"
#include "gx_animation.h"
#include "gx_system.h"

#include "ux/_ux_animation.h"

#define UX_ANIMATION_MAGIC 0xdeadbeef

//#define _DEBUG_ON

#ifdef _DEBUG_ON
#include <stdio.h>

static void show_position(const char *prompt, GX_ANIMATION *animation)
{
    GX_WIDGET *widget = animation->gx_animation_info.gx_animation_slide_screen_list[0];
    printf("%s: START(%d), CURRENT(%d)\n", prompt,
        animation->gx_animation_slide_tracking_start_pos,
        animation->gx_animation_slide_tracking_current_pos);
    printf("-->X(%d)\n", widget->gx_widget_size.gx_rectangle_left);
}
#else /* !DEBUG_ON */

#define show_position(...)
#endif

static void ux_animation_pen_down(GX_ANIMATION* animation, 
                                  GX_ANIMATION_INFO* info,
                                  GX_EVENT *event_ptr, 
                                  VOID (*pend_down)(GX_ANIMATION *, GX_ANIMATION_INFO *, GX_EVENT *))
{
    if (animation->gx_animation_status == GX_ANIMATION_IDLE) {
        _gx_system_input_capture(info->gx_animation_parent);
        animation->gx_animation_status = GX_ANIMATION_SLIDE_TRACKING;
        if (info->gx_animation_style & GX_ANIMATION_VERTICAL) {
            animation->gx_animation_slide_tracking_start_pos = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y;
            animation->gx_animation_slide_tracking_current_pos = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y;
        } else {
            animation->gx_animation_slide_tracking_start_pos = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
            animation->gx_animation_slide_tracking_current_pos = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
        }
        animation->gx_animation_slide_target_index_1 = -1;
        animation->gx_animation_slide_target_index_2 = -1;
        if (pend_down != NULL)
            pend_down(animation, info, event_ptr);
    }
}

static void ux_animation_drag(GX_ANIMATION* animation, 
                              GX_ANIMATION_INFO* info,
                              GX_EVENT *event_ptr,
                              UINT (*drag_tracking)(GX_ANIMATION *, GX_POINT),
                              UINT (*drag_tracking_start)(GX_ANIMATION *, GX_POINT))
{
    GX_EVENT input_release_event;
    UX_ANIMATION *u_animation;
    GX_WIDGET **stackptr;
    INT delta;

    if (animation->gx_animation_status == GX_ANIMATION_SLIDE_TRACKING) {
        u_animation = (UX_ANIMATION *)animation;
        if (info->gx_animation_style & GX_ANIMATION_VERTICAL) {
            delta = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y - 
                animation->gx_animation_slide_tracking_start_pos);
        } else {
            delta = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x -
               animation->gx_animation_slide_tracking_start_pos);
        }
        if ((animation->gx_animation_slide_target_index_1 == -1) &&
            (delta > GX_ANIMATION_MIN_SLIDING_DIST)) {
            /* Start swiping, remove other widgets from input capture stack.  */
            stackptr = _gx_system_input_capture_stack;
            memset(&input_release_event, 0, sizeof(GX_EVENT));
            input_release_event.gx_event_type = GX_EVENT_INPUT_RELEASE;
            while (*stackptr) {
                if (*stackptr != info->gx_animation_parent) {
                    input_release_event.gx_event_target = *stackptr;
                    _gx_system_event_send(&input_release_event);
                }
                stackptr++;
            }
            drag_tracking_start(animation, event_ptr->gx_event_payload.gx_event_pointdata);
        }
        drag_tracking(animation, event_ptr->gx_event_payload.gx_event_pointdata);       
    }
}

static void ux_animation_pen_up(GX_ANIMATION *animation, GX_ANIMATION_INFO *info,
    UINT (*slide_landing_start)(GX_ANIMATION *))
{
    GX_RECTANGLE *size;
    INT delta, shift;

    if (animation->gx_animation_status == GX_ANIMATION_SLIDE_TRACKING) {
        _gx_system_input_release(info->gx_animation_parent);
        animation->gx_animation_status = GX_ANIMATION_IDLE;
        size = &info->gx_animation_parent->gx_widget_size;
        delta = animation->gx_animation_slide_tracking_current_pos - 
                animation->gx_animation_slide_tracking_start_pos;
        if (info->gx_animation_style & GX_ANIMATION_VERTICAL)
            shift = (size->gx_rectangle_bottom - size->gx_rectangle_top + 1) >> 1;
        else
            shift = (size->gx_rectangle_right - size->gx_rectangle_left + 1) >> 1;

        if ((abs(delta) < shift) || (animation->gx_animation_slide_target_index_2 == -1)) {
            /* slide back to original when slide distance is less than half screen width/height. */
            if (animation->gx_animation_slide_target_index_2 >= 0) {
                INT temp = animation->gx_animation_slide_target_index_1;
                animation->gx_animation_slide_target_index_1 = animation->gx_animation_slide_target_index_2;
                animation->gx_animation_slide_target_index_2 = (GX_VALUE)temp;
            }
            switch (animation->gx_animation_slide_direction) {
            case GX_ANIMATION_SLIDE_LEFT:
                animation->gx_animation_slide_direction = GX_ANIMATION_SLIDE_RIGHT;
                break;
            case GX_ANIMATION_SLIDE_RIGHT:
                animation->gx_animation_slide_direction = GX_ANIMATION_SLIDE_LEFT;
                break;
            case GX_ANIMATION_SLIDE_UP:
                animation->gx_animation_slide_direction = GX_ANIMATION_SLIDE_DOWN;
                break;
            case GX_ANIMATION_SLIDE_DOWN:
                animation->gx_animation_slide_direction = GX_ANIMATION_SLIDE_UP;
                break;
            }
        }
        if (delta)
            slide_landing_start(animation);
    }
}

static void ux_animation_flick_process(GX_ANIMATION *animation, GX_ANIMATION_INFO *info,
    GX_EVENT *event_ptr, UINT(*slide_landing_start)(GX_ANIMATION *))
{
    if (animation->gx_animation_status == GX_ANIMATION_SLIDE_LANDING) {
        INT delta = event_ptr->gx_event_payload.gx_event_intdata[0];
        if (((animation->gx_animation_slide_direction == GX_ANIMATION_SLIDE_LEFT) && (delta > 0)) ||
            ((animation->gx_animation_slide_direction == GX_ANIMATION_SLIDE_RIGHT) && (delta < 0)) ||
            ((animation->gx_animation_slide_direction == GX_ANIMATION_SLIDE_UP) && (delta > 0)) ||
            ((animation->gx_animation_slide_direction == GX_ANIMATION_SLIDE_DOWN) && (delta < 0))) {
            /* landing direction is different to flick direction
               exchange targets */
            if (animation->gx_animation_slide_target_index_2 >= 0) {
                INT temp = animation->gx_animation_slide_target_index_1;
                animation->gx_animation_slide_target_index_1 = animation->gx_animation_slide_target_index_2;
                animation->gx_animation_slide_target_index_2 = (GX_VALUE)temp;
            }

            animation->gx_animation_status = GX_ANIMATION_IDLE;
            switch (animation->gx_animation_slide_direction) {
            case GX_ANIMATION_SLIDE_LEFT:
                animation->gx_animation_slide_direction = GX_ANIMATION_SLIDE_RIGHT;
                break;
            case GX_ANIMATION_SLIDE_RIGHT:
                animation->gx_animation_slide_direction = GX_ANIMATION_SLIDE_LEFT;
                break;
            case GX_ANIMATION_SLIDE_UP:
                animation->gx_animation_slide_direction = GX_ANIMATION_SLIDE_DOWN;
                break;
            default:
                animation->gx_animation_slide_direction = GX_ANIMATION_SLIDE_UP;
                break;
            }
        }
        if (delta)
            slide_landing_start(animation);
    }
}

static void ux_animation_timer_process(GX_ANIMATION *animation, GX_ANIMATION_INFO *info, 
    GX_EVENT *event_ptr, UINT (*slide_landing)(GX_ANIMATION *))
{
    if (event_ptr->gx_event_payload.gx_event_timer_id == GX_ANIMATION_SLIDE_TIMER) {
        if (animation->gx_animation_status != GX_ANIMATION_SLIDE_LANDING) {
            _gx_system_timer_stop(info->gx_animation_parent, GX_ANIMATION_SLIDE_TIMER);
            return;
        }
        slide_landing(animation);
    }
}

static UINT ux_animation_drag_event_check(GX_ANIMATION* animation, 
                                          GX_EVENT* event_ptr,
                                          UINT (*drag_tracking)(GX_ANIMATION *, GX_POINT),
                                          UINT (*drag_tracking_start)(GX_ANIMATION *, GX_POINT),
                                          UINT (*slide_landing)(GX_ANIMATION *),
                                          UINT (*slide_landing_start)(GX_ANIMATION *),
                                          VOID (*pend_down)(GX_ANIMATION *, GX_ANIMATION_INFO *, GX_EVENT *))
{
    GX_ANIMATION_INFO* info = &animation->gx_animation_info;

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_PEN_DOWN:
        ux_animation_pen_down(animation, info, event_ptr, pend_down);
        show_position("Start tracking", animation);
        break;
    case GX_EVENT_PEN_DRAG:
        ux_animation_drag(animation, info, event_ptr, drag_tracking, drag_tracking_start);
        show_position("Drag tracking", animation);
        break;
    case GX_EVENT_PEN_UP:
        ux_animation_pen_up(animation, info, slide_landing_start);
        show_position("Stop tracking", animation);
        break;
    case GX_EVENT_HORIZONTAL_FLICK:
    case GX_EVENT_VERTICAL_FLICK:
        ux_animation_flick_process(animation, info, event_ptr, slide_landing_start);
        show_position("Flick tracking", animation);
        break;
    case GX_EVENT_TIMER:
        ux_animation_timer_process(animation, info, event_ptr, slide_landing);
        show_position("Timer tracking", animation);
        break;
    }
    return GX_SUCCESS;
}

static UINT ux_animation_drag_event_process(GX_WIDGET* widget, GX_EVENT* event_ptr)
{
    GX_ANIMATION* animation = _gx_system_animation_list;

    while (animation) {
        if ((animation->gx_animation_info.gx_animation_parent == widget) &&
            (animation->gx_animation_original_event_process_function)) {
            UX_ANIMATION* u_animation = (UX_ANIMATION*)animation;
            if (u_animation->magic_number == UX_ANIMATION_MAGIC) {
                UX_ANIMATION *u_animation = (UX_ANIMATION *)animation;
                ux_animation_drag_event_check(animation, event_ptr,
                    u_animation->gx_animation_drag_tracking,
                    u_animation->gx_animation_drag_tracking_start,
                    u_animation->gx_animation_slide_landing,
                    u_animation->gx_animation_slide_landing_start,
                    u_animation->gx_animation_pendown);
            } else {
                ux_animation_drag_event_check(animation, event_ptr,
                    _gx_animation_drag_tracking,
                    _gx_animation_drag_tracking_start,
                    _gx_animation_slide_landing,
                    _gx_animation_slide_landing_start,
                    NULL);
            }
            animation->gx_animation_original_event_process_function(widget, event_ptr);
        }
        animation = animation->gx_animation_next;
    }
    return GX_SUCCESS;
}

UINT ux_animation_drag_enable(UX_ANIMATION* animation, GX_WIDGET* widget, GX_ANIMATION_INFO* info)
{
    UINT ret;

    ret = _gx_animation_drag_enable((GX_ANIMATION*)animation, widget, info);
    if (ret == GX_SUCCESS) {
        animation->gx_animation_pendown = GX_NULL;
        animation->gx_animation_slide_landing = _gx_animation_slide_landing;
        animation->gx_animation_slide_landing_start = _gx_animation_slide_landing_start;
        animation->gx_animation_drag_event_check = ux_animation_drag_event_check;
        animation->gx_animation_drag_tracking_start = _gx_animation_drag_tracking_start;
        animation->gx_animation_drag_tracking = _gx_animation_drag_tracking;
        widget->gx_widget_event_process_function = ux_animation_drag_event_process;
        animation->magic_number = UX_ANIMATION_MAGIC;
    }
    return ret;
}
