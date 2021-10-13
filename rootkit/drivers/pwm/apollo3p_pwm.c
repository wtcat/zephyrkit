#include <kernel.h>
#include <device.h>
#include <devicetree.h>
#include <soc.h>
#include <errno.h>
#include <logging/log.h>
#include <drivers/pwm.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_apollo3p);

#include "apollo3p_ctimer.h"

#define DT_DRV_COMPAT ambiq_apollo3p_pwm

/* A and B */
#define MAX_PWM_CHANNELS 2

#define PWM_COUNT_MASK 0xFFFF0000
#define PWM_CH_MASK 0x0000FFFF

#define PWM_CH(pwm)  (pwm & PWM_CH_MASK)
#define PWM_CNT(pwm) ((pwm & PWM_COUNT_MASK) >> 16)

/* Compare register 1 */
#define PWM_COMPARE_INT_MASK 0x400

struct pwm_channel {
    uint32_t duty;
    uint32_t period;
    uint16_t repeats;
    uint8_t pad;
    uint8_t up:1;
    uint8_t once:1;
    uint8_t pin_attached:1;
};

struct apollo_pwm_priv {
    struct pwm_channel ch[MAX_PWM_CHANNELS];
    uint32_t id;
    uint32_t clkfreq;
    uint32_t clksel;
};

static const uint32_t timer_segmask[] = {
    AM_HAL_CTIMER_TIMERA,
    AM_HAL_CTIMER_TIMERB,
};

static const uint8_t idle_output[2][2] = {
    {AM_HAL_CTIMER_OUTPUT_FORCE0, AM_HAL_CTIMER_OUTPUT_FORCE1},
    {AM_HAL_CTIMER_OUTPUT_FORCE1, AM_HAL_CTIMER_OUTPUT_FORCE0}
};

static void ctimer_clear_and_start(uint32_t timer, uint32_t seg, bool start)
{
    volatile uint32_t *ctrl = (uint32_t *)CTIMERADDRn(CTIMER, timer, CTRL0);
	uint32_t clr_mask, set_mask;
    uint32_t value;
    uint32_t key;
	
	clr_mask = seg & (CTIMER_CTRL0_TMRA0CLR_Msk | CTIMER_CTRL0_TMRB0CLR_Msk);
	set_mask = seg & (CTIMER_CTRL0_TMRA0EN_Msk | CTIMER_CTRL0_TMRB0EN_Msk);
    key = irq_lock();
	value = *ctrl;
	value &= ~(clr_mask | set_mask);
	/* clear timer*/
	*ctrl = value | clr_mask;
	/* start timer*/
    if (start)
	    *ctrl = value | set_mask;
    irq_unlock(key);
}

static void ctimer_pwm_isr(const struct device *dev, int offset)
{
    struct apollo_pwm_priv *priv = dev->data;
    struct  pwm_channel *ch;
    int channel;
    
    channel = offset & 1;
    ch = &priv->ch[channel];
    if (ch->once) {
        if (ch->repeats > 1) {
            ch->repeats--;
            ctimer_clear_and_start(priv->id, timer_segmask[channel], true);
        } else {
            ctimer_clear_and_start(priv->id, timer_segmask[channel], false);
        }
    }
}

static int pwm_config_once(const struct device *dev, 
    struct apollo_pwm_priv *priv, uint32_t channel, pwm_flags_t flags)
{
    uint32_t timercfg = priv->clksel | PWM_COMPARE_INT_MASK;
    int index;
    int ret;
    
    if (flags & PWM_POLARITY_INVERTED) 
        timercfg |= AM_HAL_CTIMER_FN_PWM_ONCE | AM_HAL_CTIMER_PIN_INVERT;
    else
        timercfg |= AM_HAL_CTIMER_FN_PWM_ONCE;
    am_hal_ctimer_config_single(priv->id, timer_segmask[channel], timercfg);
    index = (priv->id << 1) + 16;
    index += channel;
    ret = ctimer_register_isr(index, (void (*)(void *, int))ctimer_pwm_isr, 
        (void *)dev);
    __ASSERT(ret == 0, "");
    return ret;    
}

static int pwm_config_repeat(struct apollo_pwm_priv *priv, 
    uint32_t channel, pwm_flags_t flags)
{
    uint32_t timercfg = priv->clksel;
    int index;
    int ret;
    
    if (flags & PWM_POLARITY_INVERTED) 
        timercfg |= AM_HAL_CTIMER_FN_PWM_REPEAT | AM_HAL_CTIMER_PIN_INVERT;
    else
        timercfg |= AM_HAL_CTIMER_FN_PWM_REPEAT;
    index = (priv->id << 1) + 16;
    index += channel;
    ret = ctimer_unregister_isr(index);
    __ASSERT(ret == 0, "");
    am_hal_ctimer_config_single(priv->id, timer_segmask[channel], timercfg);
    return 0;
}

