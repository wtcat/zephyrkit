#define DT_DRV_COMPAT ambiq_apollo3p_adc

#include <errno.h>
#include <drivers/adc.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <soc.h>

#include <devicetree/io-channels.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc/adc_context.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_apollo);


#define MAX_SLOTS AM_HAL_ADC_MAX_SLOTS


struct slot_config {
    uint8_t channel;
};

struct apollo_adc_cfg {
    uint8_t reserved;
};

struct sampling_param {
    uint32_t channels; 
    uint32_t resolution;
    uint32_t oversampling;
};

struct apollo_adc_data {
	struct adc_context ctx;
    struct sampling_param param;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
    uint32_t channels;
    void *handle;
    uint16_t devno;
    struct slot_config slots[MAX_SLOTS];
};


static const uint8_t adc_channels[] = {
    AM_HAL_ADC_SLOT_CHSEL_SE0,
    AM_HAL_ADC_SLOT_CHSEL_SE1,
    AM_HAL_ADC_SLOT_CHSEL_SE2,
    AM_HAL_ADC_SLOT_CHSEL_SE3,
    AM_HAL_ADC_SLOT_CHSEL_SE4,
    AM_HAL_ADC_SLOT_CHSEL_SE5,
    AM_HAL_ADC_SLOT_CHSEL_SE6,
    AM_HAL_ADC_SLOT_CHSEL_SE7,
    AM_HAL_ADC_SLOT_CHSEL_SE8,
    AM_HAL_ADC_SLOT_CHSEL_SE9
};

static const uint8_t adc_oversampling_rate[] = {
    AM_HAL_ADC_SLOT_AVG_1,
    AM_HAL_ADC_SLOT_AVG_2,
    AM_HAL_ADC_SLOT_AVG_4,
    AM_HAL_ADC_SLOT_AVG_8,
    AM_HAL_ADC_SLOT_AVG_16,
    AM_HAL_ADC_SLOT_AVG_32,
    AM_HAL_ADC_SLOT_AVG_64,
    AM_HAL_ADC_SLOT_AVG_128
};


static const uint8_t adc_diff_channels[] = {
    AM_HAL_ADC_SLOT_CHSEL_DF0,
    AM_HAL_ADC_SLOT_CHSEL_DF1
};


