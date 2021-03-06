/*
 * CopyRight 2022 wtcat
 */
#ifndef TIMER_II_H_
#define TIMER_II_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C"{
#endif

enum timer_state {
	DOUI_TIMER_IDLE,
	DOUI_TIMER_INSERT,
	DOUI_TIMER_ACTIVED,
};

struct timer_node {
	struct timer_node *next, *prev;
};

struct timer_struct {
    struct timer_node node;
    void (*handler)(struct timer_struct*);
    long expires;
    enum timer_state state;
};

#define TIMER_II_DEFINE(_name, _handler) \
    struct timer_struct _name = { \
        .handler = _handler, \
        .state = DOUI_TIMER_IDLE \
    } 

static inline void TIMER_II_INIT(struct timer_struct *timer, 
    void (*handler)(struct timer_struct*)) {
    timer->handler = handler;
    timer->expires = 0;
    timer->state = DOUI_TIMER_IDLE;
    timer->node.next = timer->node.prev = NULL;
}

long timer_ii_dispatch(long expires);
int timer_ii_remove(struct timer_struct* timer);
int timer_ii_mod(struct timer_struct* timer, long expires);
int timer_ii_add(struct timer_struct* timer, long expires);
int timer_ii_foreach(void (*iterator)(struct timer_struct *));

#ifdef __cplusplus
}
#endif
#endif /* TIMER_II_H_ */
