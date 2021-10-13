
#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include <sys/util.h>

#include "gpio/gpio_utils.h"

LOG_MODULE_REGISTER(gpio_apollo3p);


struct gpio_apollo3p_data {
    struct gpio_driver_data common;
    sys_slist_t cb_list;
    struct device *dev;
};

struct gpio_apollo3p_cfg {
    struct gpio_driver_config common;
    void *intbase;
    uint32_t port_offset;
    uint32_t pin_offset;
};

struct gpio_isr_desc {
    void (*handler)(int, void *);
    void *arg;
};

struct gpio_intreg {
    __IOM uint32_t inten;
    __IOM uint32_t intstat;
    __IOM uint32_t intclr;
    __IOM uint32_t intset;
    uint32_t reserved[4];
};


#define REG_BASE(reg) (struct gpio_intreg *)(reg)



static struct gpio_intreg *const gpio_intreg_base[] = {
    REG_BASE(&GPIO->INT0EN),
    REG_BASE(&GPIO->INT1EN),
    REG_BASE(&GPIO->INT2EN),
};

static void gpio_apollo3p_default_isr(int pin, void *arg);
static struct gpio_isr_desc gpio_desc[AM_HAL_GPIO_MAX_PADS] = {
    [0 ... AM_HAL_GPIO_MAX_PADS-1] = {
        .handler = gpio_apollo3p_default_isr,
        .arg = NULL
    }
};
static bool gpio_irq_initialized;

static void gpio_apollo3p_default_isr(int pin, void *arg)
{
    LOG_WRN("Please install gpio-pin(%d) interrupt process routine\n", pin);
}

static void gpio_apollo3p_pin_isr(int pin, void *arg)
{
	struct gpio_apollo3p_data *data = arg;
	gpio_fire_callbacks(&data->cb_list, data->dev, BIT(pin & 31));
}

static void gpio_apollo3p_controller_isr(const void *arg)
{
    (void) arg;
    struct gpio_isr_desc *desc;
    uint32_t base = 0;
    uint32_t status;
    uint32_t ffs;
  
    while (base < AM_HAL_GPIO_MAX_PADS) {
        struct gpio_intreg *reg = gpio_intreg_base[base >> 5];
        status = reg->intstat & reg->inten;

        /* Clear GPIO pending interrupt */
        reg->intclr = status;
        while (status) {
            ffs = status & (uint32_t)(-(int32_t)status);
            ffs = 31 - AM_ASM_CLZ(ffs);  
            status &= ~(0x1ul << ffs);

            desc = &gpio_desc[base + ffs];
            desc->handler(base + ffs, desc->arg);
        }
        
        base += 32;
    }
}

static int gpio_apollo3p_enable_irq(const struct gpio_apollo3p_cfg *cfg,
    int pin)
{
    struct gpio_intreg *reg = cfg->intbase;
    unsigned int key;

    key = irq_lock();
    reg->inten |= BIT(pin);
    irq_unlock(key);
    return 0;
}

static int gpio_apollo3p_disable_irq(const struct gpio_apollo3p_cfg *cfg,
    int pin)
{
     struct gpio_intreg *reg = cfg->intbase;
     unsigned int key;

     key = irq_lock();
     reg->inten &= ~(BIT(pin));
     irq_unlock(key);
     return 0;
}

int gpio_apollo3p_request_irq(const struct device *dev, int pin, 
    void (*handler)(int, void *), void *arg,
    enum gpio_int_trig trig, bool auto_en) 
{
    const struct gpio_apollo3p_cfg *cfg = dev->config;
    am_hal_gpio_pincfg_t pincfg;
    struct gpio_isr_desc *desc;
    unsigned int key;
    int ret;
    
    if (pin >= AM_HAL_GPIO_MAX_PADS)
        return -EINVAL;

    if (handler == NULL)
        return -EINVAL;

    gpio_apollo3p_disable_irq(cfg, pin);
    pincfg = g_AM_HAL_GPIO_INPUT_PULLUP;
    switch (trig) {
    case GPIO_INT_TRIG_LOW:
        pincfg.eIntDir = AM_HAL_GPIO_PIN_INTDIR_HI2LO;
        break;
    case GPIO_INT_TRIG_HIGH:
        pincfg.eIntDir = AM_HAL_GPIO_PIN_INTDIR_LO2HI;
        break;
    case GPIO_INT_TRIG_BOTH:
        pincfg.eIntDir = AM_HAL_GPIO_PIN_INTDIR_BOTH;
        break;
    default:
        return -EINVAL;
    }

