#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <version.h>
#include <kernel.h>
#include <app_memory/app_memdomain.h>
#include <app_memory/mem_domain.h>
 
#include "gx_api.h"
#include "gx_widget.h"
#include "gx_system.h"
#include "gx_system_rtos_bind.h"

#include "base/observer.h"
#include "base/list.h"

#ifdef CONFIG_GUIX_USER_MODE
#define GUI_OPTIONS K_USER
#else
#define GUI_OPTIONS K_ESSENTIAL 
#endif

#if defined(CONFIG_FPU_SHARING)
#define GUIX_THREAD_OPTIONS (GUI_OPTIONS | K_FP_REGS)
#else
#define GUIX_THREAD_OPTIONS GUI_OPTIONS
#endif


struct guix_event {
    struct list_head node;
    GX_EVENT event;
};

struct guix_struct {
    struct guix_event events[GX_MAX_QUEUE_EVENTS];
    struct list_head pending;
    struct list_head free;
    struct k_timer timer;
    struct k_spinlock lock;
    struct k_sem wait;
    struct k_sem thread_sem;
    struct k_sem timer_sem;
    bool timer_running;
    bool timer_active;

    VOID (*entry)(ULONG); /* Point to GUIX thread entry */
    bool (*filter)(GX_EVENT *e); /* Filter GUIX event */
};


#ifdef CONFIG_GUIX_USER_MODE
K_APPMEM_PARTITION_DEFINE(guix_partition);
static struct k_mem_domain guix_mem_domain;
#endif

static K_THREAD_STACK_DEFINE(guix_stack, CONFIG_GUIX_THREAD_STACK_SIZE);
static struct k_thread guix_thread;
static K_MUTEX_DEFINE(guix_lock);


static void guix_event_queue_init(struct guix_struct *guix)
{
    for (int i = 0; i < GX_MAX_QUEUE_EVENTS; i++) 
        list_add_tail(&guix->events[i].node, &guix->free);    
}

static inline struct guix_event *event_get_first(struct list_head *head)
{
    struct list_head *node = head->next;
    list_del(node);
    return (struct guix_event *)node;
}

#if (CONFIG_GUIX_MEMPOOL_SIZE > 0)
static K_HEAP_DEFINE(guix_mpool, CONFIG_GUIX_MEMPOOL_SIZE);
static void *guix_memory_allocate(ULONG size)
{
	return k_heap_alloc(&guix_mpool, size, K_NO_WAIT);
}

static void guix_memory_free(void *ptr)
{
	k_heap_free(&guix_mpool, ptr);
}
#endif /* CONFIG_GUIX_MEMPOOL_SIZE > 0 */

/* 
 * GUIX notify interface
 */
static K_MUTEX_DEFINE(guix_notifier_lock);
static struct observer_base *guix_suspend_notifier_list;

int guix_suspend_notify_register(struct observer_base *observer)
{
    int ret;
    k_mutex_lock(&guix_notifier_lock, K_FOREVER);
    ret = observer_cond_register(&guix_suspend_notifier_list,
            observer);
    k_mutex_unlock(&guix_notifier_lock);
    return ret;
}

int guix_suspend_notify_unregister(struct observer_base *observer)
{
    int ret;
    k_mutex_lock(&guix_notifier_lock, K_FOREVER);
    ret = observer_unregister(&guix_suspend_notifier_list,
            observer);
    k_mutex_unlock(&guix_notifier_lock);
    return ret;
}

static int _guix_suspend_notify(unsigned int state)
{
    int ret;
    k_mutex_lock(&guix_notifier_lock, K_FOREVER);
    ret = observer_notify(&guix_suspend_notifier_list, state, NULL);
    k_mutex_unlock(&guix_notifier_lock);
    return ret;
}

static void guix_timer_entry(struct k_timer *timer)
{
    (void) timer;
    _gx_system_timer_expiration(0);
}

static bool guix_sleep(struct guix_struct *gx, bool nowait)
{
    bool wake_up;

    /* GUIX thread exited and stop timer */
    if (gx->timer_running) {
        wake_up = true;
        gx->timer_active = false;
        k_timer_stop(&gx->timer);
    } else {
        wake_up = false;
    }
    /* Notify system that gui will enter in suspend state */
    _guix_suspend_notify(GUIX_ENTER_SLEEP);
    
    /* Flush event and wait */
    for (;;) {
        GX_EVENT event;
        UINT ret = gx_generic_event_pop(&event, GX_FALSE);
        if (ret == GX_SUCCESS)
            continue;
        if (nowait)
            break;
        ret = gx_generic_event_pop(&event, GX_TRUE);
        if (ret == GX_SUCCESS) {
            if (gx->filter(&event))
                break;
        }
    }
    return wake_up;
}

