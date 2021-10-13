#ifndef POPUP_WIDGET_H_
#define POPUP_WIDGET_H_

#include "gx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEFINE_WIDGET_VAR
const GX_ANIMATION_INFO popup_hide_animation[] = {
    {
        .gx_animation_style = GX_ANIMATION_ELASTIC_EASE_IN_OUT,
        .gx_animation_id = 0x1001,
        .gx_animation_start_delay = 0,
        .gx_animation_frame_interval = 1,
        .gx_animation_start_position = {0, 0},
        .gx_animation_end_position = MOV_UP_DIRECTION,
        .gx_animation_start_alpha = 255,
        .gx_animation_end_alpha = 255,
        .gx_animation_steps = 15
    }
};
#else /* !DEFINE_WIDGET_VAR */

extern const GX_ANIMATION_INFO popup_hide_animation[];
#endif /* DEFINE_WIDGET_VAR */

enum goal_type {
    GOAL_1,
    GOAL_2,
    GOAL_3,
    GOAL_4
};


UINT message_window_create(const char *name, GX_WIDGET *parent, GX_WIDGET **ptr);
UINT language_window_create(const char *name, GX_WIDGET *parent, GX_WIDGET **ptr);
UINT pairing_window_create(const char *name, GX_WIDGET *parent, GX_WIDGET **ptr);

UINT shutdown_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr);
GX_WIDGET *shutdown_widget_get(void);

UINT ringing_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr);
GX_WIDGET *ringing_widget_get(void);

UINT ota_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr);
GX_WIDGET *ota_widget_get(void);
void ota_progress_set(GX_WIDGET *ota, INT value);

UINT hrov_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr);
GX_WIDGET *hrov_widget_get(INT value);

UINT charging_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr);
GX_WIDGET *charging_widget_get(INT level);

UINT sedentary_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr);
GX_WIDGET *sedentary_widget_get(INT value);

UINT alarm_remind_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr);
GX_WIDGET *alarm_remind_widget_get(GX_VALUE hour, GX_VALUE min, GX_BOOL use_hour12);

UINT goal_arrived_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr);
GX_WIDGET *goal_arrived_widget_get(enum goal_type type, INT value);

UINT lowbat_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr);
GX_WIDGET *lowbat_widget_get(INT power_lvl);

UINT shutdown_ani_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr);
GX_WIDGET *shutdown_ani_widget_get(void);

UINT sport_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr);
GX_WIDGET *sport_widget_get(void);

/* Only for test */
UINT pop_test_window_create(GX_WIDGET *parent, GX_WIDGET **ptr);

#ifdef __cplusplus
}
#endif
#endif /* POP_WIDGET_H_ */