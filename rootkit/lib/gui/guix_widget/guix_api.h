#ifndef GUIX_API_H_
#define GUIX_API_H_

#include "gx_api.h"

#ifdef __cplusplus
extern "C"{
#endif

enum slide_direction {
    SCREEN_SLIDE_LEFT,
    SCREEN_SLIDE_TOP,
    SCREEN_SLIDE_RIGHT,
    SCREEN_SLIDE_BOTTOM
};

struct slide_list {
    GX_ANIMATION anim[2];
    GX_ANIMATION_INFO anim_info[2];
};

struct checkbox_struct {
    GX_CHECKBOX_MEMBERS_DECLARE
    GX_RESOURCE_ID background_id;
    INT start_offset;
    INT end_offset;
    INT cur_offset;
    USHORT state;
};

struct checkbox_info {
    INT widget_id;
    GX_RESOURCE_ID background_id;
    GX_RESOURCE_ID checked_map_id;
    GX_RESOURCE_ID unchecked_map_id;
    INT start_offset;
    INT end_offset;
};

typedef enum slide_direction GUIX_SLIDE_DIR;
typedef struct slide_list GUIX_SLIDE_LIST;
typedef struct checkbox_struct GUIX_CHECKBOX;
typedef struct checkbox_info GUIX_CHECKBOX_INFO;


/* Point to current sceen */
extern GX_WIDGET *current_screen_ptr;


VOID guix_screen_switch_to(GX_WIDGET *parent, GX_WIDGET *next);
VOID guix_screen_slide_list_create(GUIX_SLIDE_LIST *list, USHORT id);
UINT guix_screen_slide_list_set(GUIX_SLIDE_LIST *list, USHORT frame_interval, 
    GX_UBYTE steps, GX_UBYTE start_alpha, GX_UBYTE end_alpha);
VOID guix_screen_slide_list_start(GUIX_SLIDE_LIST *handle, GX_WIDGET *parent, 
    GX_WIDGET **list, enum slide_direction dir);

VOID guix_checkbox_create(GUIX_CHECKBOX *checkbox, GX_WIDGET *parent, 
    GUIX_CHECKBOX_INFO *info, GX_RECTANGLE *size);

#ifdef __cplusplus
}
#endif
#endif /* GUIX_API_H_ */
