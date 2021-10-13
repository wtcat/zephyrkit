#ifndef __CUSTOM_ROUNDED_BUTTON_H__
#define __CUSTOM_ROUNDED_BUTTON_H__
#include "gx_api.h"

#define ROUND_BUTTON_OVERLAY_PIXELMAP_HEIGHT 10
#define ROUND_BUTTON_OVERLAY_PIXELMAP_WIDTH 10

typedef enum {
	CUSTOM_ROUNDED_BUTTON_ICON_ONLY = 0,
	CUSTOM_ROUNDED_BUTTON_TEXT_MIDDLE,
	CUSTOM_ROUNDED_BUTTON_TEXT_LEFT,
} Custom_RounderButton_DispType;

typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID bg_color;
	GX_RESOURCE_ID icon;
	GX_PROMPT desc;
	GX_PIXELMAP *overlay_map;
	Custom_RounderButton_DispType disp_type;
	GX_BOOL disable_flag;
	GX_UBYTE disable_alpha_value;
} Custom_RoundedButton_TypeDef;

typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID bg_color;
	GX_RESOURCE_ID icon;
} Sport_goal_icon_t;

typedef struct {
	GX_WIDGET widget;
	Sport_goal_icon_t goal_set;
	GX_RESOURCE_ID bg_color;
	GX_RESOURCE_ID icon0;
	GX_RESOURCE_ID icon1;
	GX_PROMPT desc;
	UCHAR disp_type;
	UCHAR id;
} Custom_RoundedIconButton_TypeDef;

void custom_rounded_button_create(Custom_RoundedButton_TypeDef *element, USHORT widget_id, GX_WIDGET *parent,
								  GX_RECTANGLE *size, GX_RESOURCE_ID color_id, GX_RESOURCE_ID icon_id, GX_STRING *desc,
								  Custom_RounderButton_DispType disp_type);
void custom_rounded_button_create2(Custom_RoundedButton_TypeDef *element, USHORT widget_id, GX_WIDGET *parent,
								   GX_RECTANGLE *size, GX_RESOURCE_ID color_id, GX_RESOURCE_ID icon_id, GX_STRING *desc,
								   GX_RESOURCE_ID font_id, Custom_RounderButton_DispType disp_type);
void custom_rounded_button_create_3(Custom_RoundedButton_TypeDef *element, USHORT widget_id, GX_WIDGET *parent,
									GX_RECTANGLE *size, GX_RESOURCE_ID color_id, GX_RESOURCE_ID icon_id,
									GX_STRING *desc, GX_RESOURCE_ID font_id, Custom_RounderButton_DispType disp_type,
									GX_UBYTE text_only_mode_offset_value, GX_UBYTE disabled_alpha_value);
void custom_rounded_button_align_create(Custom_RoundedIconButton_TypeDef *element, USHORT widget_id, GX_WIDGET *parent,
										GX_RECTANGLE *size, GX_RESOURCE_ID color_id, GX_RESOURCE_ID icon_id0,
										GX_RESOURCE_ID icon_id1, GX_STRING *desc, INT x_offset, INT y_offset);
#endif