#ifndef __CUSTOM_WINDOW_TITLE_H__
#define __CUSTOM_WINDOW_TITLE_H__
#include "gx_api.h"

typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID bg_color;
	GX_RESOURCE_ID icon;
} Custom_title_icon_t;

extern void custom_window_title_create(GX_WIDGET *parent, Custom_title_icon_t *icon_tips, GX_PROMPT *info_tips,
									   GX_PROMPT *time_tips);
#endif