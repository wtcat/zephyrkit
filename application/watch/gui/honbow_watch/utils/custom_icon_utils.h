#ifndef __CUSTOM_BUTTON_UTILS_H__
#define __CUSTOM_BUTTON_UTILS_H__
#include "gx_api.h"

typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID bg_color;
	GX_RESOURCE_ID icon;
} Custom_Icon_TypeDef;
void custom_icon_create(Custom_Icon_TypeDef *element, USHORT widget_id, GX_WIDGET *parent, GX_RECTANGLE *size,
						GX_RESOURCE_ID color_id, GX_RESOURCE_ID icon_id);
void custom_icon_draw(GX_WIDGET *widget);
void custom_icon_change_bg_color(Custom_Icon_TypeDef *element, GX_RESOURCE_ID curr_color);
void custom_icon_change_pixelmap(Custom_Icon_TypeDef *element, GX_RESOURCE_ID map_id);
#endif