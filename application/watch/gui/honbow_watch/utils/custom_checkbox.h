/* This is a small demo of the high-performance GUIX graphics framework. */

#ifndef __GX_CUSTOM_CHECKBOX_H__
#define __GX_CUSTOM_CHECKBOX_H__

#include "gx_api.h"

#define CHECKBOX_STATE_NONE 0x01
#define CHECKBOX_STATE_SLIDING 0x02

/* Define custom checkbox structure. */
typedef struct CUSTOM_CHECBOX_STRUCT {
	GX_CHECKBOX_MEMBERS_DECLARE
	GX_RESOURCE_ID background_id;
	INT start_offset;
	INT end_offset;
	INT cur_offset;
	USHORT state;
	GX_BOOL flag_effect_from_external;
} CUSTOM_CHECKBOX;

typedef struct CUSTOM_CHECKBOX_INFO_STRUCT {
	INT widget_id;
	GX_RESOURCE_ID background_id;
	GX_RESOURCE_ID checked_map_id;
	GX_RESOURCE_ID unchecked_map_id;
	INT start_offset;
	INT end_offset;
} CUSTOM_CHECKBOX_INFO;

VOID custom_checkbox_create(CUSTOM_CHECKBOX *button, GX_WIDGET *parent, const CUSTOM_CHECKBOX_INFO *info,
							GX_RECTANGLE *size);
void custom_checkbox_status_change(CUSTOM_CHECKBOX *checkbox);
void custom_checkbox_event_from_external_enable(CUSTOM_CHECKBOX *checkbox, GX_BOOL flag);
#endif