    desc = &gpio_desc[pin + cfg->pin_offset];
    key = irq_lock();
    desc->handler = handler;
    desc->arg = arg;
    irq_unlock(key);
    ret = (int)am_hal_gpio_pinconfig(pin + cfg->pin_offset, pincfg);
    if (!ret && auto_en)
        ret = gpio_apollo3p_enable_irq(cfg, pin);
    return ret;    
}

static void gpio_apollo3p_remove_irq(const struct device *dev, int pin)
{
    const struct gpio_apollo3p_cfg *cfg = dev->config;
    struct gpio_isr_desc *desc = &gpio_desc[pin + cfg->pin_offset];
    unsigned int key;
    
    key = irq_lock();
    desc->handler = gpio_apollo3p_default_isr;
    desc->arg = NULL;
    irq_unlock(key);
}

static int gpio_apollo3p_port_set_masked_raw(const struct device *dev,
    gpio_port_pins_t mask, gpio_port_value_t value)			  
{
	const struct gpio_apollo3p_cfg *cfg = dev->config;
	uint32_t port_value;

    port_value = AM_REGVAL(AM_REGADDR(GPIO, WTA) + cfg->port_offset);
    port_value = (port_value & ~mask) | (mask & value);
    AM_REGVAL(AM_REGADDR(GPIO, WTA) + cfg->port_offset) = port_value;
	return 0;
}

static int gpio_apollo3p_port_get_raw(const struct device *dev, 
    uint32_t *value)
{
	const struct gpio_apollo3p_cfg *cfg = dev->config;
    *value = AM_REGVAL(AM_REGADDR(GPIO, RDA) + cfg->port_offset);
	return 0;
}

static int gpio_apollo3p_port_set_bits_raw(const struct device *dev,
    gpio_port_pins_t pins)
{
    const struct gpio_apollo3p_cfg *cfg = dev->config;
    AM_REGVAL(AM_REGADDR(GPIO, WTSA) + cfg->port_offset) = pins;
	return 0;
}

static int gpio_apollo3p_port_clear_bits_raw(const struct device *dev,
    gpio_port_pins_t pins)
{
    const struct gpio_apollo3p_cfg *cfg = dev->config;
    AM_REGVAL(AM_REGADDR(GPIO, WTCA) + cfg->port_offset) = pins;
	return 0;
}

static int gpio_apollo3p_port_toggle_bits(const struct device *dev,
    gpio_port_pins_t pins)
{
    const struct gpio_apollo3p_cfg *cfg = dev->config;
    uint32_t mask;

    mask = AM_REGVAL(AM_REGADDR(GPIO, WTA) + cfg->port_offset);
    mask ^= pins;
    AM_REGVAL(AM_REGADDR(GPIO, WTA) + cfg->port_offset) = mask;
    return 0;
}

static int gpio_apollo3p_pin_interrupt_configure(const struct device *dev,
    gpio_pin_t pin, enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	struct gpio_stm32_data *data = dev->data;
        int err = 0;

	if (mode == GPIO_INT_MODE_DISABLED) {
        gpio_apollo3p_disable_irq(dev->config, pin);
        gpio_apollo3p_remove_irq(dev, pin);
        goto exit;
	}

	/* Level trigger interrupts not supported */
	if (mode == GPIO_INT_MODE_LEVEL) {
		err = -ENOTSUP;
		goto exit;
	}

    err = gpio_apollo3p_request_irq(dev, pin, gpio_apollo3p_pin_isr,
        data, trig, false);
    if (err)
        goto exit;
    
    gpio_apollo3p_enable_irq(dev->config, pin);
exit:
	return err;
}

static int gpio_apollo3p_manage_callback(const struct device *dev,
    struct gpio_callback *callback, bool set)      
{
	struct gpio_apollo3p_data *data = dev->data;
	return gpio_manage_callback(&data->cb_list, callback, set);
}

