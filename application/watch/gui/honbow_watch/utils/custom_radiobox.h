#ifndef __CUSTOM_RADIO_BOX_H__
#define __CUSTOM_RADIO_BOX_H__

#include "gx_api.h"

typedef struct CUSTOM_RADIOBOX_STRUCT {
	GX_WIDGET widget;
	GX_RESOURCE_ID color_id_select;
	GX_RESOURCE_ID color_id_deselect;
	GX_RESOURCE_ID color_id_background;
} CUSTOM_RADIOBOX;

void custom_radiobox_create(CUSTOM_RADIOBOX *checkbox, GX_WIDGET *parent, GX_RECTANGLE *size);
void custom_radiobox_status_change(CUSTOM_RADIOBOX *radiobox, GX_BOOL flag);
#endif