static void guix_wake_up(struct guix_struct *gx, bool active_timer)
{
    /* Notify system that gui is ready */
    _guix_suspend_notify(GUIX_EXIT_SLEEP);
    
    /* Wake up GUIX timer */
    if (active_timer) {
        gx->timer_active = true;
        k_timer_start(&gx->timer, K_MSEC(GX_SYSTEM_TIMER_MS), 
            K_MSEC(GX_SYSTEM_TIMER_MS));
    }
}

static bool guix_default_event_filter(GX_EVENT *e)
{
    (void) e;
    return true;
}

static void guix_thread_adaptor(void *arg)
{
    struct guix_struct *gx = (struct guix_struct *)arg;

    guix_wake_up(gx, false);
    for (;;) {
        /* Process GUI event */
        gx->entry(0);
        guix_wake_up(gx, guix_sleep(gx, false));
    }
}

static struct guix_struct guix_class = {
    .pending = LIST_HEAD_INIT(guix_class.pending),
    .free = LIST_HEAD_INIT(guix_class.free),
    .filter = guix_default_event_filter
};

void guix_event_filter_set(bool (*filter)(GX_EVENT *))
{
    if (filter)
        guix_class.filter = filter;
}

/* 
 * RTOS initialize: perform any setup that needs to be done 
 * before the GUIX task runs here 
 */
VOID gx_generic_rtos_initialize(VOID)
{
    struct guix_struct *gx = &guix_class;

    k_timer_init(&gx->timer, guix_timer_entry, NULL);
    k_sem_init(&gx->wait, 0, 1);
    k_sem_init(&gx->thread_sem, 0, 1);
    k_sem_init(&gx->timer_sem, 0, 1);
    guix_event_queue_init(gx);
#if (CONFIG_GUIX_MEMPOOL_SIZE > 0)
    gx_system_memory_allocator_set(guix_memory_allocate, 
        guix_memory_free);
#endif
}

#if (KERNEL_VERSION_NUMBER < ZEPHYR_VERSION(2,6,0))
static void gx_thread_abort(struct k_thread *aborted)
{
    struct guix_struct *gx = &guix_class;
    GX_TIMER **timer;

    guix_sleep(gx, true);
    /* Free all timer */
    timer = &_gx_system_free_timer_list;
    while (*timer)
        timer = &((*timer)->gx_timer_next);
    *timer = _gx_system_active_timer_list;
    _gx_system_active_timer_list = NULL;
}
#endif

/* thread_start: start the GUIX thread running. */
UINT gx_generic_thread_start(VOID(*guix_thread_entry)(ULONG))
{
    struct guix_struct *gx = &guix_class;
    k_tid_t thread;

    thread = k_thread_create(&guix_thread,
                              guix_stack,
                              K_KERNEL_STACK_SIZEOF(guix_stack),
                              (k_thread_entry_t)guix_thread_adaptor,
                              gx, NULL, NULL,
                              CONFIG_GUIX_THREAD_PRIORITY, GUIX_THREAD_OPTIONS,
                              K_FOREVER);
#ifdef CONFIG_GUIX_USER_MODE
    struct k_mem_partition *parts[] = {&guix_partition};
    k_mem_domain_init(&guix_mem_domain, 1, parts);
    k_mem_domain_add_thread(&guix_mem_domain, thread);
#endif
    k_thread_name_set(thread, "/gui@main");
#if (KERNEL_VERSION_NUMBER < ZEPHYR_VERSION(2,6,0))
    thread->fn_abort = gx_thread_abort;
#endif
    gx->entry = guix_thread_entry;
    k_thread_start(thread);
    return GX_SUCCESS;
}

/* event_post: push an event into the fifo event queue */
UINT gx_generic_event_post(GX_EVENT *event_ptr)
{
    struct guix_struct *gx = &guix_class;
    k_spinlock_key_t key;
    struct guix_event *ge;
    
    key = k_spin_lock(&gx->lock);
    if (list_empty(&gx->free)) {
        k_spin_unlock(&gx->lock, key);
        return GX_FAILURE;
    }
    ge = event_get_first(&gx->free);
    ge->event = *event_ptr;
    if (list_empty(&gx->pending)) {
        list_add_tail(&ge->node, &gx->pending);
        k_spin_unlock(&gx->lock, key);
        k_sem_give(&gx->wait);
    } else {
        list_add_tail(&ge->node, &gx->pending);
        k_spin_unlock(&gx->lock, key);
    }
    return GX_SUCCESS;
}

