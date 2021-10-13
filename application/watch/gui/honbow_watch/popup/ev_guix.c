#include <zephyr.h>
#include <init.h>

#include "base/ev_types.h"

#include "ux/ux_api.h"

#define DEFINE_WIDGET_VAR
#include "popup/widget.h"


struct ev_guix_struct {
	struct ev_struct *ev;
	GX_WIDGET *ev_target;
	UINT (*handler)(GX_WIDGET *, GX_EVENT *);
};


#define MAX_STACK_SIZE 10


static struct ev_guix_struct ev_stack[MAX_STACK_SIZE+1];
static struct ev_guix_struct *ev_current = &ev_stack[MAX_STACK_SIZE];

extern GX_WIDGET *message_pop_widget_get(void);
static UINT popup_event_wrapper(GX_WIDGET *widget, GX_EVENT *event_ptr);


static int guix_event_handler_push(GX_WIDGET *widget, struct ev_struct *e)
{
    unsigned int key;
    
    key = irq_lock();
    if (ev_current > ev_stack) {
        ev_current--;
        ev_current->handler = widget->gx_widget_event_process_function;
		ev_current->ev = e;
		ev_current->ev_target = widget;
        widget->gx_widget_event_process_function = popup_event_wrapper;
        irq_unlock(key);
		ev_get(e);
        return 0;
    }
    irq_unlock(key);
    return -EBUSY;
}

static void guix_event_handler_pop(GX_WIDGET *widget)
{
	struct ev_guix_struct *ev = ev_current;
	if (ev->ev_target) {
		unsigned int key = irq_lock();
		widget->gx_widget_event_process_function = ev->handler;
		ev_current++;
		irq_unlock(key);
		ev_put(ev->ev);
	}
}

static UINT guix_event_send(GX_WIDGET *target, 
    struct ev_struct *e)
{
    GX_EVENT ev;

    ev.gx_event_type = USER_EVENT(EV_ID(e->type));
    ev.gx_event_sender = 0;
    ev.gx_event_target = target;
    ev.gx_event_payload.gx_event_ulongdata = (ULONG)e;
    return _gx_system_event_send(&ev);
}

static void _guix_ev_send(void *owner, struct ev_struct *e)
{
    GX_WIDGET *widget = owner;
	
    if (widget->gx_widget_status & GX_STATUS_VISIBLE) 
        guix_event_send(widget, e);
}

static void guix_ev_owner_set(GX_WIDGET *widget, 
    struct ev_struct *e)
{
    e->fn = _guix_ev_send;
    e->owner = widget;
}

static void guix_ev_process(struct ev_struct *e)
{
	GX_WINDOW_ROOT *root = ux_root_window_get(0, 0);
	GX_WIDGET *widget = root->gx_widget_first_child;
	GX_EVENT ev;

	ev.gx_event_type = UX_EVENT_STOP;
	ev.gx_event_target = widget;
	_gx_system_event_send(&ev);
    if (!guix_event_handler_push(widget, e))
        guix_event_send(widget, e);
}

static UINT popup_event_wrapper(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	struct ev_guix_struct *curr = ev_current;
	GX_WIDGET *target;
    struct ev_struct *e;
	static int calldep = 0;
    UINT ret;

	calldep++;
    if (event_ptr->gx_event_type >= USER_EVENT(0)) {
		GX_ANIMATION_INFO info;

		e = (struct ev_struct *)event_ptr->gx_event_payload.gx_event_ulongdata;
    	switch (event_ptr->gx_event_type) {
    	case USER_EVENT(EV_MESSAGE):
			target = message_pop_widget_get();
    		break;
    	case USER_EVENT(EV_OTA):
			target = ota_widget_get();
    		break;
    	case USER_EVENT(EV_LOW_POWER):
			target = lowbat_widget_get(e->data);
    		break;
    	case USER_EVENT(EV_CHARGING):
			target = charging_widget_get((int)e->data);
    		break;
    	case USER_EVENT(EV_GOAL1_COMPLETED):
			target = goal_arrived_widget_get(GOAL_1, e->data);
    		break;
    	case USER_EVENT(EV_GOAL2_COMPLETED):
			target = goal_arrived_widget_get(GOAL_2, e->data);
    		break;
    	case USER_EVENT(EV_GOAL3_COMPLETED):
			target = goal_arrived_widget_get(GOAL_3, e->data);
    		break;
    	case USER_EVENT(EV_GOAL4_COMPLETED):
			target = goal_arrived_widget_get(GOAL_4, e->data);
    		break;
    	case USER_EVENT(EV_SEDENTARINESS):
			target = sedentary_widget_get(e->data);
    		break;
    	case USER_EVENT(EV_HEARTRATE_ALERT):
			target = hrov_widget_get(e->data);
    		break;
    	case USER_EVENT(EV_ALARM_ALERT):
			target = alarm_remind_widget_get(7, 0, GX_TRUE);
    		break;
    	case USER_EVENT(EV_SHUTDOWN_ANI):
			target = shutdown_ani_widget_get();
    		break;
    	case USER_EVENT(EV_SPORTING):
			target = sport_widget_get();
    		break;
    	case USER_EVENT(EV_RINGING):
			target = ringing_widget_get();
            break;
    	default:
            ret = curr->handler(widget, event_ptr);
			goto _out;
    	}
        guix_ev_owner_set(target, e);
        move_animation_setup(&info, 1, GX_ANIMATION_ELASTIC_EASE_IN_OUT,
               _ux_dir_down, 10);
		pop_widget_fire(widget, target, &info);
		ret = curr->handler(widget, event_ptr);
		goto _out;
    }
	ret = curr->handler(widget, event_ptr);
	if (event_ptr->gx_event_type == GX_EVENT_ANIMATION_COMPLETE) {
		if (calldep == 1 && 
			(curr->ev_target == widget) &&
			(widget->gx_widget_status & GX_STATUS_VISIBLE)) {
			guix_event_handler_pop(widget);
		}
	}
_out:	
	calldep--;
    return ret;
}

static int gux_evobs_action(struct observer_base *nb,
	unsigned long action, void *data)
{
	static struct ev_notifier ev_guix_notifier = {
		.notify = guix_ev_process,
	};
	ev_notifier_register(&ev_guix_notifier);
	return 0;
}

static int guix_ev_init(const struct device *dev __unused)
{
	static struct observer_base ev_guix_obs = 
		OBSERVER_STATIC_INIT(gux_evobs_action, 0);
	return guix_state_add_observer(&ev_guix_obs);
}
SYS_INIT(guix_ev_init,
	PRE_KERNEL_2, 90);
