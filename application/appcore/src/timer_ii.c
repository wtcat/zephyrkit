/*
 * CopyRight 2022 wtcat
 */
#include <stdbool.h>
#include <errno.h>

#include "timer_ii.h"

#define TIMER_ENTRY(node_ptr) \
	(struct timer_struct *)((char *)(node_ptr) - \
		offsetof(struct timer_struct, node))

#if defined(__ZEPHYR__)
#include <spinlock.h>
#include <toolchain.h>

#define TIMER_LOCK_DECLARE k_spinlock_key_t __key;
#define TIMER_LOCK()   __key = k_spin_lock(&timer_spinlock)
#define TIMER_UNLOCK() k_spin_unlock(&timer_spinlock, __key)
static struct k_spinlock timer_spinlock;
#else
#define TIMER_LOCK_DECLARE
#define TIMER_LOCK() 
#define TIMER_UNLOCK()
#endif //__ZEPHYR__

#ifndef unlikely
#define unlikely(x) (x)
#endif

#define WRITE_ONCE(x, val)						\
do {									\
	*(volatile __typeof__(x) *)&(x) = (val);				\
} while (0)

static struct timer_node timer_list = {
    &timer_list, &timer_list
};

/*
 * Timer node operations
 */
#define list_for_each(pos, head) \
		for (pos = (head)->next; pos != (head); pos = pos->next)

static inline void __list_add(struct timer_node *newnd,
	struct timer_node *prev, struct timer_node *next)  {
	next->prev = newnd;
	newnd->next = next;
	newnd->prev = prev;
	WRITE_ONCE(prev->next, newnd);
}

static inline void __list_del(struct timer_node * prev, 
	struct timer_node * next) {
	next->prev = prev;
	WRITE_ONCE(prev->next, next);
}

static inline void list_insert_before(struct timer_node *newnd, 
	struct timer_node *node)	{
	__list_add(newnd, node->prev, node);
}

static inline void list_del(struct timer_node *entry) {
	__list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

static inline int list_empty(const struct timer_node *head)  {
	return head->next == head;
}

/*
 * Timer implementiton
 */
static inline struct timer_struct *timer_first(void) {
	return !list_empty(&timer_list)? 
		TIMER_ENTRY(timer_list.next): NULL;
}

static int timer_add_locked(struct timer_struct* timer,
	unsigned long expires) {
	struct timer_node* ptr;
	if (unlikely(timer->state != DOUI_TIMER_IDLE))
		return -EBUSY;
	timer->state = DOUI_TIMER_INSERT;
	list_for_each(ptr, &timer_list) {
		struct timer_struct* tnode = TIMER_ENTRY(ptr);
		if (expires < tnode->expires) {
            tnode->expires -= expires;
			break;
        }
		expires -= tnode->expires;
	}
	timer->expires = expires;
	list_insert_before(&timer->node, ptr);
	timer->state = DOUI_TIMER_ACTIVED;
	return 0;
}

static int timer_remove_locked(struct timer_struct* timer) {
	struct timer_struct* next_timer;
	struct timer_node* next_node;
	if (timer->state == DOUI_TIMER_IDLE)
		return 0;
	next_node = timer->node.next;
	if (next_node != &timer_list) {
		next_timer = TIMER_ENTRY(next_node);
		next_timer->expires += timer->expires;
	}
	list_del(&timer->node);
	timer->state = DOUI_TIMER_IDLE;
	return 1;
}

long timer_ii_dispatch(long expires) {
	TIMER_LOCK_DECLARE
	struct timer_struct* timer;
	long next_expired;

	TIMER_LOCK();
	if (unlikely((timer = timer_first()) == NULL)) {
		next_expired = 0;
		goto _unlock;
	}
	if (timer->expires > expires) 
		goto _out;
	do {
		void (*fn)(struct timer_struct*);
		list_del(&timer->node);
		fn = timer->handler;
		expires -= timer->expires;
		timer->state = DOUI_TIMER_IDLE;
		TIMER_UNLOCK();
		fn(timer);
		TIMER_LOCK();
		if (unlikely((timer = timer_first()) == NULL)) {
			next_expired = 0;
			goto _unlock;
		}
	} while (timer->expires <= expires);
_out:
	next_expired = timer->expires - expires;
	timer->expires = next_expired;
_unlock:
	TIMER_UNLOCK();
	return next_expired;
}

int timer_ii_mod(struct timer_struct* timer, long expires) {
	TIMER_LOCK_DECLARE
	TIMER_LOCK();
	timer_remove_locked(timer);
	int ret = timer_add_locked(timer, expires);
	TIMER_UNLOCK();
	return ret;
}

int timer_ii_add(struct timer_struct* timer, long expires) {
	TIMER_LOCK_DECLARE
	TIMER_LOCK();
	int ret = timer_add_locked(timer, expires);
	TIMER_UNLOCK();
	return ret;
}

int timer_ii_remove(struct timer_struct* timer) {
	TIMER_LOCK_DECLARE
	TIMER_LOCK();
	int pending = timer_remove_locked(timer);
	TIMER_UNLOCK();
	return pending;
}

int timer_ii_foreach(void (*iterator)(struct timer_struct *)) {
    TIMER_LOCK_DECLARE
    struct timer_node *node;
    if (iterator == NULL)
        return -EINVAL;
    TIMER_LOCK();
	list_for_each(node, &timer_list) {
        iterator(TIMER_ENTRY(node));
	}
    TIMER_UNLOCK();
    return 0;
}