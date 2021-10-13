#include <drivers/adc.h>
#include <zephyr.h>
#include <devicetree.h>

#define ADC_DEVICE_NAME		DT_LABEL(DT_INST(0, ambiq_apollo3p_adc))
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_EXTERNAL0
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	3
//#define ADC_2ND_CHANNEL_ID	4

#define INVALID_ADC_VALUE SHRT_MIN

#define BUFFER_SIZE  6
static int16_t m_sample_buffer[BUFFER_SIZE];

static const struct adc_channel_cfg m_1st_channel_cfg = {
	.gain             = ADC_GAIN,
	.reference        = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id       = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive   = ADC_1ST_CHANNEL_INPUT,
#endif
};

#if defined(ADC_2ND_CHANNEL_ID)
static const struct adc_channel_cfg m_2nd_channel_cfg = {
	.gain             = ADC_GAIN,
	.reference        = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id       = ADC_2ND_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive   = ADC_2ND_CHANNEL_INPUT,
#endif
};
#endif

static const struct device *init_adc(void)
{
	const struct device *adc_dev = device_get_binding(ADC_DEVICE_NAME);
        if (adc_dev == NULL)
            return NULL;

	adc_channel_setup(adc_dev, &m_1st_channel_cfg);
#if defined(ADC_2ND_CHANNEL_ID)
	adc_channel_setup(adc_dev, &m_2nd_channel_cfg);
#endif /* defined(ADC_2ND_CHANNEL_ID) */

	for (int i = 0; i < BUFFER_SIZE; ++i)
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	return adc_dev;
}

#if defined(CONFIG_ADC_ASYNC)
static struct k_poll_signal async_sig;
#endif

int main(void)
{
	int ret;
	struct adc_sequence sequence = {
		.channels    = BIT(ADC_1ST_CHANNEL_ID),
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = 12,
		.oversampling = 3
	};
#if defined(CONFIG_ADC_ASYNC)
	struct k_poll_event  async_evt =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &async_sig);
#endif
					 
	const struct device *adc_dev = init_adc();
	
	for (; ;) {
		ret = adc_read(adc_dev, &sequence);
		if (ret == 0)
			printk("[SYNC]ADC sampling value: %d\n", m_sample_buffer[0]);
	    k_sleep(K_MSEC(1000));
#if defined(CONFIG_ADC_ASYNC)
		ret = adc_read_async(adc_dev, &sequence, &async_sig);
		if (ret == 0) {
			if (k_poll(&async_evt, 1, K_FOREVER))
				continue;
		    async_evt.poll_signal.signaled = 0;
	        async_evt.poll_signal.result = 0;
			printk("[ASYNC]ADC sampling value: %d\n", m_sample_buffer[0]);
		}
	    k_sleep(K_MSEC(1000));
#endif
	}
	return 0;
}
