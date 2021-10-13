#ifndef __APP_ALARM_PAGE_MGR_H__
#define __APP_ALARM_PAGE_MGR_H__
#include "gx_api.h"
#include "view_service_alarm.h"
#include "view_service_custom.h"
typedef enum {
	ALARM_PAGE_MAIN = 0,
	ALARM_PAGE_EDIT,
	ALARM_PAGE_TIME_SEL,
	ALARM_PAGE_RPT_SEL,
	ALARM_PAGE_VIB_DURATION,
	ALARM_PAGE_MAX
} ALARM_PAGE_T;

typedef enum {
	ALARM_OP_CANCEL = 0,
	ALARM_OP_ADD_NEW,
	ALARM_OP_EDIT,
	ALARM_OP_NEXT_OR_FINISH,
	ALARM_OP_TIME_SEL,
	ALARM_OP_RPT_SEL,
	ALARM_OP_VIB_DURATION,
} ALARM_OP_T;

typedef enum {
	ALARM_EDIT_TYPE_WIZZARD = 0x55,
	ALARM_EDIT_TYPE_SINGLE = 0xAA,
} ALARM_EDIT_TYPE;

typedef int (*alarm_page_info_update)(ALARM_EDIT_TYPE, ALARM_INFO_T *);
void app_alarm_page_mgr_init(void);
void app_alarm_instance_record(ALARM_INFO_T *info);
void app_alarm_page_reg(ALARM_PAGE_T type, GX_WIDGET *widget,
						int (*alarm_page_info_update)(ALARM_EDIT_TYPE, ALARM_INFO_T *));
void app_alarm_page_jump(ALARM_PAGE_T curr_type, ALARM_OP_T op);

#endif