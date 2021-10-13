#ifndef EVENT_MANAGER_H_
#define EVENT_MANAGER_H_

#include <sys/util.h>
#include "base/list.h"
#include "base/ev_types.h"

#ifdef __cplusplus
extern "C"{
#endif 

struct ev_struct {
    void *owner;
    void (*fn)(void *, struct ev_struct *);
    uint32_t type;
    size_t size;
    uintptr_t data;
};

struct ev_notifier {
    struct ev_notifier *next;
    void (*notify)(struct ev_struct *e);
};

#define EV_ID(ev)             ((ev) & 0xFFFF)
#define EV_PRIO(ev)           (((ev) >> 16) & 0xF)
#define EV_TYPE(prio, id)     (((uint32_t)(prio) << 16) | id)


struct ev_struct *ev_get(struct ev_struct *e);
void ev_put(struct ev_struct *e);

/*
 * When the event priority is less than prio_level, 
 * the event will be disabled
 * 
 * @prio_level: Event disable priority level
 * @return: Old disable priority level
 */  
int ev_disable_level_set(int prio_level);

/*
 * Enable event
 *
 * @ev_type: Event type
 */
void ev_enable(uint16_t ev_type);

/*
 * Disable event
 *
 * @ev_type: Event type
 */
void ev_disable(uint16_t ev_type);

/*
 * Report system events
 *
 * @ev_type: Event type
 * @prio: Event priority
 * @data: User private data
 * @size: Data size
 * @return: 0 If successful
 */
int ev_submit(uint32_t event, uintptr_t data, size_t size);

/*
 * Event process ack
 *
 * @e: Event parameter object
 */
void ev_ack(struct ev_struct *e);

/*
 * Event synchronization, It must be called 
 * after event_report
 */
void ev_sync(void);

/*
 * Clear all events that pending
 */
void ev_flush(void);

/*
 * Register a event notify callback
 *
 * @notifier: Notify object
 */
int ev_notifier_register(struct ev_notifier *notifier);


#ifdef __cplusplus
}
#endif 
#endif /* EVENT_MANAGER_H_ */
