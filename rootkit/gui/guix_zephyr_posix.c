#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <posix/pthread.h>

#include <kernel.h>

#include "gx_api.h"
#include "gx_widget.h"
#include "gx_system.h"
#include "gx_system_rtos_bind.h"

#include "base/observer.h"
#include "base/list.h"

#if defined(CONFIG_FPU_SHARING)
#define GUIX_THREAD_OPTIONS (K_ESSENTIAL | K_FP_REGS)
#else
#define GUIX_THREAD_OPTIONS K_ESSENTIAL
#endif

struct guix_event {
    struct list_head node;
    GX_EVENT event;
};

struct guix_struct {
    struct guix_event events[GX_MAX_QUEUE_EVENTS];
    struct list_head pending;
    struct list_head free;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    struct k_sem thread_sem;
    struct k_sem timer_sem;
    k_tid_t timer;
    bool timer_running;
    bool timer_active;

    VOID (*entry)(ULONG); /* Point to GUIX thread entry */
};


static K_KERNEL_STACK_DEFINE(guix_stack, CONFIG_GUIX_THREAD_STACK_SIZE);
static struct k_thread guix_thread;

static K_KERNEL_STACK_DEFINE(guix_timer_stack, 2048);
static struct k_thread guix_timer_thread;
    
static K_MUTEX_DEFINE(guix_lock);

static struct guix_struct guix_class;


static void guix_event_queue_init(struct guix_struct *guix)
{
    INIT_LIST_HEAD(&guix->free);
    INIT_LIST_HEAD(&guix->pending);
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
static K_MEM_POOL_DEFINE(guix_mpool, 4, CONFIG_GUIX_MEMPOOL_SIZE, 1, 4);

/*
 * memory pool for guix
 */
static void *guix_memory_allocate(ULONG size)
{
    return k_mem_pool_malloc(&guix_mpool, (size_t)size);
}

static void guix_memory_free(void *ptr)
{
    k_free(ptr);
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

static void guix_timer_server(void *p1, void *p2, void *p3)
{
    struct guix_struct *gx = p1;

    if (!gx->timer_running)
        k_thread_suspend(k_current_get());
    
    gx->timer_active = true;
    
    while (true) {
        k_msleep(GX_SYSTEM_TIMER_MS);

        while (!gx->timer_active) {
            k_sem_give(&gx->thread_sem);
            k_sem_take(&gx->timer_sem, K_FOREVER);
        }
        
        _gx_system_timer_expiration(0);
    }
}

static void guix_thread_adaptor(void *p1, void *p2, void *p3)
{
    struct guix_struct *gx = (struct guix_struct *)p1;
    GX_EVENT event;
    bool wake_up ;
    UINT ret;

    while (true) {
        
        /* Process GUI event */
        gx->entry(0);

        /* GUIX thread exited and stop timer */
        //int state = gx->timer->base.thread_state;
        if (gx->timer_running) {
            wake_up = true;
            gx->timer_active = false;
            k_sem_take(&gx->thread_sem, K_FOREVER);
        } else {
            wake_up = false;
        }

        /* Notify system that gui will enter in suspend state */
        _guix_suspend_notify(GUIX_ENTER_SLEEP);
        
        /* Flush event and wait */
        do {
            ret = gx_generic_event_pop(&event, GX_FALSE);
            if (ret == GX_FAILURE) {
                ret = gx_generic_event_pop(&event, GX_TRUE);
                break;
            }
        } while (true);

        /* Refresh screen */
        event.gx_event_type = GX_EVENT_REDRAW;
        event.gx_event_target = NULL;
        gx_system_event_send(&event);

        /* Notify system that gui is ready */
        _guix_suspend_notify(GUIX_EXIT_SLEEP);
        
        /* Wake up GUIX timer */
        if (wake_up) {
            gx->timer_active = true;
            k_sem_give(&gx->timer_sem);
        }
    }
}

/* 
 * rtos initialize: perform any setup that needs to be done 
 * before the GUIX task runs here 
 */
VOID gx_generic_rtos_initialize(VOID)
{
    struct guix_struct *gx = &guix_class;

    pthread_mutex_init(&gx->mutex, NULL);
    pthread_cond_init(&gx->cond, NULL);
    k_sem_init(&gx->thread_sem, 0, 1);
    k_sem_init(&gx->timer_sem, 0, 1);
    guix_event_queue_init(gx);
    
#if (CONFIG_GUIX_MEMPOOL_SIZE > 0)
    gx_system_memory_allocator_set(guix_memory_allocate, 
        guix_memory_free);
#endif
}

/* thread_start: start the GUIX thread running. */
UINT gx_generic_thread_start(VOID(*guix_thread_entry)(ULONG))
{
    struct guix_struct *gx = &guix_class;
    k_tid_t thread;
    k_tid_t timer;

    thread = k_thread_create(&guix_thread,
                              guix_stack,
                              K_KERNEL_STACK_SIZEOF(guix_stack),
                              guix_thread_adaptor,
                              gx, NULL, NULL,
                              CONFIG_GUIX_THREAD_PRIORITY, GUIX_THREAD_OPTIONS,
                              K_FOREVER);
    
    timer = k_thread_create(&guix_timer_thread,
                             guix_timer_stack,
                             K_KERNEL_STACK_SIZEOF(guix_timer_stack),
                             guix_timer_server,
                             gx, NULL, NULL,
                             CONFIG_GUIX_THREAD_PRIORITY, K_ESSENTIAL, K_FOREVER);

    k_thread_name_set(thread, "GUIX-THREAD");
    k_thread_name_set(timer, "GUIX-TIMER");

    gx->entry = guix_thread_entry;
    gx->timer = timer;
    k_thread_start(timer);
    k_thread_start(thread);
    
    return GX_SUCCESS;
}

/* event_post: push an event into the fifo event queue */
UINT gx_generic_event_post(GX_EVENT *event_ptr)
{
    struct guix_struct *gx = &guix_class;
    struct guix_event *ge;

    pthread_mutex_lock(&gx->mutex);
    if (list_empty(&gx->free)) {
        pthread_mutex_unlock(&gx->mutex);
        return GX_FAILURE;
    }

    ge = event_get_first(&gx->free);
    ge->event = *event_ptr;
    if (list_empty(&gx->pending)) {
        list_add_tail(&ge->node, &gx->pending);
        pthread_cond_signal(&gx->cond);
    } else {
        list_add_tail(&ge->node, &gx->pending);
    }
    pthread_mutex_unlock(&gx->mutex);
    return GX_SUCCESS;
}

/* event_fold: update existing matching event, otherwise post new event */
UINT gx_generic_event_fold(GX_EVENT *event_ptr)
{
    struct guix_struct *gx = &guix_class;
    struct list_head *iter;
    struct guix_event *ge;
    GX_EVENT *e;
   
    pthread_mutex_lock(&gx->mutex);
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
            pthread_mutex_unlock(&gx->mutex);
            return GX_SUCCESS;
        }
    }

    if (list_empty(&gx->free)) {
        pthread_mutex_unlock(&gx->mutex);
        return GX_FAILURE;
    }
    
    ge = event_get_first(&gx->free);
    ge->event = *event_ptr;
    if (list_empty(&gx->pending)) {
        list_add_tail(&ge->node, &gx->pending);
        pthread_cond_signal(&gx->cond);
    } else {
        list_add_tail(&ge->node, &gx->pending);
    }

    pthread_mutex_unlock(&gx->mutex);
    return GX_SUCCESS;
}

