/**
 * @file
 * @brief File with example code presenting usage of drivers for PAH8series_8112
 *
 * @sa PAH8series_8112ES
 */
// gsensor
//#include "accelerometer.h"


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
#include "hr_srv.h"

#define SENSOR_OPEN  1
#define SENSOR_CLOSE 0

#define HR_STACK  2048
static const struct device *hr_dev;
static AllDaysHr_T day_hr = {0};
static struct k_timer timer_24hour_hr;
static struct k_timer timer_per_sec;
uint8_t hr_status, set_hr_status;
uint8_t spo2_status, set_spo2_status;
static uint8_t G_Resting_hr = 0;
GuiHRcfg_t hr_cfg = {0};
static uint8_t gui_hr;
PRE_HR_T  pre_hr = {0};
uint8_t HR = 0;
uint8_t SPO2 = 0;
#define QUEUE_DEPTH 1
char gui_hr_cfg_buffer[QUEUE_DEPTH *sizeof(GuiHRcfg_t)];
struct k_msgq gui_hr_msgq;
GuiHRcfg_t hr_rx_cfg, hr_tx_cfg;
static bool motion_flag = false;

void send_gui_hr_msg_to_queue(GuiHRcfg_t *hr_cfg)
{
   // printf("send_gui_hr_msg_to_queue hr_cfg.type = %d\n", hr_cfg->CfgType);
    int ret;
    if(hr_cfg != NULL) {
        memcpy(&hr_tx_cfg, hr_cfg, sizeof(GuiHRcfg_t)); 
        ret = k_msgq_put(&gui_hr_msgq, &hr_tx_cfg, K_NO_WAIT);
        if (ret) {
            printf("GUI_hr_msg put error\n");
            k_msgq_purge(&gui_hr_msgq);
        }
    }
	// printf("send_gui_hr_msg_to_queue over\n");
}

bool watch_has_motion(void)
{
	return motion_flag;
}

uint8_t Gui_get_pre_hr(PRE_HR_T *hr_data) 
{
	if (hr_data != NULL) {
		memcpy(hr_data, &pre_hr, sizeof(PRE_HR_T));
	}
	return 0;
}

void Gui_get_24hour_hr(AllDaysHr_T *data)
{
	if (data != NULL) {
		memcpy(data, &day_hr, sizeof(AllDaysHr_T));
	}
}

uint8_t Gui_get_hr(void)
{
	gui_hr = HR;	
	if(gui_hr) {
		pre_hr.pre_hr = gui_hr;
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		pre_hr.pre_time = ts.tv_sec;		
	}
	return gui_hr;  
}

uint8_t Gui_get_resting_hr(void)
{
	return G_Resting_hr;
}

uint8_t sport_get_hr(void)
{
	return HR;
}

uint8_t Gui_get_spo2(void) 
{
	return SPO2;
}

uint8_t motion_set_cal_resting(uint8_t has_motion)
{	
	hr_cfg.CfgType = CHAN_CAL_RESTING;
	hr_cfg.iData[0] = has_motion;	
	motion_flag = has_motion;
	if (hr_status == SENSOR_OPEN) {
		send_gui_hr_msg_to_queue(&hr_cfg);
	}
	return 0;
}

uint8_t Gui_close_hr(void)
{
	hr_cfg.CfgType = CHAN_HEART_RATE;
	hr_cfg.iData[0] = 0;
	send_gui_hr_msg_to_queue(&hr_cfg);
	return 0;
}

uint8_t Gui_open_hr(void)
{
	hr_cfg.CfgType = CHAN_HEART_RATE;
	hr_cfg.iData[0] = 1;
	send_gui_hr_msg_to_queue(&hr_cfg);
	return 0;
}

uint8_t Gui_open_spo2(void)
{
	hr_cfg.CfgType = CHAN_SPO2;
	hr_cfg.iData[0] = 1;
	send_gui_hr_msg_to_queue(&hr_cfg);
	return 0;
}

uint8_t Gui_close_spo2(void)
{
	hr_cfg.CfgType = CHAN_SPO2;
	hr_cfg.iData[0] = 0;
	send_gui_hr_msg_to_queue(&hr_cfg);
	return 0;
}

