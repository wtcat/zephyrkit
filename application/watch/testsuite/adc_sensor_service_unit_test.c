#include <zephyr.h>
#include <init.h>
#include <drivers/adc.h>
#include <devicetree.h>

#include "base/schedule_service.h"

#define BUFFER_SIZE  6

struct test_private {
    int16_t buffer[BUFFER_SIZE];
};

#define ADC_DEVICE_NAME		DT_LABEL(DT_INST(0, ambiq_apollo3p_adc))
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_EXTERNAL0
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	3
#define INVALID_ADC_VALUE SHRT_MIN


static const struct adc_channel_cfg m_1st_channel_cfg = {
	.gain             = ADC_GAIN,
	.reference        = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id       = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive   = ADC_1ST_CHANNEL_INPUT,
#endif
};

static int test_service_init(struct schedule_service *ss)
{
    struct test_private *ts = ss->private;
	adc_channel_setup(ss->device, &m_1st_channel_cfg);
	for (int i = 0; i < BUFFER_SIZE; ++i)
		ts->buffer[i] = INVALID_ADC_VALUE;
    return 0;
}

static int test_service_exec(struct schedule_service *ss)
{
    struct test_private *ts = ss->private;
	struct adc_sequence sequence;
    int ret;

    printk("ADC current thread:%s\n", k_thread_name_get(k_current_get()));
    sequence.channels = BIT(ADC_1ST_CHANNEL_ID);
    sequence.buffer = ts->buffer;
    sequence.buffer_size = sizeof(ts->buffer);
    sequence.resolution = 12;
    sequence.oversampling = 3;
    sequence.options = 0;
    ret = adc_read(ss->device, &sequence);
    if (!ret)
        printk("[SYNC]ADC sampling value: %d\n", ts->buffer[0]);
    return 0;
}

ssize_t test_service_read(struct schedule_service *ss, void *buffer, size_t n)
{
    struct test_private *ts = ss->private;
    *(int *)buffer = (int)ts->buffer[0];
    return sizeof(int);
}

static struct test_private test_data;
static const struct schedule_operations test_operations = {
    .init = test_service_init,
    .exec = test_service_exec,
    .read = test_service_read
};

SCHED_SERVICE(adc_svr, ADC_DEVICE_NAME, &test_operations, &test_data);
