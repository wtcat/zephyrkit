#include <string.h>
#include <errno.h>

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(sched_svr);

#include "base/workqueue.h"
#include "base/schedule_service.h"


#define MAX_HZ CONFIG_SYS_CLOCK_TICKS_PER_SEC

static LIST_HEAD(svr_root);
static K_MUTEX_DEFINE(list_lock);
static WORKQUEUE_DEFINE(sched_wq, 2048, CONFIG_SYSTEM_WORKQUEUE_PRIORITY);

static struct schedule_service *find_service(const char *name)
{
    struct list_head *p;
    k_mutex_lock(&list_lock, K_FOREVER);
    list_for_each(p, &svr_root) {
        struct schedule_service *ss = CONTAINER_OF(p, 
            struct schedule_service, node);
        if (!strcmp(ss->name, name)) {
            k_mutex_unlock(&list_lock);
            return ss;
        }
    }
    k_mutex_unlock(&list_lock);
    return NULL;
}

static void timer_process(struct k_timer *timer)
{
	struct schedule_service *ss = CONTAINER_OF(timer, 
        struct schedule_service, timer);
    k_spinlock_key_t key;

	if (unlikely(ss->update)) {
        k_timeout_t period = K_MSEC(ss->period);
        if (Z_TICK_ABS(period.ticks) < 0)
            period.ticks = MAX(period.ticks - 1, 1);
        timer->period = period;
        ss->update = false;
	}
    if (unlikely(k_work_pending(&ss->work))) {
        LOG_ERR("Service(%s) refresh frequency is too high(Workqueue: %s)\n", 
            ss->name, k_thread_name_get(&ss->workqueue->thread));
        return;
    }
    __ASSERT(ss->wkstate == SERVICE_WORK_IDLE, "");
    key = k_spin_lock(&ss->spin);
    ss->wkstate = SERVICE_WORK_PENDING;
    k_spin_unlock(&ss->spin, key);
	k_work_submit_to_queue(ss->workqueue, &ss->work);
}

static void service_do_work(struct schedule_service *ss)
{
    k_spinlock_key_t key;
    int ret, state;

    __ASSERT(ss->device != NULL, "");
    __ASSERT(ss->ops->exec != NULL, "");
	ret = ss->ops->exec(ss);
    if (ret < 0) 
        LOG_ERR("Service(%s) execute error: %d\n", ss->name, ret);
    
    key = k_spin_lock(&ss->spin);
    state = ss->wkstate;
    ss->wkstate = SERVICE_WORK_IDLE;
    k_spin_unlock(&ss->spin, key);
    if (state == SERVICE_WORK_STOPPING) {
        __ASSERT(ss->exit != NULL, "");
        k_sem_give(ss->exit);
    }
}

static void service_work(struct k_work *work)
{
	struct schedule_service *ss = CONTAINER_OF(work, 
        struct schedule_service, work);
    service_do_work(ss);
}

static int __schedule_service_run(struct schedule_service *ss)
{
    if (ss->state != SERVICE_READY)
        return -EINVAL;
    k_timer_start(&ss->timer, K_MSEC(ss->period), K_MSEC(ss->period));
    ss->state = SERVICE_RUNING;
    return 0;
}

static void __schedule_service_pause(struct schedule_service *ss)
{
    if (ss->state != SERVICE_RUNING)
        return;
    k_timer_stop(&ss->timer);
    ss->state = SERVICE_READY;
}

static int __schedule_service_change_period(struct schedule_service *ss, 
    unsigned int period_ms)
{
    if (period_ms == 0)
        period_ms = (1000 + MAX_HZ - 1) / MAX_HZ;
    ss->period = period_ms;
	return 0;
}

static void service_close_wrapper(struct schedule_service *ss, void *arg)
{
    ARG_UNUSED(arg);
    schedule_service_close(ss);
}

struct schedule_service *schedule_service_open(const char *name, int flags)
{
    struct schedule_service *ss;

    if (name == NULL)
        return NULL;
    ss = find_service(name);
    if (!ss) {
        LOG_ERR("%s(): Not found service:(%s)\n", __func__, name);
        return NULL;
    }

    k_mutex_lock(&ss->lock, K_FOREVER);
    if (ss->state != SERVICE_STOPED)
        goto _exit;
    
