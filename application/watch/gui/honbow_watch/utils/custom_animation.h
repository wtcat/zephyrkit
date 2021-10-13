#ifndef __CUSTOM_ANIMATION_H__
#define __CUSTOM_ANIMATION_H__
#include "guix_api.h"
void custom_animation_init(void);
int custom_animation_start(GX_WIDGET **list, GX_WIDGET *parent, enum slide_direction dir);
GX_BOOL custom_animation_busy_check(void);
#endif