#include <kernel.h>
#include <version.h>
#include <device.h>
#include <drivers/led.h>
#include <drivers/pwm.h>
#include <sys/math_extras.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(motor_pwm);

#define DT_DRV_COMPAT pwm_motors

struct motor_pwm {
	const struct device *dev;
	uint32_t channel;
	uint32_t period;
	pwm_flags_t flags;
};

static int motor_pwm_blink(const struct device *dev, uint32_t exec_count,
    uint32_t delay_on, uint32_t delay_off)
{
	const struct motor_pwm *cfg = dev->config;
	uint32_t period_usec, pulse_usec;

	if (u32_add_overflow(delay_on, delay_off, &period_usec) ||
	    u32_mul_overflow(period_usec, 1000, &period_usec) ||
	    u32_mul_overflow(delay_on, 1000, &pulse_usec)) {
		return -EINVAL;
	}

    exec_count = (exec_count << 16) | cfg->channel;
	return pwm_pin_set_usec(cfg->dev, exec_count, period_usec,
        pulse_usec, cfg->flags);
}

static int motor_pwm_set_duty(const struct device *dev,
				  uint32_t led, uint8_t value)
{
	return -ENOTSUP;
}

static int motor_pwm_on(const struct device *dev, uint32_t unused)
{
    const struct motor_pwm *cfg = dev->config;
    ARG_UNUSED(unused);
	return pwm_pin_set_usec(cfg->dev, cfg->channel, USEC_PER_SEC,
        USEC_PER_SEC, cfg->flags);
}

static int motor_pwm_off(const struct device *dev, uint32_t unused)
{
    const struct motor_pwm *cfg = dev->config;
    ARG_UNUSED(unused);
	return pwm_pin_set_usec(cfg->dev, cfg->channel, cfg->period,
        0, cfg->flags);
}

static int motor_pwm_init(const struct device *dev)
{
    const struct motor_pwm *cfg = dev->config;
    if (!device_is_ready(cfg->dev)) {
    	LOG_ERR("%s: pwm device not ready", dev->name);
    	return -ENODEV;
    }
	return 0;
}

static const struct led_driver_api motor_pwm_api = {
	.on		= motor_pwm_on,
	.off		= motor_pwm_off,
	.blink		= motor_pwm_blink,
	.set_brightness	= motor_pwm_set_duty,
};

#if KERNEL_VERSION_NUMBER < ZEPHYR_VERSION(2,5,99)
#define MOTOR_PWM_ARGS(nid)	{\
	.dev		= DEVICE_DT_GET(DT_PHANDLE_BY_IDX(nid, pwms, 0)),	\
	.channel	= DT_PWMS_CHANNEL(nid),			\
	.period		= DT_PHA_OR(nid, pwms, period, 100),	\
	.flags		= DT_PHA_OR(nid, pwms, flags,		\
				    PWM_POLARITY_NORMAL),		\
},
#else
#define MOTOR_PWM_ARGS(nid)	{\
	.dev		= DEVICE_DT_GET(DT_PWMS_CTLR(nid)),	\
	.channel	= DT_PWMS_CHANNEL(nid),			\
	.period		= DT_PHA_OR(nid, pwms, period, 100),	\
	.flags		= DT_PHA_OR(nid, pwms, flags,		\
				    PWM_POLARITY_NORMAL),		\
},
#endif /* KERNEL_VERSION_NUMBER */

#define MOTOR_PWM(nid) \
static const struct motor_pwm motor_pwm_config##nid[] = { \
    DT_INST_FOREACH_CHILD(nid, MOTOR_PWM_ARGS) \
}; \
DEVICE_DEFINE(motor_pwm_##nid,   \
            DT_INST_PROP(nid, label), \
            motor_pwm_init,              \
            device_pm_control_nop,          \
            NULL,                   \
            motor_pwm_config##nid,           \
            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
            &motor_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(MOTOR_PWM)
