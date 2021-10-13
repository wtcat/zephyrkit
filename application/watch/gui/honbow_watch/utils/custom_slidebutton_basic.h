#ifndef __CUSTOM_SLIDEBUTTON_BASIC_H__
#define __CUSTOM_SLIDEBUTTON_BASIC_H__
#include "gx_api.h"

typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID bg_default_color;
	GX_RESOURCE_ID bg_active_color;

	GX_RESOURCE_ID slide_icon;
	GX_PROMPT desc;
	INT pos_offset;
	INT cur_offset;
	INT move_start;

	GX_VALUE icon_width;
} Custom_SlideButton_TypeDef;

void custom_slide_button_create(Custom_SlideButton_TypeDef *button, USHORT widget_id, GX_WIDGET *parent,
								GX_RECTANGLE *size, GX_RESOURCE_ID color_bg_default, GX_RESOURCE_ID color_bg_active,
								GX_RESOURCE_ID icon_id, GX_STRING *desc, GX_RESOURCE_ID font_id,
								GX_RESOURCE_ID font_color, INT offset);
#endif