/* event_fold: update existing matching event, otherwise post new event */
UINT gx_generic_event_fold(GX_EVENT *event_ptr)
{
    struct guix_struct *gx = &guix_class;
    struct list_head *iter;
    struct guix_event *ge;
    k_spinlock_key_t key;
    GX_EVENT *e;
   
    key = k_spin_lock(&gx->lock);
    list_for_each(iter, &gx->pending) {
        ge = (struct guix_event *)iter;
        e = &ge->event;
    
        if (e->gx_event_type == event_ptr->gx_event_type &&
            e->gx_event_target == event_ptr->gx_event_target) {

            /* for timer event, update tick count */
            if (e->gx_event_type == GX_EVENT_TIMER) {
                e->gx_event_payload.gx_event_ulongdata++;
            } else {
                e->gx_event_payload.gx_event_ulongdata =
                    event_ptr->gx_event_payload.gx_event_ulongdata;
            }
            if (e->gx_event_type == GX_EVENT_PEN_DRAG)
                 _gx_system_pen_speed_update(&e->gx_event_payload.gx_event_pointdata);
            k_spin_unlock(&gx->lock, key);
            return GX_SUCCESS;
        }
    }

    if (list_empty(&gx->free)) {
        k_spin_unlock(&gx->lock, key);
        return GX_FAILURE;
    }
    ge = event_get_first(&gx->free);
    ge->event = *event_ptr;
    if (list_empty(&gx->pending)) {
        list_add_tail(&ge->node, &gx->pending);
        k_spin_unlock(&gx->lock, key);
        k_sem_give(&gx->wait);
    } else {
        list_add_tail(&ge->node, &gx->pending);
        k_spin_unlock(&gx->lock, key);
    }
    return GX_SUCCESS;
}

/* event_pop: pop oldest event from fifo queue, block if wait and no events exist */
UINT gx_generic_event_pop(GX_EVENT *put_event, GX_BOOL wait)
{
    struct guix_struct *gx = &guix_class;
    struct guix_event *ge;
    k_spinlock_key_t key;
    
    if (!wait && list_empty_careful(&gx->pending))
       return GX_FAILURE;
    key = k_spin_lock(&gx->lock);
    while (list_empty(&gx->pending)) {
        k_spin_unlock(&gx->lock, key);
        k_sem_take(&gx->wait, K_FOREVER);
        key = k_spin_lock(&gx->lock);
    }
    ge = (struct guix_event *)event_get_first(&gx->pending);
    *put_event = ge->event;
    list_add(&ge->node, &gx->free);
    k_spin_unlock(&gx->lock, key);
    return GX_SUCCESS;
}

/* event_purge: delete events targetted to particular widget */
VOID gx_generic_event_purge(GX_WIDGET *target)
{
    struct guix_struct *gx = &guix_class;
    struct list_head *iter, *next;
    struct guix_event *ge;
    k_spinlock_key_t key;
    
    key = k_spin_lock(&gx->lock);
    list_for_each_safe(iter, next, &gx->pending) {
        GX_BOOL purge = GX_FALSE;
        ge = (struct guix_event *)iter;
        if (ge->event.gx_event_target) {
            if (ge->event.gx_event_target == target) {
                purge = GX_TRUE;
            } else {
                gx_widget_child_detect(target, ge->event.gx_event_target,
                    &purge);
            }
            if (purge == GX_TRUE) {
                list_del(iter);
                list_add(iter, &gx->free);
            }
        }
    }
    k_spin_unlock(&gx->lock, key);
}

/* start the RTOS timer */
VOID gx_generic_timer_start(VOID)
{
    struct guix_struct *gx = &guix_class;
    if (!gx->timer_running) {
        gx->timer_running = true;
        k_timer_start(&gx->timer, K_MSEC(GX_SYSTEM_TIMER_MS), 
            K_MSEC(GX_SYSTEM_TIMER_MS));
    }
}

/* stop the RTOS timer */
VOID gx_generic_timer_stop(VOID)
{
    struct guix_struct *gx = &guix_class;
    
    if (gx->timer_running) {
        gx->timer_running = false;
        k_timer_stop(&gx->timer);
    }
}

/* lock the system protection mutex */
VOID gx_generic_system_mutex_lock(VOID)
{
    k_mutex_lock(&guix_lock, K_FOREVER);
}

/* unlock system protection mutex */
VOID gx_generic_system_mutex_unlock(VOID)
{
    k_mutex_unlock(&guix_lock);
}

/* return number of low-level system timer ticks. Used for pen speed caculations */
ULONG gx_generic_system_time_get(VOID)
{
    return (ULONG)k_uptime_ticks();
}

/* thread_identify: return current thread identifier, cast as VOID * */
VOID *gx_generic_thread_identify(VOID)
{
    return (VOID *)k_current_get();
}

VOID gx_generic_time_delay(int ticks)
{
    k_timeout_t t = {ticks};
    k_sleep(t);
}
