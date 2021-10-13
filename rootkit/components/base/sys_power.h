#ifndef BASE_SYS_POWER_H_
#define BASE_SYS_POWER_H_

#include "base/observer.h"

#ifdef __cplusplus
extern "C"{
#endif

/*
 * System power state
 */
enum power_state {
    SYS_POWER_NORMAL,
    SYS_POWER_SUSPEND,
    SYS_POWER_LOWER,
};

struct power_observer {
    struct observer_base base;
};

const char *sys_power_state_text(void);
int sys_power_observer_add(struct power_observer *obs);
int sys_power_request(enum power_state state);
void sys_power_request_reboot(void);

#ifdef __cplusplus
}
#endif
#endif /* BASE_SYS_POWER_H_ */