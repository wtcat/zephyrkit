#include <init.h>

#include "timer_ii.h"
#include "app_timer.h"

#if 1

APP_TIMER_DEF(timer_200ms);
APP_TIMER_DEF(timer_300ms);
APP_TIMER_DEF(timer_1000ms);

static int timer_200ms_counter;
static int timer_300ms_counter;
static int timer_1000ms_counter;

static void timer_200ms_cb(void *context) {
    timer_200ms_counter++;
}

static void timer_300ms_cb(void *context) {
    timer_300ms_counter++;
}

static void timer_1000ms_cb(void *context) {
    timer_1000ms_counter++;
    printk("TIMER: [200ms] = %d, [300] = %d, [1000] = %d\n",
        timer_200ms_counter, timer_300ms_counter, timer_1000ms_counter);
    if (timer_1000ms_counter == 10)
        app_timer_stop(timer_200ms);
    if (timer_1000ms_counter == 20)
        app_timer_start(timer_200ms, 200, NULL);
}

static int timer_test_init(const struct device *dev __unused) {
    app_timer_create(&timer_200ms, APP_TIMER_MODE_REPEATED_ISR, timer_200ms_cb);
    app_timer_create(&timer_300ms, APP_TIMER_MODE_REPEATED_ISR, timer_300ms_cb);
    app_timer_create(&timer_1000ms, APP_TIMER_MODE_REPEATED_ISR, timer_1000ms_cb);

    app_timer_start(timer_200ms, 200, NULL);
    app_timer_start(timer_300ms, 300, NULL);
    app_timer_start(timer_1000ms, 1000, NULL);
    return 0;
}

#else

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

#endif 
SYS_INIT(timer_test_init, APPLICATION, 20);
