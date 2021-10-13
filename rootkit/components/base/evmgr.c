#include <string.h>

#include <zephyr.h>
#include <init.h>
#include <sys/atomic.h>
#include <sys/__assert.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(evmgr);

#include "base/evmgr.h"

#define EV_BUFF_MAX     10
#define EV_THREAD_PRIO  5
#define EV_STACK_SZ     2048


/*
 * Event status 
 */
#define EV_STATE_IDLE       0x01
#define EV_STATE_PENDING    0x02
#define EV_STATE_SCHEDULED  0x04

#define EV_PRIO_MAX  32
#define EV_MAX 48

struct ev_node {
    struct ev_struct event;
    struct list_head node;
    atomic_t state;
    atomic_t refcnt;
};

struct ev_notifier_struct {
    struct k_sem lock;
    struct ev_notifier *nodes;
};

struct ev_manager {
    struct k_thread daemon;
    struct k_sem sync;
    struct k_sem lock;
    struct k_mutex mutex;
    atomic_t bitmap;
    atomic_t bitmsk;
    struct k_spinlock spin;
    struct list_head ready[EV_PRIO_MAX];
    struct list_head free;
    struct ev_notifier_struct notifier;
    ATOMIC_DEFINE(evtype, EV_MAX);
    struct ev_node evbuff[EV_BUFF_MAX];
};

#define ev_lock(_lock) \
    k_sem_take(_lock, K_FOREVER);
#define ev_unlock(_lock) \
    k_sem_give(_lock);

static struct ev_node *current_event;
static K_THREAD_STACK_DEFINE(ev_stack, EV_STACK_SZ);
static struct ev_manager ev_mgr = {
    .sync = Z_SEM_INITIALIZER(ev_mgr.sync, 0, 1),
    .lock = Z_SEM_INITIALIZER(ev_mgr.lock, 1, 1),
    .mutex = Z_MUTEX_INITIALIZER(ev_mgr.mutex),
};


static void ev_priority_map(struct ev_manager *mgr, 
    struct ev_node *ev, uint16_t prio)
{
    list_add_tail(&ev->node, &mgr->ready[prio]);
    atomic_set_bit(&mgr->bitmap, prio);
}

static void ev_priority_unmap(struct ev_manager *mgr, 
    struct ev_node *ev, uint16_t prio)
{
    list_del(&ev->node);
    if (list_empty(&mgr->ready[prio]))
        atomic_clear_bit(&mgr->bitmap, prio);
}

static bool __ev_free(struct ev_manager *mgr, 
    struct ev_node *ev)
{
    if (atomic_cas(&ev->state, EV_STATE_SCHEDULED, 
        EV_STATE_IDLE)) {
        uint16_t prio = EV_PRIO(ev->event.type);
        k_mutex_lock(&mgr->mutex, K_FOREVER);
        ev_priority_unmap(mgr, ev, prio);
        list_add(&ev->node, &mgr->free);
        k_mutex_unlock(&mgr->mutex);
        k_sem_give(&mgr->sync);
        return true;
    }
    return false;
}

static struct ev_node *ev_highest_priority_node_get(
    struct ev_manager *mgr, 
    struct list_head *ready, 
    atomic_t *bitmap)                        
{
    struct list_head *rd;
    int prio;

    prio = __builtin_ctz(atomic_get(bitmap));
    rd = &ready[prio];
    __ASSERT(!list_empty_careful(rd), "");
    return CONTAINER_OF(rd->next, struct ev_node, node);
}

static void ev_notify(struct ev_manager *mgr, 
    struct ev_node *ev)
{
    ev_lock(&mgr->notifier.lock);
    for (struct ev_notifier *ptr = mgr->notifier.nodes; 
        ptr != NULL; ptr = ptr->next) {
        if (atomic_cas(&ev->state, EV_STATE_PENDING, 
            EV_STATE_SCHEDULED))
            ptr->notify(&ev->event);
    }
    if (!atomic_get(&ev->refcnt))
        __ev_free(mgr, ev);
    ev_unlock(&mgr->notifier.lock);
}

static void ev_daemon_thread(void *arg)
{
    struct ev_manager *mgr = arg;

    for (;;) {
        k_sem_take(&mgr->sync, K_FOREVER);
        while (atomic_get(&mgr->bitmap)) {
            struct ev_node *ev = ev_highest_priority_node_get(mgr, 
                mgr->ready, &mgr->bitmap);
            current_event = ev;
            if (atomic_get(&ev->state) == EV_STATE_SCHEDULED)
                break;
            ev_notify(mgr, ev);
            k_yield();
        }
    }
}

static atomic_val_t ev_priority_mask_set(atomic_t *target, 
    atomic_val_t clr_bitmask, atomic_val_t set_bitmask)
{
    atomic_val_t prio_mask = atomic_get(target);
    atomic_val_t temp;
    
    temp = (prio_mask & ~clr_bitmask) | set_bitmask;
    atomic_set(target, temp);
    return prio_mask;
}

static inline void ev_notity_owner(struct ev_struct *e)
{
    if (e->owner && e->fn)
        e->fn(e->owner, e);
}

int ev_disable_level_set(int prio_level)
{
    atomic_val_t old = atomic_get(&ev_mgr.bitmsk);

    __ASSERT(prio_level <= EV_PRIO_MAX, "");
    atomic_val_t set = (atomic_val_t)((0x1ull << prio_level) - 1);
    ev_priority_mask_set(&ev_mgr.bitmsk, ~set, set);
    return __builtin_ctz(old);
}

