#include "gx_api.h"

#include "ux/_ux_action.h"
#include "ux/_ux_utils.h"

static inline GX_WIDGET *ux_action_target_get(GX_WIDGET *current, const struct ux_action *action)
{
    (void)current;
    return action->target;
}

static inline GX_WIDGET *ux_action_target_find(GX_WIDGET *current, const struct ux_action *action)
{
    (void)current;
    return (GX_WIDGET *)action->target;
}

static inline GX_WIDGET *ux_action_parent_find(GX_WIDGET *current, const struct ux_action *action)
{
    (void)current;
    return action->parent;
}

void ux_animation_execute(GX_WIDGET *current, const struct ux_action *action)
{
    GX_ANIMATION *animation;

    gx_system_animation_get(&animation);
    if (animation) {
        GX_ANIMATION_INFO animation_info = *action->animation;
        GX_WIDGET *parent = NULL;
        GX_WIDGET *target = NULL;

        if ((action->flags & UX_ACTION_FLAG_POP_TARGET) ||
            (action->flags & UX_ACTION_FLAG_POP_PARENT))
            gx_system_screen_stack_get((GX_WIDGET **)&parent, &target);

        if (action->flags & UX_ACTION_FLAG_POP_TARGET)
            animation_info.gx_animation_target = target;

        if (action->flags & UX_ACTION_FLAG_POP_PARENT)
            animation_info.gx_animation_parent = (GX_WIDGET *)parent;

        if ((!animation_info.gx_animation_target) &&
            (action->flags & UX_ACTION_FLAG_DYNAMIC_TARGET)) {
            target = ux_action_target_get(current, action);
            animation_info.gx_animation_target = target;
        }

        if (!animation_info.gx_animation_parent)
            animation_info.gx_animation_parent = ux_action_parent_find(current, action);

        if (animation_info.gx_animation_target && animation_info.gx_animation_parent) {
            animation_info.gx_animation_id = 1;
            gx_animation_start(animation, &animation_info);
        }
    }
}

bool ux_action_process(GX_WIDGET *widget, const struct ux_action *actions)
{
    const struct ux_action *action = actions;
    const GX_WIDGET *parent = GX_NULL;
    GX_WIDGET *target = GX_NULL;

    while (action->opcode) {
        switch (action->opcode) {
        case GX_ACTION_TYPE_ATTACH:
            if ((action->flags & UX_ACTION_FLAG_POP_TARGET) ||
                (action->flags & UX_ACTION_FLAG_POP_PARENT))
                gx_system_screen_stack_get((GX_WIDGET **)&parent, &target);
            if (!(action->flags & UX_ACTION_FLAG_POP_PARENT))
                parent = action->parent;
            if (!(action->flags & UX_ACTION_FLAG_POP_TARGET))
                target = ux_action_target_get(widget, action);
            if (parent && target)
                gx_widget_attach(parent, target);
            break;
        case GX_ACTION_TYPE_DETACH:
            target = ux_action_target_find(widget, action);
            if (target) {
                gx_widget_detach(target);
                if (target->gx_widget_status & GX_STATUS_STUDIO_CREATED)
                    gx_widget_delete(target);
            }
            break;
        case GX_ACTION_TYPE_TOGGLE:
            if (action->flags & UX_ACTION_FLAG_POP_TARGET)
                gx_system_screen_stack_get(GX_NULL, &target);
            else
                target = ux_action_target_get(widget, action);
            ux_screen_move(widget, target, NULL);
            break;
        case GX_ACTION_TYPE_SHOW:
            target = ux_action_target_get(widget, action);
            if (target)
                gx_widget_show(target);
            break;
        case GX_ACTION_TYPE_HIDE:
            target = ux_action_target_find(widget, action);
            if (target)
                gx_widget_hide(target);
            break;
        case GX_ACTION_TYPE_ANIMATION:
            ux_animation_execute(widget, action);
            break;
        case GX_ACTION_TYPE_WINDOW_EXECUTE:
            if ((action->flags & UX_ACTION_FLAG_POP_TARGET) ||
                (action->flags & UX_ACTION_FLAG_POP_PARENT))
                gx_system_screen_stack_get((GX_WIDGET **)&parent, &target);
            if (!(action->flags & UX_ACTION_FLAG_POP_PARENT))
                parent = widget->gx_widget_parent;
            if (!(action->flags & UX_ACTION_FLAG_POP_TARGET))
                target = ux_action_target_get(widget, action);
            if (parent && target) {
                gx_widget_attach(parent, target);
                gx_window_execute((GX_WINDOW *)target, GX_NULL);
            }
            break;
        case GX_ACTION_TYPE_WINDOW_EXECUTE_STOP:
            return false;  /*event_ptr->gx_event_sender; */
        case GX_ACTION_TYPE_SCREEN_STACK_PUSH:
            target = ux_action_target_get(widget, action);
            if (target)
                gx_system_screen_stack_push(target);
            break;
        case GX_ACTION_TYPE_SCREEN_STACK_POP:
            gx_system_screen_stack_pop();
            break;
        case GX_ACTION_TYPE_SCREEN_STACK_RESET:
            gx_system_screen_stack_reset();
            break;
        default:
            break;
        }
        action++;
    }
    return true;
}

bool ux_action_event_filter(const struct ux_event *uevent, GX_EVENT *event_ptr)
{
    if (uevent->type != event_ptr->gx_event_type)
        return false;
    if ((uevent->type == GX_EVENT_ANIMATION_COMPLETE) &&
        (uevent->sender != event_ptr->gx_event_sender))
        return false;
    return true;
}

bool ux_action_event_bypass_filter(const struct ux_event *uevent, GX_EVENT *event_ptr)
{
    (void)uevent;
    (void)event_ptr;
    return true;
}

UINT ux_action_event_wrapper(GX_WIDGET *widget, 
    GX_EVENT *event_ptr, 
    const struct ux_event *events,
    bool (*event_filter)(const struct ux_event *, GX_EVENT *),
    UINT (*event_fn)(GX_WIDGET *, GX_EVENT *))
{
    for (const struct ux_event *uevent = events; uevent->type; uevent++) {
        if (!event_filter(uevent, event_ptr))
            continue;
        if (!ux_action_process(widget, uevent->actions))
            return event_ptr->gx_event_sender;
    }
    if (event_fn)
        return event_fn(widget, event_ptr);
    return GX_SUCCESS;
}