static uint32_t gpio_apollo3p_get_pending_int(const struct device *dev)
{
    return 0;
}

static int gpio_apollo3p_config(const struct device *dev,
    gpio_pin_t pin, gpio_flags_t flags)
{
    const struct gpio_apollo3p_cfg *cfg = dev->config;
    am_hal_gpio_pincfg_t pincfg;

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_SINGLE_ENDED) {
		    if (flags & GPIO_LINE_OPEN_DRAIN)
                pincfg.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN;
	        else
			    return -ENOTSUP;
		} else {
		    pincfg.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL;
		}

		if (flags & GPIO_PULL_UP) {
			pincfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_24K;
		} else if (flags & GPIO_PULL_DOWN) {
			pincfg.ePullup = AM_HAL_GPIO_PIN_PULLDOWN;
		}
	} else if (flags & GPIO_INPUT) {
        pincfg.eGPOutcfg = AM_HAL_GPIO_PIN_OUTCFG_DISABLE;
        pincfg.eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE;
	    if (flags & GPIO_PULL_UP) {
                pincfg.ePullup = AM_HAL_GPIO_PIN_PULLUP_24K;
	    } else if (flags & GPIO_PULL_DOWN) {
		    pincfg.ePullup = AM_HAL_GPIO_PIN_PULLDOWN;
	    } else {
		    return -ENOTSUP;
	    }
	} else {
	    return -ENOTSUP;
	}

    pincfg.uFuncSel = 3;
    pincfg.eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA;
    am_hal_gpio_pinconfig(pin + cfg->pin_offset, pincfg);
	if (flags & GPIO_OUTPUT) {
	    if (flags & GPIO_OUTPUT_INIT_HIGH) {
	        gpio_apollo3p_port_set_bits_raw(dev, BIT(pin));
	    } else if (flags & GPIO_OUTPUT_INIT_LOW) {
		    gpio_apollo3p_port_clear_bits_raw(dev, BIT(pin));
	    }
	}

	return 0;
}

static const struct gpio_driver_api gpio_apollo3p_driver_api = {
	.pin_configure = gpio_apollo3p_config,
	.port_get_raw = gpio_apollo3p_port_get_raw,
	.port_set_masked_raw = gpio_apollo3p_port_set_masked_raw,
	.port_set_bits_raw = gpio_apollo3p_port_set_bits_raw,
	.port_clear_bits_raw = gpio_apollo3p_port_clear_bits_raw,
	.port_toggle_bits = gpio_apollo3p_port_toggle_bits,
	.pin_interrupt_configure = gpio_apollo3p_pin_interrupt_configure,
	.manage_callback = gpio_apollo3p_manage_callback,
	.get_pending_int = gpio_apollo3p_get_pending_int
};

static int gpio_apollo3p_init(const struct device *dev)
{
    /* */
    if (!gpio_irq_initialized) {
        gpio_irq_initialized = true;
        
        for (int i = 0; i < ARRAY_SIZE(gpio_intreg_base); i++) {
            struct gpio_intreg *gpio = gpio_intreg_base[i];
            gpio->intclr = 0xFFFFFFFF;
        } 

        irq_connect_dynamic(GPIO_IRQn, 0, gpio_apollo3p_controller_isr,
            NULL, 0);
        irq_enable(GPIO_IRQn);
    }
    return 0;
}

#define DT_DRV_COMPAT ambiq_apollo3p_gpio
#define GPIO_INIT(nid) \
    static struct gpio_apollo3p_data apollo3p_gpio##nid##_private; \
    static const struct gpio_apollo3p_cfg apollo3p_gpio##nid##_config = { \
        .common = {GPIO_PORT_PIN_MASK_FROM_NGPIOS(32)}, \
        .intbase = (void *)(&GPIO->INT##nid##EN), \
        .port_offset = 4 * nid, \
        .pin_offset = 32 * nid \
    }; \
    DEVICE_DT_DEFINE(DT_DRV_INST(nid),   \
        gpio_apollo3p_init,              \
        device_pm_control_nop,          \
        &apollo3p_gpio##nid##_private,                   \
        &apollo3p_gpio##nid##_config,           \
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
        &gpio_apollo3p_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_INIT)
