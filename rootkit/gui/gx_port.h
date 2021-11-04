#ifndef LIB_GX_PORT_H_
#define LIB_GX_PORT_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <sys/util.h>

#ifdef CONFIG_GUIX_USER_MODE
#include <app_memory/app_memdomain.h>
#endif

#include "guix_zephyr_notify.h"
#include "base/observer_class.h"

typedef INT    GX_BOOL;
typedef SHORT  GX_VALUE;


#define UX_MSEC(n) \
    (((n) + (GX_SYSTEM_TIMER_MS - 1)) / GX_SYSTEM_TIMER_MS)

#define GX_VALUE_MAX                        0x7FFF

/* Define the basic system parameters.  */
#ifndef GX_THREAD_STACK_SIZE
#define GX_THREAD_STACK_SIZE                (6 * 1024)
#endif

#ifndef GX_TIMER_THREAD_STACK_SIZE
#define GX_TIMER_THREAD_STACK_SIZE          (4 * 1024)
#endif

#ifndef GX_TICKS_SECOND
#define GX_TICKS_SECOND                     20
#endif

#ifndef GX_WIDGET_USER_DATA
#define GX_WIDGET_USER_DATA
#endif

#define GX_CONST                            const
#define GX_INCLUDE_DEFAULT_COLORS
#define GX_MAX_ACTIVE_TIMERS                32
#define GX_SYSTEM_TIMER_MS                  10   /* ms */
#define GX_SYSTEM_TIMER_TICKS               2           
#define GX_MAX_VIEWS                        32
#define GX_MAX_DISPLAY_HEIGHT               800
#define GX_SYSTEM_THREAD_PRIORITY           9

/* Override define */
#define GX_CALLER_CHECKING_EXTERNS
#define GX_THREADS_ONLY_CALLER_CHECKING
#define GX_INIT_AND_THREADS_CALLER_CHECKING
#define GX_NOT_ISR_CALLER_CHECKING
#define GX_THREAD_WAIT_CALLER_CHECKING

#define GX_MAX_CONTEXT_NESTING 20

/* */
#define __gxidata K_APP_DMEM(guix_partition)
#define __gxdata  K_APP_BMEM(guix_partition)

#ifdef __cplusplus
extern "C"{
#endif

struct GX_DISPLAY_STRUCT;
struct GX_THEME_STRUCT;
struct GX_STRING_STRUCT;
struct GX_EVENT_STRUCT;


#define INVALID_MAP_ADDR ((void *)(-1))
struct gxres_device {
    const char *dev_name;
    UINT link_id;
    void *(*mmap)(size_t size);
    void (*unmap)(void *ptr);
    struct gxres_device *next;
};

struct guix_driver {
    UINT (*setup)(struct GX_DISPLAY_STRUCT *display);
    UINT id;

    /* Private data */
    const struct gxres_device *dev;
    struct guix_driver *next;
    void *map_base;
};

static inline void *guix_resource_map(struct guix_driver *drv, size_t size)
{
    return drv->dev->mmap(size);
}

static inline void guix_resource_unmap(struct guix_driver *drv, void *ptr)
{
    return drv->dev->unmap(ptr);
}

UINT guix_binres_load(struct guix_driver *drv, INT theme_id, 
    struct GX_THEME_STRUCT **theme, struct GX_STRING_STRUCT ***language);
UINT guix_main(UINT disp_id, struct guix_driver *drv);

/*
 * No thread-safe
 */
int guix_driver_register(struct guix_driver *drv);
int gxres_device_register(struct gxres_device *dev);

/*
 * Wake up event filter
 */
void guix_event_filter_set(bool (*filter)(struct GX_EVENT_STRUCT *));

/*
 * method:
 *  int guix_state_notify(unsigned long state, void *ptr)
 *  int guix_state_add_observer(struct observer_base *obs)
 *  int guix_state_remove_observer(struct observer_base *obs)
 */
#define GUIX_STATE_DOWN 0x00 
#define GUIX_STATE_UP   0x01 /* GUIX initialize completed */

OBSERVER_CLASS_DECLARE(guix_state)

#ifdef __cplusplus
}
#endif
#endif /* LIB_GX_PORT_H_ */

