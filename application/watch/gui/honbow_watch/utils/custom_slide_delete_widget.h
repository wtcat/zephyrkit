#ifndef __CUSTOM_SLIDE_DELETE_WIDGET_H__
#define __CUSTOM_SLIDE_DELETE_WIDGET_H__
#include "gx_api.h"
#include "custom_rounded_button.h"
typedef struct {
	GX_WIDGET widget_main;
	GX_WIDGET *child1;
	GX_WIDGET *child2;
	GX_POINT widget_move_start;
	GX_VALUE snap_back_distance;
} CUSTOM_SLIDE_DEL_WIDGET;

void custom_deletable_slide_widget_create(CUSTOM_SLIDE_DEL_WIDGET *widget, USHORT widget_id, GX_WIDGET *parent,
										  GX_RECTANGLE *size, GX_WIDGET *child1, GX_WIDGET *child2);
UINT custom_deletable_slide_widget_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr);
void custom_deletable_slide_widget_reset(CUSTOM_SLIDE_DEL_WIDGET *widget, GX_POINT *point, GX_BOOL froce_flag);
#endif