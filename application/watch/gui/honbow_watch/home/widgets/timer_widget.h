#ifndef TIMER_WIDGET_H__
#define TIMER_WIDGET_H__
#include "gx_api.h"
typedef struct {
	GX_PIXELMAP_BUTTON counter_down_widget;
	GX_PROMPT prompt_counter_value;
	GX_PROMPT prompt_unit;
	GX_CHAR *counter_value;
	GX_CHAR *unit_value;
	USHORT button_id;
} TIMER_WIDGET_TYPE;

#endif