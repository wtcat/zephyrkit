#include <init.h>

#include "timer_ii.h"
#include "app_timer.h"

extern void root_timer_restart(int res, bool fixed_period);

static void timer_1s(struct timer_struct *timer) {
    printk("--------------------- TimerOut: 5000\n");
    //timer_ii_add(timer, 5000);
}

static void timer_500ms(struct timer_struct *timer) {
    printk("----------- TimerOut: 3000\n");
    //timer_ii_add(timer, 3000);
}

static void timer_20ms(struct timer_struct *timer) {
    printk("- TimerOut: 1000\n");
    //timer_ii_add(timer, 1000);
}

static TIMER_II_DEFINE(timer_hdl_1s, timer_1s);
static TIMER_II_DEFINE(timer_hdl_500ms, timer_500ms);
static TIMER_II_DEFINE(timer_hdl_20ms, timer_20ms);
static long timer_expires[3];
static int offset;

static void timer_iterator(struct timer_struct *timer) {
    timer_expires[offset] = timer->expires;
    offset++;
}

void timer_ii_test(void) {
    root_timer_restart(100, false);
    timer_ii_add(&timer_hdl_1s, 5000);
    timer_ii_add(&timer_hdl_500ms, 3000);
    timer_ii_add(&timer_hdl_20ms, 1000);
    timer_ii_foreach(timer_iterator);

    for (int i = 0; i < 3; i++)
        printk("Expires(%d): %ld\n", i, timer_expires[i]);
}

static int timer_test_init(const struct device *dev __unused) {
    timer_ii_test();
    return 0;
}

SYS_INIT(timer_test_init, APPLICATION, 20);
