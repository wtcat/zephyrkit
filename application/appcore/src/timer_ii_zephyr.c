#include <stdbool.h>
#include <kernel.h>
#include <sys/__assert.h>

#include "timer_ii.h"

static struct k_timer root_timer;

static void variable_period_cb(struct k_timer *timer) {
    (void) timer;
    long next_expired = timer_ii_dispatch();
    if (next_expired > 0) {
        k_timer_start(&root_timer, K_MSEC(next_expired), 
            K_NO_WAIT);
    }
}

static void fixed_period_cb(struct k_timer *timer) {
    (void) timer;
    timer_ii_dispatch();
}

void root_timer_restart(int tick_res, bool fixed_period) {
    k_timer_stop(&root_timer);
    timer_ii_change_resolution(tick_res);
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