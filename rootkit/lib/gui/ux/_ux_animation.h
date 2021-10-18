#ifndef UX_UX_ANIMATION_H_
#define UX_UX_ANIMATION_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UX_ANIMATION_STRUCT {
    GX_ANIMATION animation;
    ULONG magic_number;
    VOID(*gx_animation_pendown)(GX_ANIMATION *, GX_ANIMATION_INFO *, GX_EVENT *);
    UINT(*gx_animation_slide_landing)(GX_ANIMATION *);
    UINT(*gx_animation_slide_landing_start)(GX_ANIMATION *);
    UINT(*gx_animation_drag_tracking)(GX_ANIMATION *, GX_POINT);
    UINT(*gx_animation_drag_tracking_start)(GX_ANIMATION *, GX_POINT);
    UINT(*gx_animation_drag_event_check)(GX_ANIMATION *,
        GX_EVENT *,
        UINT(*drag_tracking_start)(GX_ANIMATION *, GX_POINT),
        UINT(*drag_tracking)(GX_ANIMATION *, GX_POINT),
        UINT(*slide_landing)(GX_ANIMATION *),
        UINT(*slide_landing_start)(GX_ANIMATION *),
        VOID(*pend_down)(GX_ANIMATION *, GX_ANIMATION_INFO *, GX_EVENT *));
} UX_ANIMATION;

UINT ux_animation_drag_enable(UX_ANIMATION *animation, GX_WIDGET *widget, GX_ANIMATION_INFO *info);

#ifdef __cplusplus
}
#endif
#endif /* UX_UX_ANIMATION_H_ */