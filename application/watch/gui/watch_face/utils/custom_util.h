#include <stdint.h>
#ifndef __GX_CUSTOM_UTIL_H__
#define __GX_CUSTOM_UTIL_H__

#include "gx_api.h"

VOID screen_switch(GX_WIDGET *parent, GX_WIDGET **current_screen,
				   GX_WIDGET *new_screen);

UINT string_length_get(GX_CONST GX_CHAR *input_string, UINT max_string_length);

#endif