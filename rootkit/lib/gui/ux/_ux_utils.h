#ifndef UX__UX_UTILS_H_
#define UX__UX_UTILS_H_

#include "gx_system.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ux_action;

#define UX_TIMER_ID(widget) ((UINT)(widget))

/*
 * Current screen pointer
 */


/*
 * Language string definition macro
 */
#define _USTR(_type, ...) [_type] = (const GX_CHAR *[]){ __VA_ARGS__ }
#define USTR_START(name) \
    static const GX_CHAR **const __utils_##name##__[] = {
#define USTR_END(name) };\
    static inline const GX_CHAR **name##_language_string_get(void) { \
        GX_UBYTE id = ux_current_language_get(); \
        return __utils_##name##__[id]; \
    } \
    static inline void name##_language_string_set(INT offset, GX_STRING *str) { \
        GX_UBYTE id = ux_current_language_get(); \
        ux_language_string_set(__utils_##name##__[id], offset, str); \
    }

#define ux_widget_string_set(ustr, offset, widget, fn) \
do { \
    GX_STRING __str; \
    ustr##_language_string_set(offset, &__str); \
    fn((void *)widget, &__str); \
} while (0)

#define ux_root_window_get(match, context) \
    _gx_system_root_window_created_list
#define ux_root_window_display_get(_root) (_root)? \
    (_root)->gx_window_root_canvas->gx_canvas_display: \
    NULL

static inline void ux_language_string_set(const GX_CHAR **table, 
    INT offset, GX_STRING *str)
{
    const char *pstr = table[offset];
    str->gx_string_ptr = pstr;
    str->gx_string_length = strlen(pstr);
}

static inline GX_UBYTE ux_current_language_get(void)
{
    GX_WINDOW_ROOT *root = ux_root_window_get(NULL, NULL);
    return root->gx_window_root_canvas->gx_canvas_display->
        gx_display_active_language;
}

/*
 * Input capture stack
 */
void ux_input_capture_stack_release(GX_WIDGET *notifer);


/*
 * Screen switch
 */
UINT __ux_screen_move(GX_WIDGET *parent, GX_WIDGET *from, GX_WIDGET *to,
    const GX_ANIMATION_INFO *info);
UINT ux_screen_push_and_switch(GX_WIDGET *curr, GX_WIDGET *next, 
    const GX_ANIMATION_INFO *info);
UINT ux_screen_pop_and_switch(GX_WIDGET *curr, const GX_ANIMATION_INFO *info);
UINT ux_screen_goto(GX_WIDGET *parent, GX_WIDGET *from, GX_WIDGET *to,
    struct ux_action *actions);

static inline UINT ux_screen_move(GX_WIDGET *from, GX_WIDGET *to,
    const GX_ANIMATION_INFO *info)
{
    return __ux_screen_move(from->gx_widget_parent, from, to, info);
}

/*
 * Draw round-rectangle and pixelmap
 */
UINT ux_rectangle_background_render(const GX_RECTANGLE *client, GX_RESOURCE_ID color,
    UINT style, INT delta, GX_COLOR(*color_preprocess)(GX_COLOR));
UINT __ux_rectangle_pixelmap_draw(const GX_RECTANGLE *client, GX_RESOURCE_ID pixelmap,
    UINT style, GX_UBYTE alpha);
UINT ux_rectangle_pixelmap_draw(const GX_RECTANGLE *client, GX_RESOURCE_ID pixelmap,
    UINT style);
UINT ux_round_rectangle_render(const GX_RECTANGLE *client, GX_RESOURCE_ID fill_color,
    GX_VALUE boardwidth);
UINT ux_round_rectangle_render2(const GX_RECTANGLE *client,
                                GX_VALUE boardwidth,
                                GX_VALUE upper_width,
                                GX_RESOURCE_ID color_upper,
                                GX_RESOURCE_ID color_bottom);
UINT ux_arc_progress_render(INT xorg, INT yorg, UINT r, UINT width,
    INT start_angle, INT end_angle, GX_RESOURCE_ID color);
UINT ux_circle_render(INT xorg, INT yorg, UINT r, GX_RESOURCE_ID color);

#ifdef __cplusplus
}
#endif

#endif /* UX__UX_UTILS_H_ */
