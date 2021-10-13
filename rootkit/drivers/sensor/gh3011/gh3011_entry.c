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

#include "zephyr.h"
#include <device.h>
#include <drivers/gpio.h>
#include <kernel.h>
#include <soc.h>
#include "gh3011_i2c.h"
#include "gpio/gpio_apollo3p.h"
#include "drivers_ext/sensor_priv.h"
#include "pah_platform_types.h"
#include "gh3011_example.h"
#include "math.h"


struct gh3011_data gh3011_data;
static struct k_sem gh3011_lock;
static K_THREAD_STACK_DEFINE(thread_stack, CONFIG_GH3011_THREAD_STACK_SIZE);

#if defined(CONFIG_MULTITHREADING)
/* semaphore for locking flash resources (tickers) */

#define SYNC_INIT() k_sem_init(&sem_lock, 0, 10)
#define SYNC_LOCK() k_sem_take(&sem_lock, K_FOREVER)
#define SYNC_UNLOCK() k_sem_give(&sem_lock)
#else
#define SYNC_INIT()
#define SYNC_LOCK()
#define SYNC_UNLOCK()
#endif
/**
 *  
 */
static const struct device *sensor_dev;
uint8_t motion_set_cal_resting(uint8_t has_motion);
static uint32_t diff_data = 150;
static void timer_motion(struct k_timer *timer)
{	
	static uint8_t count = 0;
	static int32_t pre_acc = 0;
	int32_t now_acc = 0;
	struct sensor_value accel[3];
	static uint8_t has_motion = false;
	count++;
	sensor_channel_get(sensor_dev, SENSOR_CHAN_ACCEL_XYZ, accel);
	accel[0].val1 >>= 2;
	accel[1].val1 >>= 2;
	accel[2].val1 >>= 2;
	now_acc = accel[0].val1 * accel[0].val1 + accel[1].val1 * accel[1].val1 + accel[2].val1 * accel[2].val1;	
	now_acc = sqrt(now_acc);
	int data = abs(now_acc - pre_acc);

	if ( data >= 30) {
		hal_gsensor_int1_init();	
		if(data > diff_data) {
			printf("&&&&&&&&&&&&&&&&&&&&&&&&&  abs(now_acc - pre_acc) > %d, has_motion\n", diff_data);
			has_motion = true;
		}
	}

	if (count == 33){
		motion_set_cal_resting(has_motion);
		has_motion = false;
		count = 0;
	}	
	pre_acc = now_acc;
}

void hal_gh30x_int_handler(void);
static void gh3011_thread(int dev_ptr, int unused)
{
	while (1) {
		k_sem_take(&gh3011_lock, K_FOREVER);
		hal_gh30x_int_handler();
	}
}

void call_hal_gh30x_int_handler(int irq, void *func)
{
	k_sem_give(&gh3011_lock);
}

#define GH3011_PIN(pin) (pin & 31)
#define CONFIG_GH3011_THREAD_PRIORITY 9
static int gh3011_probe(const struct device *dev)
{
	printk("gh3011_init!\r\n");
	struct gh3011_data *data = dev->data;	
	k_tid_t thr;
	//accelerometer_init();
	k_sem_init(&gh3011_lock, 0, 1);
	thr = k_thread_create(&data->thread, thread_stack,
			CONFIG_GH3011_THREAD_STACK_SIZE,
			(k_thread_entry_t)gh3011_thread, (void *)dev,
			0, NULL, K_PRIO_COOP(CONFIG_GH3011_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(thr, "/sensor@heart");

	k_usleep(10000);

	sensor_dev = device_get_binding("ICM42607");
	const struct device *gpio_dev = device_get_binding("GPIO_1");
	if (gpio_dev != NULL) {
		gpio_pin_configure(gpio_dev, GH3011_PIN(50),
			   GPIO_OUTPUT|GPIO_DS_DFLT_HIGH);
	}
	hal_gh30x_int_init();
	gpio_pin_set(gpio_dev, GH3011_PIN(50), 1);		
	gh30x_module_init();
	k_timer_init(&data->timer_motion, timer_motion, NULL);
	k_timer_start(&data->timer_motion, K_MSEC(2000), K_MSEC(30));
	return 0;
}

int gh3011_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
					 const struct sensor_value *val)
{
	if (chan == SENSOR_CHAN_HEART_RATE) {
		if (attr == SENSOR_ATTR_SENSOR_START_STOP) {
			if (val->val1 == 1) {		
				diff_data = 150;
				gh30x_module_start(RUN_MODE_ADT_HB_DET);
				printk("sensor heart rate start!\n");
			} else {
				diff_data = 1000;
				gh30x_module_stop();
				printk("sensor heart rate stop!\n");
			}
		}
	} else if (chan == SENSOR_CHAN_SPO2) {
		if (attr == SENSOR_ATTR_SENSOR_START_STOP) {
			if (val->val1 == 1) {
				diff_data = 100;
				gh30x_module_start(RUN_MODE_SPO2_DET);
				printk("sensor spo2 start!\n");
			} else {
				diff_data = 1000;
				gh30x_module_stop();
				printk("sensor spo2 stop!\n");
			}
		}
	}
	return 0;
}

int gh3011_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	struct gh3011_data *data = dev->data;
	if (chan == SENSOR_CHAN_HEART_RATE) {
		val->val1 = data->hr_data;
		//val->val2 = data->hr_data_rdy_flag;
	} else if (chan == SENSOR_CHAN_SPO2) {
		val->val1 = data->spo2_data;
		//val->val2 = data->spo2_data_rdy_flag;
	}
	return 0;
}
int gh3011_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
					 struct sensor_value *val)
{
	return 0;
}

static const struct sensor_driver_api gh3011_api_funcs = {
	.channel_get = gh3011_channel_get,
	.attr_set = gh3011_attr_set,
	.attr_get = gh3011_attr_get,
};
DEVICE_DEFINE(sensor_gh3011, "sensor_gh3011", &gh3011_probe, NULL, &gh3011_data, NULL, POST_KERNEL,
			  CONFIG_APPLICATION_INIT_PRIORITY, &gh3011_api_funcs);
