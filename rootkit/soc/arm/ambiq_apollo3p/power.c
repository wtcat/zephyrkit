
#include <zephyr.h>
#include <version.h>
#include <soc.h>
#include <init.h>

#if (KERNEL_VERSION_NUMBER >= ZEPHYR_VERSION(2,6,0))
#include <pm/pm.h>
#else
#include <power/power.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(soc);


static inline void cpu_rest(void)
{
    uint32_t value;
    __asm__ volatile(
        "dsb 0xF\n"
        "ldr %[val], =0x5FFF0000\n"
        "ldr %[val], [%[val]]\n"
        "wfi\n"
        "isb 0xF\n"
        : [val] "=r" (value)
        : :"memory"
    );
}

static bool cpu_enter_sleep(bool deepsleep)
{
    am_hal_burst_mode_e mode;
    bool restore, success = true;
    uint32_t scr;

    AM_CRITICAL_BEGIN
    irq_unlock(0); /* Reenable interrupts */
    if (am_hal_burst_mode_status() == AM_HAL_BURST_MODE) {
        if (am_hal_burst_mode_disable(&mode)) {
            LOG_ERR("%s(): Failed transition (TurboSpot -> Active)\n", __func__);
            success = false;
            goto _failed;
        }
        restore = true;
    } else {
        restore = false;
    }

    scr = SCB->SCR;
    if (deepsleep && !MCUCTRL->TPIUCTRL_b.ENABLE) {
        if (!gAmHalResetStatus)
            gAmHalResetStatus = RSTGEN->STAT;
        SCB->SCR = scr | _VAL2FLD(SCB_SCR_SLEEPDEEP, 1);
    } else {
        SCB->SCR = scr & ~_VAL2FLD(SCB_SCR_SLEEPDEEP, 1);
    }

    cpu_rest();
    if (restore && am_hal_burst_mode_enable(&mode)) {
        LOG_ERR("%s(): Failed transition (Active -> TurboSpot)\n", __func__);
    }

_failed:
    AM_CRITICAL_END
    return success;
}

void pm_power_state_set(struct pm_state_info info)
{
    if (info.state != PM_STATE_SUSPEND_TO_IDLE) {
      LOG_DBG("Unsupported power state %u", info.state);
      return;
    }

    LOG_DBG("Enter sleep...\n");
    irq_unlock(0);
    switch (info.substate_id) {
    case 0: 
        cpu_enter_sleep(false);
        break;
    case 1:
        cpu_enter_sleep(true);
        break;
    default:
        LOG_DBG("Unsupported power state substate-id %u", info.substate_id);
        break;
    }
    LOG_DBG("Exit sleep...\n");
}

void pm_power_state_exit_post_ops(struct pm_state_info info)
{
    __ASSERT(info.state == PM_STATE_SUSPEND_TO_IDLE, 
        "Unsupported power substate-id");
    __ASSERT(info.substate_id == 0 || info.substate_id == 1, 
        "Unsupported power substate-id");
}