uint8_t put_hr_to_heartrate_srv(uint8_t sensor_hr)
{	
	hr_cfg.CfgType = CHAN_HEART_DATA;
	hr_cfg.iData[0] = sensor_hr;
	send_gui_hr_msg_to_queue(&hr_cfg);
	return 0;
}

uint8_t put_spo2_to_heartrate_srv(uint8_t sensor_spo2)
{
	hr_cfg.CfgType = CHAN_SPO2_DATA;
	hr_cfg.iData[0] = sensor_spo2;
	send_gui_hr_msg_to_queue(&hr_cfg);
	return 0;
}

static struct sensor_value cfg;
uint8_t open_hr_sensor(void)
{
	if ((spo2_status == SENSOR_OPEN) || (hr_status == SENSOR_OPEN)) {
		return 1;
	}	
	cfg.val1 = SENSOR_OPEN;
	if (NULL != hr_dev) {
		sensor_attr_set(hr_dev, SENSOR_CHAN_HEART_RATE, SENSOR_ATTR_SENSOR_START_STOP, &cfg);
	} else {
		printk("open_hr_sensor hr_dev = NULL\n");
	}	
	hr_status = SENSOR_OPEN;
	return 0;
}

uint8_t close_hr_sensor(void)
{	
	if ((spo2_status == SENSOR_OPEN) || (hr_status == SENSOR_CLOSE)) {
		return 1;
	}
	cfg.val1 = SENSOR_CLOSE;
	if (NULL != hr_dev) {
		sensor_attr_set(hr_dev, SENSOR_CHAN_HEART_RATE, SENSOR_ATTR_SENSOR_START_STOP, &cfg);
	} else {
		printk("close_hr_sensor hr_dev = NULL\n");
	}
	hr_status = SENSOR_CLOSE;
	return 0;
}

uint8_t open_spo2_sensor(void)
{
	if ((hr_status == SENSOR_OPEN) || (spo2_status == SENSOR_OPEN)) {
		return 1;
	}
	cfg.val1 = SENSOR_OPEN;
	if (NULL != hr_dev) {
		sensor_attr_set(hr_dev, SENSOR_CHAN_SPO2, SENSOR_ATTR_SENSOR_START_STOP, &cfg);
	} else {
		printk("open_spo2_sensor hr_dev = NULL\n");
	}
	spo2_status = SENSOR_OPEN;
	return 0;
}

uint8_t close_spo2_sensor(void)
{	
	if ((spo2_status == SENSOR_CLOSE) || (hr_status == SENSOR_OPEN)) {
		return 1;
	}
	cfg.val1 = SENSOR_CLOSE;
	if (NULL != hr_dev) {
		sensor_attr_set(hr_dev, SENSOR_CHAN_SPO2, SENSOR_ATTR_SENSOR_START_STOP, &cfg);
	} else {
		printk("close_spo2_sensor hr_dev = NULL\n");
	}
	spo2_status = SENSOR_CLOSE;
	return 0;
}

static void start_24_hour_prc(struct k_timer *timer)
{
	k_timer_start(&timer_per_sec, K_SECONDS(0), K_SECONDS(1));
}

static void proc_24hour_hr(struct k_timer *timer)
{	
	static uint32_t no_data = 0;
	if ((SENSOR_CLOSE == hr_status) || (spo2_status == SENSOR_CLOSE)) {
		open_hr_sensor();
		if (0 == no_data) {
			cfg.val1 = SENSOR_OPEN;
			sensor_attr_set(hr_dev, SENSOR_CHAN_HEART_RATE, SENSOR_ATTR_SENSOR_START_STOP, &cfg);		
			no_data = 10;
		}
	} else {
		if (HR){
			struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			time_t now = ts.tv_sec;
			struct tm *tm_now = gmtime(&now);
			uint8_t hour = tm_now->tm_hour;
			if ((HR >= 40 ) && (HR > day_hr.hour_hr[hour].max_hr)) {				
				day_hr.hour_hr[hour].max_hr = HR;
			}		
			if ((HR >= 40 ) && (HR < day_hr.hour_hr[hour].min_hr)) {
				day_hr.hour_hr[hour].min_hr = HR;
			}			
			k_timer_stop(&timer_per_sec);
			no_data = 0;
		}else {
			no_data++;
			if (no_data > 15) {
				k_timer_stop(&timer_per_sec);
				no_data = 0;
			}
		}
	}	
}