void ev_flush(void)
{
    struct ev_manager *mgr = &ev_mgr;
    struct list_head *iter;

    ev_lock(&mgr->lock);
    for (int i = 0; i < EV_PRIO_MAX; i++) {
        struct list_head *ready = &mgr->ready[i];
        if (list_empty(ready))
            continue;
        list_splice_init(ready, &mgr->free);
    }
    atomic_set(&mgr->bitmap, 0);
    list_for_each(iter, &mgr->free) {
        struct ev_node *ev = CONTAINER_OF(iter, 
            struct ev_node, node);
        atomic_set(&ev->state, EV_STATE_IDLE);
    }
    ev_unlock(&mgr->lock);
}

struct ev_struct *ev_get(struct ev_struct *e)
{
    struct ev_node *ev = CONTAINER_OF(e, 
        struct ev_node, event);
    atomic_inc(&ev->refcnt);
    LOG_DBG("EV_GET(%d): Refcnt(%d)\n", EV_ID(e->type), 
        atomic_get(&ev->refcnt));
    return e;
}

void ev_put(struct ev_struct *e)
{
    struct ev_node *ev = CONTAINER_OF(e, 
        struct ev_node, event);
    if (atomic_dec(&ev->refcnt) == 1) {
        __ev_free(&ev_mgr, ev);
        LOG_DBG("EV_PUT(%d): Refcnt(%d)\n", EV_ID(e->type), 
            atomic_get(&ev->refcnt));
        e->type = UINT_MAX;
    }
}

void ev_enable(uint16_t type)
{
    __ASSERT(type < EV_MAX, "");
    atomic_set_bit(ev_mgr.evtype, type);
}

void ev_disable(uint16_t type)
{
    __ASSERT(type < EV_MAX, "");
    atomic_clear_bit(ev_mgr.evtype, type);
}

int ev_notifier_register(struct ev_notifier *node)
{
    struct ev_notifier_struct *p;
    struct ev_notifier **iter;

    if (!node)
        return -EINVAL;
    if (!node->notify)
        return -EINVAL;

    p = &ev_mgr.notifier;
    iter = &p->nodes;
    ev_lock(&p->lock);
    while (*iter)
        iter = &((*iter)->next);
    *iter = node;
    node->next = NULL;
    ev_unlock(&p->lock);
    return 0;
}

int ev_submit(uint32_t event, uintptr_t data, 
    size_t size)
{
    struct ev_manager *mgr = &ev_mgr;
    struct ev_node *ev;
    struct list_head *node;

    __ASSERT(EV_PRIO(event) < EV_PRIO_MAX, "");
    __ASSERT(EV_ID(event) < EV_MAX, "");
    if (!atomic_test_bit(mgr->evtype, EV_ID(event)))
        return -ENOTSUP;
    if (!atomic_test_bit(&mgr->bitmsk, EV_PRIO(event)))
        return -EINVAL;

    if (current_event->event.type == event) {
        current_event->event.data = data;
        current_event->event.size = size;
        ev_notity_owner(&current_event->event);
        return 0;
    }
    ev_lock(&mgr->lock);
    if (list_empty(&mgr->free)) {
        ev_unlock(&mgr->lock);
        return -ENOMEM;
    }
    node = mgr->free.next;
    list_del(node);
    ev = CONTAINER_OF(node, struct ev_node, node);
    ev->event.owner = NULL;
    ev->event.fn = NULL;
    ev->event.type = event;
    ev->event.data = data;
    ev->event.size = size;
    __ASSERT(atomic_get(&ev->state) == EV_STATE_IDLE, "");
    atomic_set(&ev->state, EV_STATE_PENDING);
    atomic_set(&ev->refcnt, 0);
    ev_priority_map(mgr, ev, EV_PRIO(event));
    ev_unlock(&mgr->lock);
    k_sem_give(&mgr->sync);
    return 0;
}

void ev_sync(void)
{
    //k_sem_give(&ev_mgr.sync);
}

static int ev_init(const struct device *dev __unused)
{
    struct ev_manager *mgr = &ev_mgr;
    k_tid_t tid;
    int i;

    INIT_LIST_HEAD(&mgr->free);
    for (i = 0; i < EV_PRIO_MAX; i++)
        INIT_LIST_HEAD(&mgr->ready[i]);
    for (i = 0; i < EV_BUFF_MAX; i++) 
        list_add_tail(&mgr->evbuff[i].node, &mgr->free);
    current_event = &mgr->evbuff[0];
    atomic_set(&mgr->bitmsk, UINT_MAX);
    atomic_set(&mgr->bitmap, 0);
    k_sem_init(&mgr->notifier.lock, 1, 1);
    memset(&mgr->evtype, 0xFF, sizeof(mgr->evtype));
    tid = k_thread_create(&mgr->daemon, ev_stack, 
        K_THREAD_STACK_SIZEOF(ev_stack), 
        (k_thread_entry_t)ev_daemon_thread, mgr, NULL, NULL, 
        EV_THREAD_PRIO, 0, K_NO_WAIT);
    k_thread_name_set(tid, "/system@events");
    return 0;
}

SYS_INIT(ev_init, POST_KERNEL, 
    CONFIG_APPLICATION_INIT_PRIORITY);

