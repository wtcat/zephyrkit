#ifndef UX_UX_PIXELMAPBUTTON_H_
#define UX_UX_PIXELMAPBUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif

struct pixelmap_button {
    GX_PIXELMAP_BUTTON button;
    GX_RESOURCE_ID normal_fill_color;
    GX_RESOURCE_ID selected_fill_color;
    void (*color_set)(GX_WIDGET *, struct pixelmap_button *);
};
typedef struct pixelmap_button UX_PIXELMAP_BUTTON;

struct ux_text_button {
    GX_TEXT_BUTTON button;
    GX_RESOURCE_ID normal_fill_color;
    GX_RESOURCE_ID selected_fill_color;
    void (*color_set)(GX_WIDGET *, struct ux_text_button *);
};
typedef struct ux_text_button UX_TEXT_BUTTON;

UINT ux_pixelmap_button_create(UX_PIXELMAP_BUTTON *button,
    const char *name,
    GX_WIDGET *parent,
    GX_RESOURCE_ID pixelmap_id,
    GX_RESOURCE_ID normal_color_id,
    GX_RESOURCE_ID selected_color_id,
    USHORT button_id,
    ULONG style,
    GX_CONST GX_RECTANGLE *size,
    void (*color_set)(GX_WIDGET *, struct pixelmap_button *));
    
UINT ux_text_button_create(UX_TEXT_BUTTON *button,
    const char *name,
    GX_WIDGET *parent,
    GX_RESOURCE_ID text_id,
    GX_RESOURCE_ID normal_bg_color,
    GX_RESOURCE_ID selected_bg_color,
    USHORT button_id,
    ULONG style,
    GX_CONST GX_RECTANGLE *size,
    void (*color_set)(GX_WIDGET *, struct ux_text_button *));
#ifdef __cplusplus
}
#endif
#endif /* UX_UX_PIXELMAPBUTTON_H_ */