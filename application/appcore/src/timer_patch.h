/*
 * CopyRight 2022 wtcat
 */
#ifndef TIMER_PATCH_H_
#define TIMER_PATCH_H_

#include <zephyr.h>

#include "timer_ii.h"

#ifdef __cplusplus
extern "C"{
#endif

struct timer_operations {
	int (*start)(app_timer_t * ptimer, long expires);
	int (*stop)(app_timer_t * ptimer);
};

//static struct hrtimer timer_ii_root;
static struct k_timer timer_ii_root;
static int timer_ii_res = 200;    /* Default timer resolution: 200ms */

static int hr_timer_start(app_timer_t * ptimer, long expired) {
	//hrtimer_start(&ptimer->timer, HRTIMER_MSEC((s32_t)expired), 0);
    k_timer_start(&ptimer->timer, K_MSEC(expired), K_NO_WAIT);
	return 0;
}

static int hr_timer_stop(app_timer_t * ptimer) {
	//hrtimer_stop(&ptimer->timer);
    k_timer_stop(&ptimer->timer);
	return 0;
}

static int timer_ii_start(app_timer_t * ptimer, long expired) {
	return timer_ii_mod(&ptimer->timer_ii, expired);
}

static int timer_ii_stop(app_timer_t * ptimer) {
	return timer_ii_remove(&ptimer->timer_ii);
}

static const struct timer_operations timer_ops[] = {
	{ .start = hr_timer_start, .stop = hr_timer_stop },
	{ .start = timer_ii_start, .stop = timer_ii_stop }
};

static inline int abstract_timer_start(app_timer_t * ptimer, long expired) {
	return timer_ops[ptimer->is_timer_ii].start(ptimer, expired);
}

static inline int abstract_timer_stop(app_timer_t * ptimer) {
	return timer_ops[ptimer->is_timer_ii].stop(ptimer);
}

static inline void timer_ii_hrtimer_cb(struct k_timer *timer) {
	(void) timer;
	long next_expires = timer_ii_dispatch(timer_ii_res);
	if (next_expires > 0) {
		printk("Next Timeout: %ld\n", next_expires);
		timer_ii_res = (int)next_expires;
        k_timer_start(&timer_ii_root, K_MSEC(next_expires), K_NO_WAIT);
		//hrtimer_start(&timer_ii_root, HRTIMER_MSEC(next_expires), 0);
	}
}

static inline void abstract_timer_post(app_timer_t * ptimer) {
	if (ptimer->is_timer_ii && !timer_ii_root.timeout.node.next) {
        k_timer_stop(&timer_ii_root);
        k_timer_init(&timer_ii_root, timer_ii_hrtimer_cb, NULL);
        k_timer_start(&timer_ii_root, K_MSEC(timer_ii_res), K_NO_WAIT);
		// hrtimer_stop(&timer_ii_root);
		// hrtimer_init(&timer_ii_root, timer_ii_hrtimer_cb, NULL);
		// hrtimer_start(&timer_ii_root, HRTIMER_MSEC(timer_ii_res), 0);
	}
}

static inline void abstract_timer_reset(app_timer_t * ptimer, long expired) {
	if (expired == APP_TIMER_TICKS(200) ||
		expired == APP_TIMER_TICKS(1000)) {
		ptimer->is_timer_ii = 1;
		TIMER_II_INIT(&ptimer->timer_ii, 
			(void (*)(struct timer_struct*))app_timer_callback);
	} else {
		ptimer->is_timer_ii = 0;
	}
}


#ifdef __cplusplus
}
#endif
#endif /* TIMER_PATCH_H_ */