static void adc_apollo_setup_pins(void)
{
#define ADC_CHANNEL(index) \
    DT_IO_CHANNELS_INPUT_BY_NAME(DT_NODELABEL(channel), adcse##index)
    am_hal_gpio_pincfg_t adc_ioconf = {0};

    if (IS_ENABLED(CONFIG_ADC_CH0))
        am_hal_gpio_pinconfig(16, adc_ioconf);
    if (IS_ENABLED(CONFIG_ADC_CH1))
        am_hal_gpio_pinconfig(29, adc_ioconf);
    if (IS_ENABLED(CONFIG_ADC_CH2))
        am_hal_gpio_pinconfig(11, adc_ioconf);
    if (IS_ENABLED(CONFIG_ADC_CH3))
        am_hal_gpio_pinconfig(31, adc_ioconf);
    if (IS_ENABLED(CONFIG_ADC_CH4))
        am_hal_gpio_pinconfig(32, adc_ioconf);
    if (IS_ENABLED(CONFIG_ADC_CH5))
        am_hal_gpio_pinconfig(33, adc_ioconf);
    if (IS_ENABLED(CONFIG_ADC_CH6))
        am_hal_gpio_pinconfig(34, adc_ioconf);
    if (IS_ENABLED(CONFIG_ADC_CH7))
        am_hal_gpio_pinconfig(35, adc_ioconf);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
        bool repeat_sampling)
{
	struct apollo_adc_data *data =
		CONTAINER_OF(ctx, struct apollo_adc_data, ctx);
	if (repeat_sampling)
		data->buffer = data->repeat_buffer;
}

static inline void adc_start_convert(int devno)
{
    /* Write to the Software trigger register in the ADC */
    ADCn(devno)->SWT = 0x37;    
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct apollo_adc_data *data = 
        CONTAINER_OF(ctx, struct apollo_adc_data, ctx);
	data->repeat_buffer = data->buffer;
    adc_start_convert(data->devno);
}

static void adc_apollo_channel_disable_all(struct apollo_adc_data *data)
{
    uint32_t base = (uint32_t)&ADCn(data->devno)->SL0CFG;
    AM_REGVAL(base + (0 * 4)) = 0;
#if (MAX_SLOTS > 1)
    AM_REGVAL(base + (1 * 4)) = 0;
#endif
#if (MAX_SLOTS > 2)
    AM_REGVAL(base + (2 * 4)) = 0;
#endif
#if (MAX_SLOTS > 3)
    AM_REGVAL(base + (3 * 4)) = 0;
#endif
#if (MAX_SLOTS > 4)
    AM_REGVAL(base + (4 * 4)) = 0;
#endif
#if (MAX_SLOTS > 5)
    AM_REGVAL(base + (5 * 4)) = 0;
#endif
#if (MAX_SLOTS > 6)
    AM_REGVAL(base + (6 * 4)) = 0;
#endif
#if (MAX_SLOTS > 7)
    AM_REGVAL(base + (7 * 4)) = 0;
#endif
}

static void adc_apollo_channel_enable(struct apollo_adc_data *data,
    uint32_t channel_no, uint32_t resolution, uint32_t oversampling)
{
    uint32_t base = (uint32_t)&ADCn(data->devno)->SL0CFG;
    uint32_t chsel = data->slots[channel_no].channel;
    uint32_t cr = 0;

    cr |= _VAL2FLD(ADC_SL0CFG_ADSEL0, oversampling);
    cr |= _VAL2FLD(ADC_SL0CFG_PRMODE0, resolution);
    cr |= _VAL2FLD(ADC_SL0CFG_CHSEL0, chsel);
    cr |= _VAL2FLD(ADC_SL0CFG_WCEN0, 0);
    cr |= _VAL2FLD(ADC_SL0CFG_SLEN0, 1);
    AM_REGVAL(base + (channel_no << 2)) = cr;
}

static void adc_apollo_isr(const struct device *dev)
{
    struct apollo_adc_data *data = dev->data;
    uint32_t devno = data->devno;
    uint32_t status = ADCn(devno)->INTSTAT;
    uint32_t value;

    ADCn(devno)->INTCLR = status;
    if (status & AM_HAL_ADC_INT_SCNCMP) {
        if (status & AM_HAL_ADC_INT_CNVCMP) {
            do {
                value = ADCn(devno)->FIFOPR;
                if (!AM_HAL_ADC_FIFO_COUNT(value))
                    break;
                *data->buffer++ = AM_HAL_ADC_FIFO_SAMPLE(value);
                data->channels &= ~(1u << AM_HAL_ADC_FIFO_SLOT(value));
            } while (true);
        }

        if (data->channels)
            adc_start_convert(devno);
        else
            adc_context_on_sampling_done(&data->ctx, dev);
    }
    LOG_DBG("ISR triggered.");
}

static int check_buffer_size(const struct adc_sequence *sequence,
			     uint8_t active_channels)
{
	size_t needed_bsize = active_channels * sizeof(uint16_t);
	if (sequence->options)
		needed_bsize *= (1 + sequence->options->extra_samplings);
	if (sequence->buffer_size < needed_bsize) {
		LOG_ERR("%s(): Provided buffer is too small (%u/%u)", __func__,
				sequence->buffer_size, needed_bsize);
		return -ENOMEM;
	}
	return 0;
}

static int start_read(const struct device *dev,
        const struct adc_sequence *sequence)
{
    struct apollo_adc_data *data = dev->data;
    struct sampling_param *param = &data->param;
    uint32_t channels = sequence->channels;
    uint32_t resolution, avg;
    uint32_t actived, no;
    int ret;

    if (channels == param->channels &&
        sequence->resolution == param->resolution &&
        sequence->oversampling == param->oversampling)
        goto _start_convert;
    
    switch (sequence->resolution) {
    case 8:
        resolution = AM_HAL_ADC_SLOT_8BIT;
        break;
    case 10:
        resolution = AM_HAL_ADC_SLOT_10BIT;
        break;
    case 12:
        resolution = AM_HAL_ADC_SLOT_12BIT;
        break;
    case 14:
        resolution = AM_HAL_ADC_SLOT_14BIT;
        break;
    default:
        LOG_ERR("%s(): Unsupported resolution value %d\n", __func__, 
            sequence->resolution);
        return -EINVAL;
    }

    if (sequence->oversampling > 7) {
        LOG_ERR("%s(): Unsupported oversampling value %d\n", __func__,
            sequence->oversampling);
        return -EINVAL;
    }
    
    avg = adc_oversampling_rate[sequence->oversampling];
    adc_apollo_channel_disable_all(data);
    for (no = 0, actived = 0; channels; no++) {
        if (channels & 1) {
            adc_apollo_channel_enable(data, no, resolution, avg);
            actived++;
        }
        channels >>= 1;
    }
    
    ret = check_buffer_size(sequence, actived);
    if (ret)
        goto _exit;

    param->channels = sequence->channels;
    param->resolution = sequence->resolution;
    param->oversampling = sequence->oversampling;
    
_start_convert:     
    data->buffer = sequence->buffer;
    data->channels = sequence->channels;
	adc_context_start_read(&data->ctx, sequence);
    ret = adc_context_wait_for_completion(&data->ctx);
_exit:
    return ret;
}

static int adc_apollo_read(const struct device *dev,
        const struct adc_sequence *sequence)
{
    struct apollo_adc_data *data = dev->data;
    int error;

    adc_context_lock(&data->ctx, false, NULL);
    error = start_read(dev, sequence);
    adc_context_release(&data->ctx, error);
    return error;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_apollo_read_async(const struct device *dev,
           const struct adc_sequence *sequence,
           struct k_poll_signal *async)
{
    struct apollo_adc_data *data = dev->data;
    int error;

    adc_context_lock(&data->ctx, true, async);
    error = start_read(dev, sequence);
    adc_context_release(&data->ctx, error);
    return error;
}
#endif

static int adc_apollo_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
    struct apollo_adc_data *data = dev->data;
    uint8_t channel_id = channel_cfg->channel_id;
    if (!channel_cfg->differential) {
        if (channel_id >= MAX_SLOTS) {
            LOG_ERR("%s(): ADC single-end channel-id is too large\n", 
                __func__);
            return -EINVAL;
        }
        data->slots[channel_id].channel = adc_channels[channel_id];
    } else {
        if (channel_id >= 2) {
            LOG_ERR("%s(): ADC differential channel-id is too large\n", 
                __func__);
            return -EINVAL;
        }
        data->slots[channel_id].channel = adc_diff_channels[channel_id];
    }
    return 0;
}

static int adc_apollo_init(const struct device *dev)
{
    struct apollo_adc_data *data = dev->data;
    am_hal_adc_config_t adc_cfg;

    if (am_hal_adc_initialize(data->devno, &data->handle)) {
        LOG_ERR("%s(): ADC initialize failed(module: %d)\n", 
            __func__, data->devno);
        return -EINVAL;
    }

    if (am_hal_adc_power_control(data->handle, AM_HAL_SYSCTRL_WAKE, false)) {
        LOG_ERR("%s(): ADC power on\n", __func__);
        return -EINVAL;
    }

    /* Set up the ADC configuration parameters */
    adc_cfg.eClock             = AM_HAL_ADC_CLKSEL_HFRC;
    adc_cfg.ePolarity          = AM_HAL_ADC_TRIGPOL_RISING;
    adc_cfg.eTrigger           = AM_HAL_ADC_TRIGSEL_SOFTWARE;
    adc_cfg.eReference         = AM_HAL_ADC_REFSEL_INT_1P5;
    adc_cfg.eClockMode         = AM_HAL_ADC_CLKMODE_LOW_POWER;
    adc_cfg.ePowerMode         = AM_HAL_ADC_LPMODE0;
    adc_cfg.eRepeat            = AM_HAL_ADC_SINGLE_SCAN;
    if (am_hal_adc_configure(data->handle, &adc_cfg)) {
        LOG_ERR("%s(): Configuring ADC failed\n", __func__);
        return -EIO;
    }

    adc_apollo_setup_pins();
    am_hal_adc_interrupt_clear(data->handle, 0xFFFFFFFF);
    am_hal_adc_interrupt_disable(data->handle, 0xFFFFFFFF);
    am_hal_adc_interrupt_enable(data->handle, AM_HAL_ADC_INT_SCNCMP);
    irq_connect_dynamic(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
        (void(*)(const void *))adc_apollo_isr, dev, 0);
    irq_enable(DT_INST_IRQN(0));
    am_hal_adc_enable(data->handle);
    adc_context_unlock_unconditionally(&data->ctx);
    return 0;
}

static const struct adc_driver_api api_apollo_driver_api = {
    .channel_setup = adc_apollo_channel_setup,
    .read = adc_apollo_read,
#ifdef CONFIG_ADC_ASYNC
    .read_async = adc_apollo_read_async,
#endif
    .ref_internal = 1500
};

#define APOLLO_ADC_INIT(index) \
static const struct apollo_adc_cfg adc_apollo_cfg_##index = {};	\
static struct apollo_adc_data adc_apollo_data_##index = {		\
	ADC_CONTEXT_INIT_TIMER(adc_apollo_data_##index, ctx),	\
	ADC_CONTEXT_INIT_LOCK(adc_apollo_data_##index, ctx),		\
	ADC_CONTEXT_INIT_SYNC(adc_apollo_data_##index, ctx),		\
	.devno = index                                                \
};									\
DEVICE_DEFINE(adc_##index, DT_INST_LABEL(index),		\
		    &adc_apollo_init,					\
            NULL,                   \
		    &adc_apollo_data_##index, &adc_apollo_cfg_##index,	\
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
		    &api_apollo_driver_api);

DT_INST_FOREACH_STATUS_OKAY(APOLLO_ADC_INIT)
