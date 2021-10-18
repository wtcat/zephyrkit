#ifndef UX_UX_ACTION_H_
#define UX_UX_ACTION_H_

#ifdef __cplusplus
extern "C" {
#endif


#define UX_ACTION_FLAG_DYNAMIC_TARGET 0x01
#define UX_ACTION_FLAG_DYNAMIC_PARENT 0x02
#define UX_ACTION_FLAG_POP_TARGET     0x04
#define UX_ACTION_FLAG_POP_PARENT     0x08

struct ux_action {
    GX_UBYTE opcode;
    GX_UBYTE flags;
    void *parent;
    void *target;
    const GX_ANIMATION_INFO *animation;
};

struct ux_event {
    ULONG  type;
    USHORT sender;
    const struct ux_action *actions;
};

#define UX_ACTION_NULL {0, 0, GX_NULL, GX_NULL, GX_NULL}
#define UX_EVENT_NULL  {0, 0, GX_NULL}

#define _UX_ACT(opcode, flags, parent, target, animation) \
    {opcode, flags, parent, target, animation}

#define UX_ACT(opcode, animation) \
    _UX_ACT(opcode, UX_ACTION_FLAG_DYNAMIC_TARGET, NULL, NULL, animation)

#define UX_ACTIONS(_name, ...) \
    struct ux_action _name[] = {\
        __VA_ARGS__, \
        UX_ACTION_NULL, \
    }

#define _UX_EVT(type, sender, actions) \
    {type, sender, actions}
#define UX_EVENTS(_name, ...) \
    struct ux_event _name[] = { \
        __VA_ARGS__, \
        UX_EVENT_NULL \
    }

void ux_animation_execute(GX_WIDGET *current, const struct ux_action *action);
bool ux_action_process(GX_WIDGET *widget, const struct ux_action *actions);
bool ux_action_event_bypass_filter(const struct ux_event *uevent, GX_EVENT *event_ptr);
bool ux_action_event_filter(const struct ux_event *uevent, GX_EVENT *event_ptr);
UINT ux_action_event_wrapper(GX_WIDGET *widget, GX_EVENT *event_ptr,
    const struct ux_event *events,
    bool (*event_filter)(const struct ux_event *, GX_EVENT *),
    UINT(*event_fn)(GX_WIDGET *, GX_EVENT *));

#ifdef __cplusplus
}
#endif
#endif /* UX_UX_ACTION_H_ */
