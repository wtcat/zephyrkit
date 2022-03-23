#include <stdbool.h>
#include <kernel.h>
#include <sys/__assert.h>

#include "timer_ii.h"

static struct k_timer root_timer;
static int tick_resolution = 1;

static void variable_period_cb(struct k_timer *timer) {
    (void) timer;
    long next_expires = timer_ii_dispatch(tick_resolution);
    if (next_expires > 0) {
        printk("Next Expires: %ld\n", next_expires);
        tick_resolution = next_expires;
        k_timer_start(&root_timer, K_MSEC(next_expires), 
            K_NO_WAIT);
    }
}

static void fixed_period_cb(struct k_timer *timer) {
    (void) timer;
    long next_expired;
    next_expired = timer_ii_dispatch(tick_resolution);
    printk("Next Expired**: %ld\n", next_expired);
}

void root_timer_restart(int tick_res, bool fixed_period) {
    k_timer_stop(&root_timer);
    tick_resolution = tick_res;
    if (fixed_period) {
        k_timer_init(&root_timer, fixed_period_cb, NULL);
        k_timer_start(&root_timer, K_MSEC(tick_res), K_MSEC(tick_res));
    } else {
        k_timer_init(&root_timer, variable_period_cb, NULL);
        k_timer_start(&root_timer, K_MSEC(tick_res), K_NO_WAIT);
    }   
}

void root_timer_stop(void) {
    k_timer_stop(&root_timer);
}