void start_24hour_hr(void)
{
	k_timer_start(&timer_24hour_hr, K_SECONDS(1), K_SECONDS(60));
}

void stop_24hour_hr(void)
{
	k_timer_stop(&timer_24hour_hr);
}

int cal_resting_hr(uint8_t has_motion)
{
	static uint32_t count = 0;
	static uint32_t sum = 0;
	if(has_motion) {
		count = sum = 0;
		return 1;
	}
	if((hr_status == SENSOR_OPEN) && (HR >= 40)) {
		count++;
		sum += HR;
	} else {
		count = sum = 0;
	}
	if (count == 60){
		int resting = sum/60;		
		if ((resting < G_Resting_hr) || (0 == G_Resting_hr)){
			G_Resting_hr = resting;
		}
	}
	return 0;
}

void gui_hr_proc(GuiHRcfg_t *cfg)
{
	switch (cfg->CfgType) {
	case CHAN_HEART_RATE:
		if (cfg->iData[0] == 1) {
			open_hr_sensor();
			set_hr_status = 1;
		} else {
			close_hr_sensor();		
			HR = 0;
			set_hr_status = 0;
		}
	break;
	case CHAN_SPO2:
		if (cfg->iData[0] == 1) {
			open_spo2_sensor();
			set_spo2_status = 1; 
		} else {
			close_spo2_sensor();
			set_spo2_status = 0;
			SPO2 = 0;
		}
	break;	
	case CHAN_HEART_DATA:
		HR = cfg->iData[0];	
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		uint8_t hour = tm_now->tm_hour;
		if(!day_hr.hour_hr[hour].max_hr) {
			day_hr.hour_hr[hour].max_hr = HR;
		}
		
		if (!day_hr.hour_hr[hour].min_hr) {
			day_hr.hour_hr[hour].min_hr = HR;
		}

		if ((HR >= 40 ) && (HR > day_hr.hour_hr[hour].max_hr)) {	
				
			day_hr.hour_hr[hour].max_hr = HR;
		}		
		if ((HR >= 40 ) && (HR < day_hr.hour_hr[hour].min_hr)) {
			day_hr.hour_hr[hour].min_hr = HR;
		}				
	break;
	case CHAN_SPO2_DATA:
		SPO2 = cfg->iData[0];
	break;
	case CHAN_CAL_RESTING:
		cal_resting_hr(cfg->iData[0]);
		break;
	default:
		break;
	}
}

#define TEST 1
void hr_service_thread(void)
{
	hr_dev = device_get_binding("sensor_gh3011");
	printk("hr_dev = 0x%x\n", hr_dev);
	struct sensor_value  sensor_data;

	sensor_channel_get(hr_dev, SENSOR_CHAN_HEART_RATE, &sensor_data);
	k_msgq_init(&gui_hr_msgq, gui_hr_cfg_buffer, sizeof(GuiHRcfg_t), QUEUE_DEPTH);
	k_timer_init(&timer_24hour_hr, start_24_hour_prc, NULL);
	k_timer_init(&timer_per_sec, proc_24hour_hr, NULL);
	while (1) {
		k_msgq_get(&gui_hr_msgq, &hr_rx_cfg, K_FOREVER);
		gui_hr_proc(&hr_rx_cfg);
		// if (!k_msgq_get(&gui_hr_msgq, &hr_rx_cfg, K_FOREVER)) {
        //     gui_hr_proc(&hr_rx_cfg);
    	// }		
	}
}

K_THREAD_DEFINE(hr_service, 
			HR_STACK,
			hr_service_thread, 
			NULL, NULL, NULL,
			11,
			0, 
			100);
