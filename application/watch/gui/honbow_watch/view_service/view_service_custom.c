#include "view_service_custom.h"
#include <kernel.h>
#include <logging/log.h>
#include <sys/_timespec.h>
#include <time.h>
#include "drivers/counter.h"
#include "drivers_ext/rtc.h"
#include "posix/time.h"
#include "sys/timeutil.h"

LOG_MODULE_REGISTER(guix_view_service);
struct view_service_struct view_service;
view_event_handle handles[VIEW_EVENT_TYPE_MAX];

#define STACKSIZE 2048
#define PRIORITY 6
static struct k_thread thread_view_service;
static K_THREAD_STACK_DEFINE(thread_view_service_stack, STACKSIZE);
K_HEAP_DEFINE(view_service_heap, 2048);

static int view_generic_event_post(VIEW_EVENT *event_ptr)
{
	struct view_service_struct *service = &view_service;
	k_spinlock_key_t key;
	struct view_event *event;
	key = k_spin_lock(&service->lock);
	if (sys_slist_is_empty(&service->free)) {
		k_spin_unlock(&service->lock, key);
		return -1;
	}
	event = (struct view_event *)sys_slist_get_not_empty(&service->free);
	event->event = *event_ptr;
	if (sys_slist_is_empty(&service->pending)) {
		sys_slist_append(&service->pending, &event->node);
		k_sem_give(&service->wait);
	} else {
		sys_slist_append(&service->pending, &event->node);
	}
	k_spin_unlock(&service->lock, key);
	return 0;
}

static int view_generic_event_fold(VIEW_EVENT *event_ptr)
{
	struct view_service_struct *service = &view_service;
	k_spinlock_key_t key;
	struct view_event *event;
	key = k_spin_lock(&service->lock);

	sys_snode_t *node, *tmp, *prev = NULL;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&service->pending, node, tmp)
	{
		event = (struct view_event *)node;
		if (event->event.view_event_type == event_ptr->view_event_type) {
			k_heap_free(&view_service_heap, event->event.data);
			event->event.data = event_ptr->data;
			k_spin_unlock(&service->lock, key);
			return 0;
		}
		prev = node;
	}
	k_spin_unlock(&service->lock, key);
	return view_generic_event_post(event_ptr);
}

static int view_generic_event_pop(VIEW_EVENT *put_event, bool wait)
{
	struct view_service_struct *service = &view_service;
	k_spinlock_key_t key;
	struct view_event *event;

	key = k_spin_lock(&service->lock);
	if (!wait && sys_slist_is_empty(&service->pending)) {
		k_spin_unlock(&service->lock, key);
		return -1;
	}

	while (sys_slist_is_empty(&service->pending)) {
		k_spin_unlock(&service->lock, key);
		k_sem_take(&service->wait, K_FOREVER);
		key = k_spin_lock(&service->lock);
	}

	event = (struct view_event *)sys_slist_get_not_empty(&service->pending);
	*put_event = event->event;
	sys_slist_append(&service->free, &event->node);
	k_spin_unlock(&service->lock, key);
	return 0;
}
void view_service_thread(void *p1, void *p2, void *p3)
{
	VIEW_EVENT event;
	while (1) {
		view_generic_event_pop(&event, true);
		if (handles[event.view_event_type] != NULL) {
			handles[event.view_event_type](event.view_event_type, event.data);
		}
		k_heap_free(&view_service_heap, event.data);
		k_msleep(100); //最快100ms处理一条
	}
}

extern void view_service_display_init(void);
extern void view_service_night_mode_init(void);
extern void view_service_misc_init(void);
extern void view_service_alarm_init(void);
extern void view_service_reminders_init(void);
extern void view_service_stop_watch_init(void);
void view_service_init(void)
{
	const struct device *rtc = device_get_binding("apollo3p_rtc");
	int ticks;

	if (rtc) {
		struct tm tm_now;
		tm_now.tm_year = 121; // year since 1900
		tm_now.tm_mon = 5;
		tm_now.tm_mday = 17;
		tm_now.tm_hour = 10;
		tm_now.tm_min = 0;
		tm_now.tm_sec = 0;
		time_t ts = timeutil_timegm(&tm_now);

		ticks = counter_us_to_ticks(rtc, ts * USEC_PER_SEC);
		counter_set_value(rtc, ticks);
	}

	// sync rtc ticks to system time
	if (rtc) {
		counter_get_value(rtc, &ticks);
		uint64_t us_cnt = counter_ticks_to_us(rtc, ticks);

		// convert tick to timespec,then set to posix real time
		struct timespec time;
		time.tv_sec = us_cnt / 1000000UL;
		time.tv_nsec = 0;
		clock_settime(CLOCK_REALTIME, &time);
	}

	for (uint8_t i = 0; i < VIEW_EVENT_TYPE_MAX; i++) {
		handles[i] = NULL;
	}
	sys_slist_init(&view_service.free);
	sys_slist_init(&view_service.pending);
	k_sem_init(&view_service.wait, 0, 1);

	for (uint8_t i = 0; i < MAX_QUEUE_EVENTS; i++) {
		sys_slist_append(&view_service.free, &view_service.events[i].node);
	}

	k_thread_create(&thread_view_service, thread_view_service_stack, STACKSIZE, (k_thread_entry_t)view_service_thread,
					NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread_view_service, "view service");
	view_service_display_init();
	view_service_night_mode_init();
	view_service_misc_init();
	view_service_alarm_init();
	view_service_reminders_init();
	view_service_stop_watch_init();
}

int view_service_callback_reg(VIEW_EVENT_TYPE type, view_event_handle handle)
{
	if (type < VIEW_EVENT_TYPE_MAX) {
		if (handles[type] == NULL) {
			handles[type] = handle;
			return 0;
		}
		LOG_ERR("handle already registed!");
	} else {
		LOG_ERR("unknown event type!");
	}
	return -1;
}

void view_service_event_submit(VIEW_EVENT_TYPE type, void *data, int data_size)
{
	void *p = NULL;
	if (data_size != 0) {
		p = k_heap_alloc(&view_service_heap, data_size, K_NO_WAIT);
		if (p != NULL) {
			memcpy(p, data, data_size);
		} else {
			LOG_ERR("[%s] alloc fail!", __FUNCTION__);
			return;
		}
	}

	VIEW_EVENT event;
	event.view_event_type = type;
	event.data = p;
	view_generic_event_fold(&event);
}