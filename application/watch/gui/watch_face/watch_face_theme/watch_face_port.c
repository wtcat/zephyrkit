#include "watch_face_port.h"
#include <stdint.h>

#include "drivers/counter.h"
#include "posix/sys/time.h"
#include "sys/timeutil.h"
#include "time.h"
#include <posix/time.h>

#define TIME_TEST 0

#if TIME_TEST
static uint8_t hour = 8;
static uint8_t min = 10;
static uint8_t sec = 20;

void time_update() // called by sec handler
{
	sec++;
	if (sec >= 60) {
		sec = 0;
		min++;
		if (min >= 60) {
			min = 0;
			hour++;
			if (hour >= 24) {
				hour = 0;
			}
		}
	}
}
#endif

struct tm posix_time_now;

void posix_time_update(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	time_t now = tv.tv_sec;
	struct tm *tm_now = gmtime(&now);

	posix_time_now = *tm_now;
}

uint8_t watch_face_get_hour(void)
{
#if TIME_TEST
	return hour;
#else
	return posix_time_now.tm_hour;
#endif
}

uint8_t watch_face_get_min(void)
{
#if TIME_TEST
	return min;
#else
	return posix_time_now.tm_min;
#endif
}

uint8_t watch_face_get_sec(void)
{
#if TIME_TEST
	time_update();
	return sec;
#else
	return posix_time_now.tm_sec;
#endif
}

uint8_t batt_cap = 30;
uint8_t watch_face_get_batt_cap(void)
{
	batt_cap += 1;
	if (batt_cap > 100) {
		batt_cap = 0;
	}
	return batt_cap;
}

uint8_t watch_face_get_month(void)
{
	return posix_time_now.tm_mon;
}

int watch_face_get_year(void)
{
	return posix_time_now.tm_year;
}

uint8_t watch_face_get_day(void)
{
	return posix_time_now.tm_mday;
}

uint8_t watch_face_get_weather_type(void)
{
	return 9;
}

static uint8_t heart_rate = 60;
uint8_t watch_face_get_heart_rate(void)
{
	heart_rate++;

	if (heart_rate >= 100) {
		heart_rate = 60;
	}

	return heart_rate;
}

static int16_t temp = -10;
int16_t watch_face_get_temp(void)
{
	temp++;

	if (temp >= 30) {
		temp = -10;
	}

	return temp;
}

uint8_t watch_face_get_week(void)
{
	return posix_time_now.tm_wday == 7 ? 0 : posix_time_now.tm_wday;
}

static uint32_t steps = 1000;
uint32_t watch_face_get_steps(void)
{
	steps += 100;
	if (steps >= 12000) {
		steps = 0;
	}
	return steps;
}

static uint16_t calories = 100;
uint16_t watch_face_get_calories(void)
{
	calories += 10;
	if (calories >= 260) {
		calories = 0;
	}
	return calories;
}

static uint8_t am_pm_type = 0;
uint8_t watch_face_get_am_pm_type(void)
{
	if (posix_time_now.tm_hour >= 12) {
		am_pm_type = 1;
	} else {
		am_pm_type = 0;
	}
	return am_pm_type;
}