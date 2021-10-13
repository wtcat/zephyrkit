#include "watch_face_port.h"
#include "drivers/counter.h"
#include "posix/sys/time.h"
#include "sys/timeutil.h"
#include "time.h"
#include "watch_face_theme.h"
#include <posix/time.h>

struct tm posix_time_now;

void wf_port_time_update(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	time_t now = tv.tv_sec;
	struct tm *tm_now = gmtime(&now);

	posix_time_now = *tm_now;
}

static void wf_get_year(void *data)
{
	*(int *)data = posix_time_now.tm_year;
}

static void wf_get_month(void *data)
{
	*(int *)data = posix_time_now.tm_mon;
}

static void wf_get_mday(void *data)
{
	*(int *)data = posix_time_now.tm_mday;
}

static void wf_get_wday(void *data)
{
	*(int *)data = posix_time_now.tm_wday;
}
static void wf_get_hour(void *data)
{
	*(int *)data = posix_time_now.tm_hour;
}

static void wf_get_hour_tens(void *data)
{
	*(int *)data = posix_time_now.tm_hour / 10;
}
static void wf_get_hour_uinits(void *data)
{
	*(int *)data = posix_time_now.tm_hour % 10;
}

static void wf_get_min(void *data)
{
	*(int *)data = posix_time_now.tm_min;
}
static void wf_get_min_tens(void *data)
{
	*(int *)data = posix_time_now.tm_min / 10;
}
static void wf_get_min_uinits(void *data)
{
	*(int *)data = posix_time_now.tm_min % 10;
}
static void wf_get_sec(void *data)
{
	*(int *)data = posix_time_now.tm_sec;
}
static void wf_get_sec_tens(void *data)
{
	*(int *)data = posix_time_now.tm_sec / 10;
}
static void wf_get_sec_units(void *data)
{
	*(int *)data = posix_time_now.tm_sec % 10;
}

static void wf_get_batt_cap(void *data)
{
	static uint8_t batt_cap = 30;

	batt_cap += 1;
	if (batt_cap > 100) {
		batt_cap = 0;
	}

	*(uint8_t *)data = batt_cap;
}

static void wf_get_weather_type(void *data)
{
	*(uint8_t *)data = 9;
}

static void wf_get_heart_rate(void *data)
{
	static uint16_t heart_rate = 60;

	heart_rate++;

	if (heart_rate >= 100) {
		heart_rate = 60;
	}

	*(uint16_t *)data = heart_rate;
}

static void wf_get_temp(void *data)
{
	static int16_t temp = -10;
	temp++;
	if (temp >= 30) {
		temp = -10;
	}

	*(uint16_t *)data = temp;
}

static void wf_get_steps(void *data)
{
	static uint32_t steps = 1000;

	steps += 100;
	if (steps >= 12000) {
		steps = 0;
	}
	*(uint32_t *)data = steps;
}

static void wf_get_calories(void *data)
{
	static uint16_t calories = 100;

	calories += 10;
	if (calories >= 260) {
		calories = 0;
	}
	*(uint16_t *)data = calories;
}

static void wf_get_am_pm(void *data)
{
	if (posix_time_now.tm_hour >= 12) {
		*(uint8_t *)data = 1;
	} else {
		*(uint8_t *)data = 0;
	}
}

static wf_port_get_value func_array[ELEMENT_TYPE_MAX] = {NULL};

void wf_reg_data_func(wf_port_get_value func, element_type_enum type)
{
	func_array[type] = func;
}

wf_port_get_value wf_get_data_func(element_type_enum type)
{
	return func_array[type];
}

void wf_port_init(void)
{
	// time
	wf_reg_data_func(wf_get_year, ELEMENT_TYPE_YEAR);
	wf_reg_data_func(wf_get_month, ELEMENT_TYPE_MONTH);
	wf_reg_data_func(wf_get_mday, ELEMENT_TYPE_DAY);
	wf_reg_data_func(wf_get_wday, ELEMENT_TYPE_WEEK_DAY);
	wf_reg_data_func(wf_get_hour, ELEMENT_TYPE_HOUR);
	wf_reg_data_func(wf_get_hour_tens, ELEMENT_TYPE_HOUR_TENS);
	wf_reg_data_func(wf_get_hour_uinits, ELEMENT_TYPE_HOUR_UNITS);
	wf_reg_data_func(wf_get_min, ELEMENT_TYPE_MIN);
	wf_reg_data_func(wf_get_min_tens, ELEMENT_TYPE_MIN_TENS);
	wf_reg_data_func(wf_get_min_uinits, ELEMENT_TYPE_MIN_UNITS);
	wf_reg_data_func(wf_get_sec, ELEMENT_TYPE_SEC);
	wf_reg_data_func(wf_get_sec_tens, ELEMENT_TYPE_SEC_TENS);
	wf_reg_data_func(wf_get_sec_units, ELEMENT_TYPE_SEC_UNITS);
	wf_reg_data_func(wf_get_am_pm, ELEMENT_TYPE_AM_PM);

	// battery
	wf_reg_data_func(wf_get_batt_cap, ELEMENT_TYPE_BATTERY_CAPACITY);

	// weather
	wf_reg_data_func(wf_get_weather_type, ELEMENT_TYPE_WEATHER);

	// heartrate
	wf_reg_data_func(wf_get_heart_rate, ELEMENT_TYPE_HEART_RATE);

	// temperate
	wf_reg_data_func(wf_get_temp, ELEMENT_TYPE_TEMP);

	// steps
	wf_reg_data_func(wf_get_steps, ELEMENT_TYPE_STEPS);

	// calories
	wf_reg_data_func(wf_get_calories, ELEMENT_TYPE_CALORIE);
}