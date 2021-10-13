#ifndef __HOME_WINDOW_H__
#define __HOME_WINDOW_H__
#include "gx_api.h"
#include "stdbool.h"
extern GX_ANIMATION slide_ver_animation;
extern GX_ANIMATION slide_hor_animation;
UINT home_window_event(GX_WINDOW *window, GX_EVENT *event_ptr);
bool home_window_is_pen_down(void);
bool home_window_drag_anim_busy(void);
void home_window_init(GX_WIDGET *home);

typedef enum {
	HOME_WIDGET_INDEX_LEFT_1 = 0,
	HOME_WIDGET_INDEX_LEFT_2,
	HOME_WIDGET_INDEX_CENTER,
	HOME_WIDGET_INDEX_RIGHT_2,
	HOME_WIDGET_INDEX_RIGHT_1,
	HOME_WIDGET_INDEX_MAX
} HOME_WIDGET_INDEX;

typedef enum {
	HOME_WIDGET_TYPE_MUSIC = 0,
	HOME_WIDGET_TYPE_HR,
	HOME_WIDGET_TYPE_COMPASS,
	HOME_WIDGET_TYPE_STOPWATCH,
	HOME_WIDGET_TYPE_COUNTER_DOWN,
	HOME_WIDGET_TYPE_OVERVIEW,
	HOME_WIDGET_TYPE_WORKOUT,
	HOME_WIDGET_TYPE_MAX
} HOME_WIDGET_TYPE;
void home_widget_type_reg(GX_WIDGET *widget, HOME_WIDGET_TYPE type);

#endif