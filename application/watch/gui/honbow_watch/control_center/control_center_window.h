#ifndef __CONTROL_CENTER_WINDOW_H__
#define __CONTROL_CENTER_WINDOW_H__
#include "gx_api.h"

#define NIGHT_MODE_ICON_ID 1
#define BRIGHT_SCREEN_ON_ICON_ID 2
#define DISTURB_MODE_ICON_ID 3
#define BRIGHTNESS_ICON_ID 4
#define SET_UP_ICON_ID 5

void control_center_window_draw(GX_WIDGET *widget);
void control_center_window_init(GX_WIDGET *parent);
UINT control_center_window_event(GX_WINDOW *window, GX_EVENT *event_ptr);
#endif