static int apollo_pwm_set_cycles(const struct device *dev, uint32_t pwm,
    uint32_t period, uint32_t pulse, pwm_flags_t flags)
{
    struct apollo_pwm_priv *priv = dev->data;
    struct  pwm_channel *ch;
    uint32_t channel = PWM_CH(pwm);
    uint16_t count = PWM_CNT(pwm);
    uint32_t seg;

    __ASSERT(channel < MAX_PWM_CHANNELS, "");
    __ASSERT(pulse <= period, "");
    ch = &priv->ch[channel];
    seg = timer_segmask[channel];
    __ASSERT(ch->pad != 0xFF, "");
    if (pulse == 0 || pulse == period) {
        ch->pin_attached = false;
        am_hal_ctimer_clear(priv->id, seg);
        am_hal_ctimer_output_config(priv->id, seg, ch->pad,
            idle_output[flags][!!pulse], AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA);
        return 0;
    }

    if (ch->once != !!count) {
        ch->once = !!count;
        ch->up = true;
        if (ch->once) {
            pwm_config_once(dev, priv, channel, flags);
            ch->repeats = count;
        } else {
            pwm_config_repeat(priv, channel, flags);
        }
    }  else if (!ch->up) {
        ch->up = true;
        pwm_config_repeat(priv, channel, flags);
    } else {
        ch->repeats = count;
    }

    if (!ch->pin_attached) {
        ch->pin_attached = true;
        am_hal_ctimer_output_config(priv->id, seg, ch->pad,
            AM_HAL_CTIMER_OUTPUT_NORMAL, AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA);
    }

    if (pulse != ch->duty || period != ch->period) {
        am_hal_ctimer_period_set(priv->id, seg, period, pulse);
        ch->duty = pulse;
        ch->period = period;
    }
    am_hal_ctimer_start(priv->id, seg);
    return 0;
}

static int apollo_pwm_get_cycles_per_sec(const struct device *dev, 
    uint32_t pwm, uint64_t *cycles)
{
    struct apollo_pwm_priv *priv = dev->data;
    if (PWM_CH(pwm) >=  MAX_PWM_CHANNELS)
        return -EINVAL;
    *cycles = priv->clkfreq;
    return 0;
}

static const struct pwm_driver_api apollo_pwm_api = {
	.pin_set = apollo_pwm_set_cycles,
	.get_cycles_per_sec = apollo_pwm_get_cycles_per_sec,
};

static int apollo_pwm_init(const struct device *dev)
{
    struct apollo_pwm_priv *priv = dev->data;

    switch (priv->clksel) {
    // case AM_HAL_CTIMER_HFRC_24MHZ:
    //     priv->clkfreq = 24000000;
    //     break;
    case AM_HAL_CTIMER_HFRC_3MHZ:
        priv->clkfreq = 3000000;
        break;
    case AM_HAL_CTIMER_HFRC_187_5KHZ:
        priv->clkfreq = 187500;
        break;
    case AM_HAL_CTIMER_HFRC_47KHZ:
        priv->clkfreq = 47000;
        break;
    case AM_HAL_CTIMER_HFRC_12KHZ:
        priv->clkfreq = 12000;
        break;
    case AM_HAL_CTIMER_XT_32_768KHZ:
        priv->clkfreq = 32768;
        break;
    case AM_HAL_CTIMER_RTC_100HZ:
        priv->clkfreq = 100;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

#define PWM_DEVICE_INIT(inst)\
static struct apollo_pwm_priv DT_CAT(pwm_private_, inst) = { \
    .ch = { \
        [0] = {.pad = DT_PROP(DT_DRV_INST(inst), ch0_pin)}, \
        [1] = {.pad = DT_PROP(DT_DRV_INST(inst), ch1_pin)}, \
     }, \
    .id = DT_REG_ADDR(DT_PARENT(DT_DRV_INST(inst))), \
    .clksel = DT_PROP_BY_IDX(DT_PARENT(DT_DRV_INST(inst)), clock_source, 0), \
};\
DEVICE_DT_DEFINE(DT_DRV_INST(inst), \
		    &apollo_pwm_init, \
		    device_pm_control_nop, \
		    &DT_CAT(pwm_private_, inst), \
		    NULL, \
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
		    &apollo_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT)
    