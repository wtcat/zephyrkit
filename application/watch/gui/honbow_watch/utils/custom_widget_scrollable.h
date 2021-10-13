#ifndef __CUSTOM_WIDGET_SCROLLABLE_H__
#define __CUSTOM_WIDGET_SCROLLABLE_H__
#include "gx_api.h"

typedef struct CUSTOM_WIDGET_STRUCT {
	GX_WINDOW widget;
	GX_VALUE gx_snap_back_distance;

} CUSTOM_SCROLLABLE_WIDGET;

VOID custom_scrollable_widget_create(CUSTOM_SCROLLABLE_WIDGET *widget, GX_WIDGET *parent, GX_RECTANGLE *size);
UINT custom_scrollable_widget_event_process(CUSTOM_SCROLLABLE_WIDGET *custom_widget, GX_EVENT *event_ptr);
void custom_scrollable_widget_reset(CUSTOM_SCROLLABLE_WIDGET *custom_widget);
#endif