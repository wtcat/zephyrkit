#ifndef __APP_SETTING_DISP_PAGE_H__
#define __APP_SETTING_DISP_PAGE_H__
#include "gx_api.h"

typedef enum {
	DISP_SETTING_CHILD_PAGE_BRIGHTNESS = 0,
	DISP_SETTING_CHILD_PAGE_BRIGHT_TIME,
	DISP_SETTING_CHILD_PAGE_WRIST_RAISE,
	DISP_SETTING_CHILD_PAGE_NIGHT_MODE,
	DISP_SETTING_CHILD_PAGE_ECO_MODE,
} DISP_SETTING_CHILD_PAGE_T;

void setting_disp_page_children_reg(GX_WIDGET *dst, DISP_SETTING_CHILD_PAGE_T id);
void setting_disp_page_children_return(GX_WIDGET *curr);

#endif