#ifndef __CUSTOM_SCREEN_STACK_H__
#define __CUSTOM_SCREEN_STACK_H__
#include "gx_api.h"
UINT custom_screen_stack_create(GX_SCREEN_STACK_CONTROL *control, GX_WIDGET **memory, INT size);
UINT custom_screen_stack_push(GX_SCREEN_STACK_CONTROL *control, GX_WIDGET *screen, GX_WIDGET *new_screen);
UINT custom_screen_stack_pop(GX_SCREEN_STACK_CONTROL *control);
UINT custom_screen_stack_reset(GX_SCREEN_STACK_CONTROL *control);

#endif