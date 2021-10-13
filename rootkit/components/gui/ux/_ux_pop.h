#ifndef UX__UX_POP_H_
#define UX__UX_POP_H_

#ifdef __cplusplus
extern "C" {
#endif

void pop_widget_fire(GX_WIDGET *curr, GX_WIDGET *next,
    const GX_ANIMATION_INFO *info);

#ifdef __cplusplus
}
#endif

#endif /* UX__UX_POP_H_ */
