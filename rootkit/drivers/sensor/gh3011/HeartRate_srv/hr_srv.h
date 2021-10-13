/**
 * @file
 * @brief File with example code presenting usage of drivers for PAH8series_8112
 *
 * @sa PAH8series_8112ES
 */
// gsensor
//#include "accelerometer.h"
#include "zephyr.h"

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
#include "gpio/gpio_apollo3p.h"
#include "drivers_ext/sensor_priv.h"
// #include "../gh3011_code/gh3011_example.h"
#include "math.h"
#include <posix/time.h>


typedef struct {
	uint8_t max_hr;
	uint8_t min_hr;
}HOUR_HR_T;

typedef struct {
	HOUR_HR_T hour_hr[24];
}AllDaysHr_T;

typedef struct {
	time_t pre_time;
	uint8_t pre_hr;
}PRE_HR_T;

typedef struct {
    uint32_t CfgType;
    uint32_t index;
	int iData[16];   
} GuiHRcfg_t;

enum hr_srv_channel_ext {
	CHAN_HEART_RATE = 0,
	CHAN_SPO2,
	CHAN_HEART_DATA,
	CHAN_SPO2_DATA,
	CHAN_CAL_RESTING,
};

bool watch_has_motion(void);
void send_gui_hr_msg_to_queue(GuiHRcfg_t *hr_cfg);
void Gui_get_24hour_hr(AllDaysHr_T *day_hr);
uint8_t Gui_get_hr(void);
uint8_t Gui_get_resting_hr(void);
uint8_t sport_get_hr(void);
uint8_t Gui_get_spo2(void);
uint8_t motion_set_cal_resting(uint8_t has_motion);
uint8_t Gui_close_hr(void);
uint8_t Gui_open_hr(void);
uint8_t Gui_open_spo2(void);
uint8_t Gui_close_spo2(void);
uint8_t put_hr_to_heartrate_srv(uint8_t sensor_hr);
uint8_t put_spo2_to_heartrate_srv(uint8_t sensor_spo2);
uint8_t Gui_get_pre_hr(PRE_HR_T *hr_data);
