#ifndef UX_UX_WRAPPER_H_
#define UX_UX_WRAPPER_H_

#ifdef __cplusplus
extern "C"{
#endif

#define POP_WIDGET_TIMER_ID 0xB001

#define ux_key_click_event_wrapper(a, b, k, c, d, f) \
    _ux_key_event_wrapper(a, b, k, c, d, key_event_empty_fire, f)

#define ux_key_timeout_event_wrapper(a, b, k, c, e, f) \
    _ux_key_event_wrapper(a, b, k, c, key_event_empty_fire, e, f)

#define _ux_key_event_wrapper(a, b, k, c, d, e, f) \
    __ux_key_event_wrapper((GX_WIDGET *)a, b, k, c, \
        (UINT (*)(GX_WIDGET *widget))d, \
        (UINT (*)(GX_WIDGET *widget))e, \
        (UINT (*)(GX_WIDGET *, GX_EVENT *))f)

UINT key_event_empty_fire(GX_WIDGET *widget);
UINT __ux_key_event_wrapper(GX_WIDGET *widget, GX_EVENT *event_ptr,
     USHORT key,
     UINT timeout,
     UINT (*click_fire)(GX_WIDGET *widget),
     UINT (*timeout_fire)(GX_WIDGET *widget),
     UINT (*event_fn)(GX_WIDGET *, GX_EVENT *));

UINT pop_widget_event_process_wrapper(GX_WIDGET *widget,
    GX_EVENT *event_ptr,
    UINT timeout,
    UINT(*event_process)(GX_WIDGET *, GX_EVENT *),
    const GX_ANIMATION_INFO *info);

#ifdef __cplusplus
}
#endif
#endif /* UX_UX_WRAPPER_H_ */