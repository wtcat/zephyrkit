#ifndef LIB_GUI_GX_ZEPHYR_NOTIFY_H_
#define LIB_GUI_GX_ZEPHYR_NOTIFY_H_

#include "base/observer.h"

#ifdef __cplusplus
extern "C"{
#endif

//struct observer_base;

#define GUIX_ENTER_SLEEP  0
#define GUIX_EXIT_SLEEP   1

/* Public interface */
int guix_suspend_notify_register(struct observer_base *observer);
int guix_suspend_notify_unregister(struct observer_base *observer);


#ifdef __cplusplus
}
#endif
#endif /* LIB_GUI_GX_ZEPHYR_NOTIFY_H_ */
