#ifndef __VIEW_SERVICE_CUSTOM_H__
#define __VIEW_SERVICE_CUSTOM_H__
#include "kernel.h"
#include "stdint.h"
#include "sys/slist.h"

#define MAX_QUEUE_EVENTS 10

typedef enum {
	VIEW_EVENT_TYPE_BRIGHRNESS_CHG = 0,
	VIEW_EVENT_TYPE_BRIGHT_TIME_CHG,
	VIEW_EVENT_TYPE_WRIST_RAISE_CFG_CHG,
	VIEW_EVENT_TYPE_NIGHT_MODE_CFG_CHG,
	VIEW_EVENT_TYPE_APP_VIEW_CFG_CHG,
	VIEW_EVENT_TYPE_ALARM_ADD,
	VIEW_EVENT_TYPE_ALARM_DEL,
	VIEW_EVENT_TYPE_ALARM_EDITED,
	VIEW_EVENT_TYPE_REMINDERS_ADD,
	VIEW_EVENT_TYPE_REMINDERS_DEL,
	VIEW_EVENT_TYPE_REMINDERS_EDITED,
	VIEW_EVENT_TYPE_REBOOT,
	VIEW_EVENT_TYPE_STOP_WATCH_OP,
	VIEW_EVENT_TYPE_MAX,
} VIEW_EVENT_TYPE;

typedef struct VIEW_EVENT_STRUCT {
	VIEW_EVENT_TYPE view_event_type; /* Global event type                        */
	uint16_t view_event_sender;		 /* ID of the event sender                   */
	void *data;
} VIEW_EVENT;

struct view_event {
	sys_snode_t node;
	VIEW_EVENT event;
};

struct view_service_struct {
	struct view_event events[MAX_QUEUE_EVENTS];
	sys_slist_t pending;
	sys_slist_t free;
	struct k_spinlock lock;
	struct k_sem wait;
};

typedef void (*view_event_handle)(VIEW_EVENT_TYPE type, void *data);

void view_service_init(void);
int view_service_callback_reg(VIEW_EVENT_TYPE type, view_event_handle handle);
void view_service_event_submit(VIEW_EVENT_TYPE type, void *data, int data_size);

#endif
