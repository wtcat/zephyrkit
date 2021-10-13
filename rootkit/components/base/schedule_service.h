#ifndef SCHEDULE_SERVICE_H_
#define SCHEDULE_SERVICE_H_

#include <sys/types.h>
#include <zephyr.h>

#include "base/list.h"

#ifdef __cplusplus
extern "C"{
#endif

struct device;
struct schedule_service;

/*
 * Open flags
 */
#define O_SVR_DEVICE (0x01)  /* Device service */


enum service_state {
    SERVICE_STOPED       = 0,
    SERVICE_READY        = 1,
    SERVICE_RUNING       = 2
};

enum servicework_state {
    SERVICE_WORK_IDLE      = 0,
    SERVICE_WORK_PENDING   = 1,
    SERVICE_WORK_STOPPING  = 2
};

struct schedule_operations {
	int (*init)(struct schedule_service *ss);
    int (*exit)(struct schedule_service *ss);
	int (*exec)(struct schedule_service *ss);
	ssize_t (*read)(struct schedule_service *ss, void *buffer, size_t n);
};

struct schedule_service {
    struct k_work work;
    struct k_timer timer;
    struct k_mutex lock;
    struct k_spinlock spin;
    struct list_head node;
    enum service_state state;
    enum servicework_state wkstate;
    struct k_sem *exit;
    struct k_work_q *workqueue;
	const struct device *device;
    const char *name;
    const struct schedule_operations *ops;
    uint32_t period;
	bool update;

    void *private; /* user private data */
};


/*
 * SCHED_SERVICE(_svr, _name, _ops, _private)
 * 
 * @_svr: name
 * @_name: match name
 * @_ops: service operations
 * @_private: private data
 */ 
#define SCHED_SERVICE(_svr, _name, _ops, _private) \
    static int _svr##_sched__service_init(const struct device *dev) { \
        static struct schedule_service __sched_svr; \
        ARG_UNUSED(dev); \
        __sched_svr.private = _private; \
        return schedule_service_register(&__sched_svr, _ops, _name); \
    } \
    SYS_INIT(_svr##_sched__service_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT)

/*
 * Read data from service
 * 
 * @ss: service handle
 * @buffer: user data buffer
 * @n: data items(not size)
 * @return: data size or error code
 */
static inline ssize_t schedule_service_read(struct schedule_service *ss, 
    void *buffer, size_t nitems)
{
    return ss->ops->read(ss, buffer, nitems);
}

/*
 * Open service
 * 
 * @name: service name
 * flags: open flags
 * @return: NULL is failed
 */
struct schedule_service *schedule_service_open(const char *name, int flags);

/*
 * Close service 
 */
void schedule_service_close(struct schedule_service *ss);

/*
 * Close all services of opened
 */
void schedule_services_close(void);

/*
 * Set run-queue for a service
 * 
 * @ss: service handle
 * @wq: work queue
 * @return: 0 is success
 */
int schedule_service_set_workqueue(struct schedule_service *ss, 
    struct k_work_q *wq);

/*
 * Run service
 * 
 * @ss: service handle
 * @period: execute period(unit: ms)
 * @return: 0 is success
 */ 
int schedule_service_run(struct schedule_service *ss, 
    unsigned int period);

 /*
  * Pause a service that running now 
  * 
  * @ss: service handle
  */   
void schedule_service_pause(struct schedule_service *ss);

 /*
  * Change execute period of service
  * 
  * @ss: service handle
  * @period: execute period(unit: ms)
  * @return: 0 is success
  */ 
int schedule_service_change_period(struct schedule_service *ss, 
    unsigned int period);

/*
 * Register a new service
 * 
 * @ss: service handle
 * @ops: the method of service class
 * @name: service name
 * return: 0 is success
 */
int schedule_service_register(struct schedule_service *ss, 
    const struct schedule_operations *ops, const char *name);

/*
 * Service iterator
 */
void schedule_service_iterate(
    void (*iterator)(struct schedule_service *, void *), 
    void *arg);

#ifdef __cplusplus
}
#endif
#endif //SCHEDULE_SERVICE_H_
