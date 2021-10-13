#ifndef __CUSTOM_CHECKBOX_BASIC_H__
#define __CUSTOM_CHECKBOX_BASIC_H__
#include "gx_api.h"

typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID background_checked;
	GX_RESOURCE_ID background_uchecked;
	GX_RESOURCE_ID rolling_ball_color;
	GX_VALUE curr_offset;
	GX_VALUE pos_offset;
	GX_BOOL effect_from_external;
	GX_VALUE rolling_ball_r;
} CUSTOM_CHECKBOX_BASIC;

typedef struct {
	INT widget_id;
	GX_RESOURCE_ID background_checked;
	GX_RESOURCE_ID background_uchecked;
	GX_RESOURCE_ID rolling_ball_color;
	GX_VALUE pos_offset;
} CUSTOM_CHECKBOX_BASIC_INFO;

VOID custom_checkbox_basic_create(CUSTOM_CHECKBOX_BASIC *checkbox, GX_WIDGET *parent, CUSTOM_CHECKBOX_BASIC_INFO *info,
								  GX_RECTANGLE *size);
void custom_checkbox_basic_draw_func(GX_WIDGET *widget);
void custom_checkbox_basic_toggle(CUSTOM_CHECKBOX_BASIC *checkbox);
#endif