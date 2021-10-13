#include <kernel.h>
#include <logging/log.h>

#include "base/sys_power.h"

LOG_MODULE_REGISTER(sys_power);

static K_MUTEX_DEFINE(mutex);
static struct observer_base *power;
static enum power_state current_state;
static const char *power_state_str[] = {
    [SYS_POWER_NORMAL]  = "Normal",
    [SYS_POWER_LOWER]   = "Lower-power",
    [SYS_POWER_SUSPEND] = "Suspend"
};

static bool sys_power_state_check(int req)
{
    switch (req) {
    case SYS_POWER_NORMAL:
    case SYS_POWER_SUSPEND:
        if (current_state == req)
            return false;
        break;
    case SYS_POWER_LOWER:
        if (current_state != SYS_POWER_NORMAL)
            return false;
        break;
    default:
        return false;
    }
    return true;
}

const char *sys_power_state_text(void)
{
    return power_state_str[current_state];
}

int sys_power_observer_add(struct power_observer *obs)
{
    int ret;
    __ASSERT(obs != NULL, "");
    k_mutex_lock(&mutex, K_FOREVER);
    ret = observer_cond_register(&power, &obs->base);
    k_mutex_unlock(&mutex);
    return ret;
}

int sys_power_request(enum power_state state)
{
    int ret;
    k_mutex_lock(&mutex, K_FOREVER);
    if (sys_power_state_check(state)) {
        ret = observer_notify(&power, state, NULL);
        if (!ret) {
            LOG_INF("System power changed: %s -> %s\n", 
                power_state_str[current_state], power_state_str[state]);
            current_state = state;
        }
    } else {
        ret = -EINVAL;
    }
    k_mutex_unlock(&mutex);
    return ret;
}

void sys_power_request_reboot(void)
{
    sys_power_request(SYS_POWER_SUSPEND);
    sys_arch_reboot(0);
    for (;;) {
        LOG_INF("Waiting system reboot...\n");
        k_sleep(K_MSEC(1000));
    };
}