/* event_pop: pop oldest event from fifo queue, block if wait and no events exist */
UINT gx_generic_event_pop(GX_EVENT *put_event, GX_BOOL wait)
{
    struct guix_struct *gx = &guix_class;
    struct guix_event *ge;
    
    if (!wait && list_empty_careful(&gx->pending))
       return GX_FAILURE;

    pthread_mutex_lock(&gx->mutex);
    while (list_empty(&gx->pending))
        pthread_cond_wait(&gx->cond, &gx->mutex);

    ge = (struct guix_event *)event_get_first(&gx->pending);
    *put_event = ge->event;
    list_add(&ge->node, &gx->free);
    pthread_mutex_unlock(&gx->mutex);
    return GX_SUCCESS;
}

/* event_purge: delete events targetted to particular widget */
VOID gx_generic_event_purge(GX_WIDGET *target)
{
    struct guix_struct *gx = &guix_class;
    struct list_head *iter, *next;
    struct guix_event *ge;
    
    pthread_mutex_lock(&gx->mutex);
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
    
    pthread_mutex_unlock(&gx->mutex);
}

/* start the RTOS timer */
VOID gx_generic_timer_start(VOID)
{
    struct guix_struct *gx = &guix_class;
    if (!gx->timer_running) {
        gx->timer_running = true;
        if (gx->timer)
            k_thread_resume(gx->timer);
    }
}

/* stop the RTOS timer */
VOID gx_generic_timer_stop(VOID)
{
    struct guix_struct *gx = &guix_class;
    
    if (gx->timer_running) {
        gx->timer_running = false;
        if (gx->timer)
            k_thread_suspend(gx->timer);
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

