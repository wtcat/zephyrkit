#ifndef __APP_REMINDERS_PAGE_MGR_H__
#define __APP_REMINDERS_PAGE_MGR_H__
#include "gx_api.h"
#include "view_service_reminders.h"
#include "view_service_custom.h"
typedef enum {
	REMINDERS_PAGE_MAIN = 0,
	REMINDERS_PAGE_EDIT,
	REMINDERS_PAGE_TIME_SEL,
	REMINDERS_PAGE_RPT_SEL,
	REMINDERS_PAGE_VIB_DURATION,
	REMINDERS_PAGE_MAX
} REMINDERS_PAGE_T;

typedef enum {
	REMINDERS_OP_CANCEL = 0,
	REMINDERS_OP_ADD_NEW,
	REMINDERS_OP_EDIT,
	REMINDERS_OP_NEXT_OR_FINISH,
	REMINDERS_OP_TIME_SEL,
	REMINDERS_OP_RPT_SEL,
	REMINDERS_OP_VIB_DURATION,
} REMINDERS_OP_T;

typedef enum {
	REMINDERS_EDIT_TYPE_WIZZARD = 0x55,
	REMINDERS_EDIT_TYPE_SINGLE = 0xAA,
} REMINDERS_EDIT_TYPE;

typedef int (*reminders_page_info_update)(REMINDERS_EDIT_TYPE, REMINDER_INFO_T *);
void app_reminders_page_mgr_init(void);
void app_reminders_instance_record(REMINDER_INFO_T *info);
void app_reminders_page_reg(REMINDERS_PAGE_T type, GX_WIDGET *widget,
							int (*alarm_page_info_update)(REMINDERS_EDIT_TYPE, REMINDER_INFO_T *));
void app_reminders_page_jump(REMINDERS_PAGE_T curr_type, REMINDERS_OP_T op);

#endif