    if (flags & O_SVR_DEVICE) {
        ss->device = device_get_binding(name);
        if (!ss->device) {
            LOG_ERR("%s(): Not found device: %s\n", __func__, name);
            ss = NULL;
            goto _exit;
        }
    }
    if (ss->ops->init) {
        if (ss->ops->init(ss) < 0) {
            LOG_ERR("%s(): Initialize device failed\n", __func__);
            ss = NULL;
            goto _exit;
        }
    }

    ss->state = SERVICE_READY;
    ss->workqueue = &sched_wq;
    k_timer_init(&ss->timer, timer_process, NULL);
	k_work_init(&ss->work, service_work);
_exit:
    k_mutex_unlock(&ss->lock);
	return ss;
}

int schedule_service_set_workqueue(struct schedule_service *ss, 
    struct k_work_q *wq)
{
    __ASSERT(ss != NULL, "");
    __ASSERT(wq != NULL, "");
    k_mutex_lock(&ss->lock, K_FOREVER);
    enum service_state state = ss->state;
    if (state == SERVICE_RUNING) {
        __schedule_service_pause(ss);
        ss->workqueue = wq;
        __schedule_service_run(ss);
    } else {
        ss->workqueue = wq;
    }
    k_mutex_unlock(&ss->lock);
    return 0;
}

int schedule_service_run(struct schedule_service *ss, 
    unsigned int period_ms)
{
    __ASSERT(ss != NULL, "");
    k_mutex_lock(&ss->lock, K_FOREVER);
    LOG_INF("Starting %s service...\n", ss->name);
    __schedule_service_change_period(ss, period_ms);
    int ret = __schedule_service_run(ss);
    k_mutex_unlock(&ss->lock);
    return ret;
}

void schedule_service_pause(struct schedule_service *ss)
{
    __ASSERT(ss != NULL, "");
    k_mutex_lock(&ss->lock, K_FOREVER);
    __schedule_service_pause(ss);
    k_mutex_unlock(&ss->lock);
}

void schedule_service_close(struct schedule_service *ss)
{
    struct k_sem sem;
    k_spinlock_key_t key;

    __ASSERT(ss != NULL, "");
    k_mutex_lock(&ss->lock, K_FOREVER);
    if (ss->state == SERVICE_STOPED) {
        k_mutex_unlock(&ss->lock);
        return;
    }
    LOG_INF("Stopping %s service...\n", ss->name);
    __schedule_service_pause(ss);
    
    k_sem_init(&sem, 0, 1);
    key = k_spin_lock(&ss->spin);
    if (ss->wkstate == SERVICE_WORK_PENDING) {
        ss->wkstate = SERVICE_WORK_STOPPING;
        ss->exit = &sem;
        k_spin_unlock(&ss->spin, key);
        LOG_INF("Waiting service(%s) stop...\n", ss->name);
        k_sem_take(ss->exit, K_FOREVER);
    } else {
        k_spin_unlock(&ss->spin, key);
    }

    if (ss->ops->exit)
        ss->ops->exit(ss);  
    ss->exit = NULL;
    ss->state = SERVICE_STOPED;
    k_mutex_unlock(&ss->lock);
    LOG_INF("Service(%s) stoped now\n", ss->name);
}

void schedule_services_close(void)
{
    schedule_service_iterate(service_close_wrapper, NULL);
}

int schedule_service_change_period(struct schedule_service *ss, 
    unsigned int period_ms)
{
    __ASSERT(ss != NULL, "");
    k_mutex_lock(&ss->lock, K_FOREVER);
    LOG_INF("Service(%s) update refresh period: %d ms\n", 
        ss->name, period_ms);
    __schedule_service_change_period(ss, period_ms);
    ss->update = true;
    k_mutex_unlock(&ss->lock);
    return 0;
}

void schedule_service_iterate(
    void (*iterator)(struct schedule_service *, void *), 
    void *arg)
{
    struct list_head *p;
    k_mutex_lock(&list_lock, K_FOREVER);
    list_for_each(p, &svr_root) {
        struct schedule_service *ss = CONTAINER_OF(p, 
            struct schedule_service, node);
        iterator(ss, arg);
    }
    k_mutex_unlock(&list_lock);
}

int schedule_service_register(struct schedule_service *ss, 
    const struct schedule_operations *ops, const char *name)
{
    if (!ss)
        return -EINVAL;
    if (!ops)
        return -EINVAL;
    if (!name)
        return -EINVAL;
    
    k_mutex_init(&ss->lock);
    ss->name = name;
    ss->ops = ops;
    k_mutex_lock(&list_lock, K_FOREVER);
    list_add_tail(&ss->node, &svr_root);
    k_mutex_unlock(&list_lock);
    return 0;
}
