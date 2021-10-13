#include "spinlock.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <kernel.h>
#include <errno.h>
#include <init.h>
#include "boot_event.h"

struct event_struct {
    struct k_spinlock lock;
    struct k_sem wait;
    struct boot_event event;
};

static struct event_struct _event;

/* caller need malloc put_event first!*/
int boot_event_event_pop(struct boot_event * event_rec, bool wait)
{
    if (NULL == event_rec) {
        return -ENODATA;
    }
    struct event_struct *event_internal = &_event;
    k_spinlock_key_t key;
    int ret;

    if (true != wait) {
        ret = k_sem_take(&event_internal->wait, K_NO_WAIT);
    } else {
        ret = k_sem_take(&event_internal->wait, K_FOREVER);
    }

    if (!ret) {
        key = k_spin_lock(&event_internal->lock);
        memcpy(event_rec, event_internal->event.msg, sizeof(event_internal->event));
        k_spin_unlock(&event_internal->lock, key);
    }
    return ret;
}

/* caller need malloc put_event first!*/
int boot_event_event_push(struct boot_event * event_send)
{
    if (NULL == event_send) {
        return -ENODATA;
    }

    struct event_struct *event_internal = &_event;
    k_spinlock_key_t key;

    key = k_spin_lock(&event_internal->lock);
    memcpy(event_internal->event.msg, event_send, sizeof(event_internal->event));
    k_spin_unlock(&event_internal->lock, key);

    k_sem_give(&event_internal->wait);
    return 0;
}

void send_event_to_display(uint8_t evt_type, uint8_t evt_para)
{
	struct boot_event event_push;
	event_push.msg[0] = evt_type;
	event_push.msg[1] = evt_para;
	boot_event_event_push(&event_push);
}

static int boot_event_init(const struct device *dev)
{
    ARG_UNUSED(dev);
    struct event_struct *event_internal = &_event;
    k_sem_init(&event_internal->wait, 0, 1);
    return 0;
}

SYS_INIT(boot_event_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
