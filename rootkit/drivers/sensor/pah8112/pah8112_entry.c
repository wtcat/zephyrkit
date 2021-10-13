/**
 * @file
 * @brief File with example code presenting usage of drivers for PAH8series_8112
 *
 * @sa PAH8series_8112ES
 */
// gsensor
//#include "accelerometer.h"

#include "system_clock.h"

// C library
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// pah8112_Driver
#include "pah8series_config.h"
#include "pah_8112_internal.h"
#include "pah_comm.h"
#include "pah_driver.h"
#include "pah_driver_types.h"

// pah8112_Function
#include "pah_hrd_function.h"
#include "pah_spo2_function.h"

#include "zephyr.h"
#include <device.h>
#include <drivers/gpio.h>
#include <kernel.h>
#include <soc.h>
#include "i2c.h"
#include "gpio/gpio_apollo3p.h"
#include "drivers_ext/sensor_priv.h"

extern void demo_ppg_dri_SPO2(void);
#define GPIO_8112_PIN(pin) (pin & 31)

struct pah8112_data pah8112_data;

#if defined(CONFIG_MULTITHREADING)
/* semaphore for locking flash resources (tickers) */
static struct k_sem sem_lock;
#define SYNC_INIT() k_sem_init(&sem_lock, 1, 1)
#define SYNC_LOCK() k_sem_take(&sem_lock, K_FOREVER)
#define SYNC_UNLOCK() k_sem_give(&sem_lock)
#else
#define SYNC_INIT()
#define SYNC_LOCK()
#define SYNC_UNLOCK()
#endif

/*============================================================================
STATIC VARIABLES
============================================================================*/
static volatile bool has_interrupt_button = false;
static volatile bool has_interrupt_pah = false;
static volatile uint64_t interrupt_pah_timestamp = 0;

// SpO2 rtc ex
static uint8_t spo2_polling_en = 0;
static uint8_t spo2_dri_en = 0;

static void pah8112_irq_handler(int pin, void *params)
{
	has_interrupt_pah = true;
	interrupt_pah_timestamp = get_tick_count();

// compiler_barrier();
#if 0
	if (pah8112_data.hrd_timer_status) {
	} else if (pah8112_data.spo2_timer_status) {
	}
#endif
}

static void hrd_timer(struct k_timer *timer)
{
	pah8series_ppg_dri_HRD_task(&has_interrupt_pah, &has_interrupt_button, &interrupt_pah_timestamp);
	hrd_alg_task();
}
static void spo2_timer(struct k_timer *timer)
{
	pah8series_read_task_spo2_dri(&has_interrupt_pah, &has_interrupt_button,
								  &interrupt_pah_timestamp); // only output raw data
	spo2_alg_task();
	spo2_algorithm_calculate_run();
}
/**
 *  PAH8series_8112 Sample code
 */
static int pah8112_probe(const struct device *dev)
{
	printk("pah8112_init!\r\n");

	struct pah8112_data *data = dev->data;

	const struct device *irq_gpio_dev = device_get_binding("GPIO_1");
	if (irq_gpio_dev != NULL) {
		gpio_apollo3p_request_irq(irq_gpio_dev, GPIO_8112_PIN(32), pah8112_irq_handler, NULL, GPIO_INT_TRIG_HIGH, true);
	} else {
		return -EINVAL;
	}

	/* Initialization of I2C */
	if (false == i2c_init()) {
		return -EINVAL;
	}

	SYNC_INIT();

	accelerometer_init();

	k_timer_init(&data->timer_hrd, hrd_timer, NULL);
	k_timer_init(&data->timer_spo2, spo2_timer, NULL);

	data->dev = dev;
	SYNC_LOCK();
	data->hrd_timer_status = 0;
	data->spo2_timer_status = 0;
	SYNC_UNLOCK();

	return 0;
}
int pah8112_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
					 const struct sensor_value *val)
{
	struct pah8112_data *data = dev->data;

	if (chan == SENSOR_CHAN_HEART_RATE) {
		if (attr == SENSOR_ATTR_SENSOR_START_STOP) {
			if (val->val1 == 1) {
				SYNC_LOCK();
				if ((data->hrd_timer_status != 0) || (data->spo2_timer_status != 0)) {
					SYNC_UNLOCK();
					return -EBUSY;
				}
				pah8series_ppg_dri_HRD_init();
				pah8series_ppg_HRD_start();
				k_timer_start(&data->timer_hrd, K_MSEC(500), K_MSEC(500));
				data->hrd_timer_status = 1;
				SYNC_UNLOCK();
				printk("sensor heart rate start!\n");
			} else {
				SYNC_LOCK();
				k_timer_status_sync(&data->timer_hrd);
				k_timer_stop(&data->timer_hrd);
				pah8series_ppg_HRD_stop();
				hrd_alg_task();
				data->hrd_timer_status = 0;
				SYNC_UNLOCK();
				printk("sensor heart rate stop!\n");
			}
		}
	} else if (chan == SENSOR_CHAN_SPO2) {
		if (attr == SENSOR_ATTR_SENSOR_START_STOP) {
			if (val->val1 == 1) {
				SYNC_LOCK();
				if ((data->hrd_timer_status != 0) || (data->spo2_timer_status != 0)) {
					SYNC_UNLOCK();
					return -EBUSY;
				}
				pah8series_ppg_dri_SPO2_init();
				pah8series_ppg_SPO2_start();
				k_timer_start(&data->timer_spo2, K_MSEC(500), K_MSEC(500));
				data->spo2_timer_status = 1;
				SYNC_UNLOCK();
				printk("sensor spo2 start!\n");
			} else {
				SYNC_LOCK();
				k_timer_status_sync(&data->timer_spo2);
				k_timer_stop(&data->timer_spo2);
				pah8series_ppg_SPO2_stop();
				spo2_alg_task();
				data->spo2_timer_status = 0;
				SYNC_UNLOCK();
				printk("sensor spo2 stop!\n");
			}
		}
	}
}

int pah8112_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	struct pah8112_data *data = dev->data;
	if (chan == SENSOR_CHAN_HEART_RATE) {
		val->val1 = data->hr_data;
		val->val2 = data->hr_data_rdy_flag;
	} else if (chan == SENSOR_CHAN_SPO2) {
		val->val1 = data->spo2_data;
		val->val2 = data->spo2_data_rdy_flag;
	}
}
int pah8112_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
					 struct sensor_value *val)
{
	struct pah8112_data *data = dev->data;
	if (chan == SENSOR_CHAN_HEART_RATE) {
		if (attr == SENSOR_ATTR_SENSOR_START_STOP) {
			SYNC_LOCK();
			val->val1 = data->hrd_timer_status;
			SYNC_UNLOCK();
		}
	} else if (chan == SENSOR_CHAN_SPO2) {
		if (attr == SENSOR_ATTR_SENSOR_START_STOP) {
			SYNC_LOCK();
			val->val1 = data->spo2_timer_status;
			SYNC_UNLOCK();
		}
	}
}
static const struct sensor_driver_api pah8112_api_funcs = {
	.channel_get = pah8112_channel_get,
	.attr_set = pah8112_attr_set,
	.attr_get = pah8112_attr_get,
};
DEVICE_DEFINE(sensor_pah8112, "sensor_pah8112", &pah8112_probe, NULL, &pah8112_data, NULL, POST_KERNEL,
			  CONFIG_APPLICATION_INIT_PRIORITY, &pah8112_api_funcs);
