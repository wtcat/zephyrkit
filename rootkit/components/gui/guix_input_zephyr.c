#include "guix_input_zephyr.h"
#include "gx_system.h"

struct guix_kscan_notifier {
	struct observer_base ob;
	const struct device *touch;
};


static void touch_down_event_send(GX_EVENT *event, struct event_record *touch)
{
	event->gx_event_type = GX_EVENT_PEN_DOWN;
	event->gx_event_payload.gx_event_pointdata = touch->current;

	gx_system_event_send(event);
	touch->last = touch->current;
	touch->state = GX_TOUCH_STATE_TOUCHED;
}

static void touch_drag_event_send(GX_EVENT *event, struct event_record *touch)
{
	int x_delta = abs(touch->current.gx_point_x - touch->last.gx_point_x);
	int y_delta = abs(touch->current.gx_point_y - touch->last.gx_point_y);

	if (x_delta > touch->drag_delta || y_delta > touch->drag_delta) {
		event->gx_event_type = GX_EVENT_PEN_DRAG;
		event->gx_event_payload.gx_event_pointdata = touch->current;
		touch->last = touch->current;
		gx_system_event_fold(event);
	}
}

static void touch_up_event_send(GX_EVENT *event, struct event_record *touch)
{
	event->gx_event_type = GX_EVENT_PEN_UP;
	event->gx_event_payload.gx_event_pointdata = touch->current;
	touch->last = touch->current;
	gx_system_event_send(event);
	touch->state = GX_TOUCH_STATE_RELEASED;
}

static void touch_event_send(struct event_record *touch, GX_EVENT *event, int state)
{
	if (state == GX_TOUCH_STATE_TOUCHED) {
		if (touch->state == GX_TOUCH_STATE_TOUCHED) {
			touch_drag_event_send(event, touch);
		} else {
			touch_down_event_send(event, touch);
		}
	} else {
		touch_up_event_send(event, touch);
	}
}
static struct guix_driver *guix_driver_list = NULL;
#define FOREACH_ITEM(_iter, _head) for (_iter = (_head); _iter != NULL; _iter = _iter->next)

static void guix_input_touch_callback(const struct device *dev, uint32_t row, uint32_t column, bool pressed)
{
	GX_EVENT event = {
		.gx_event_sender = 0,
		.gx_event_target = 0,
		.gx_event_display_handle = 0,
	};

	static struct event_record touch_record = {
		.drag_delta = 8,
		.state = GX_TOUCH_STATE_RELEASED,
	};

	touch_record.current.gx_point_x = row;
	touch_record.current.gx_point_y = column;

	struct guix_driver *drv;

	FOREACH_ITEM(drv, guix_driver_list)
	{
		event.gx_event_display_handle = (ULONG)drv;
		if (pressed == true) {
			touch_event_send(&touch_record, &event, GX_TOUCH_STATE_TOUCHED);
		} else {
			touch_event_send(&touch_record, &event, GX_TOUCH_STATE_RELEASED);
		}
	}
}

static void guix_key_callback(const struct device *dev, uint32_t row, uint32_t column, bool pressed)
{
	if (row != DT_PROP(DT_NODELABEL(home_button), code)) { // only process home button.
		return;
	}

	GX_EVENT event = {
		.gx_event_sender = 0,
		.gx_event_target = 0,
		.gx_event_display_handle = 0,
	};

	struct guix_driver *drv;

	FOREACH_ITEM(drv, guix_driver_list)
	{
		GX_WIDGET *focus_owner = _gx_system_focus_owner;

		event.gx_event_display_handle = (ULONG)drv;
		if (pressed == true) {
			event.gx_event_type = GX_EVENT_KEY_DOWN;
			event.gx_event_payload.gx_event_ushortdata[0] = GX_KEY_HOME;
		} else {
			event.gx_event_type = GX_EVENT_KEY_UP;
			event.gx_event_payload.gx_event_ushortdata[0] = GX_KEY_HOME;
		}
		if (!focus_owner->gx_widget_parent)
			event.gx_event_target = focus_owner->gx_widget_first_child;
		else
			event.gx_event_target = NULL;
		gx_system_event_send(&event);
	}
}

/*
 * The button-device can not enter sleep
 */
static int guix_kscan_power_manage(struct observer_base *nb, 
	unsigned long action, void *data)
{
	struct guix_kscan_notifier *kn = CONTAINER_OF(nb, 
		struct guix_kscan_notifier, ob);
	switch (action) {
	case GUIX_ENTER_SLEEP:
		if (kn->touch)
			kscan_disable_callback(kn->touch);
		break;
	case GUIX_EXIT_SLEEP:
		if (kn->touch)
			kscan_enable_callback(kn->touch);
		break;
	}
	return NOTIFY_DONE;
}

static bool guix_event_filter(GX_EVENT *e)
{
	if (e->gx_event_type == GX_EVENT_KEY_UP)
		return e->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME;
	return false;
}

// need binding all input device and reg callbak.
void guix_zephyr_input_dev_init(struct guix_driver *list)
{
	static struct guix_kscan_notifier kn;
	const struct device *dev_touch;
	const struct device *dev_key;

	dev_touch = device_get_binding("touch_cyttsp5");
	if (dev_touch != NULL) {
		kscan_config(dev_touch, guix_input_touch_callback);
		kscan_enable_callback(dev_touch);
	}
	dev_key = device_get_binding("buttons");
	if (dev_key != NULL) {
		kscan_config(dev_key, guix_key_callback);
		kscan_enable_callback(dev_key);
	}

	// add key callback self
	guix_driver_list = list;

	/* Register touch-pannel power callback */
	kn.touch = dev_touch;
	kn.ob.update = guix_kscan_power_manage;
	guix_suspend_notify_register(&kn.ob);
	guix_event_filter_set(guix_event_filter);
}
