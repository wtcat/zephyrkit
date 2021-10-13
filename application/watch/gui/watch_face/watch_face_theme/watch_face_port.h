#ifndef __WATCH_FACE_PORT_H
#define __WATCH_FACE_PORT_H
#include "stdint.h"

typedef enum {
	CLOCK_SKIN_WEATHER_SUNNY = 0,
	CLOCK_SKIN_WEATHER_CLOUDY,
	CLOCK_SKIN_WEATHER_OVERCAST,
	CLOCK_SKIN_WEATHER_FOG,
	CLOCK_SKIN_WEATHER_RAIN,
	CLOCK_SKIN_WEATHER_SHOWERS,
	CLOCK_SKIN_WEATHER_T_STORM,
	CLOCK_SKIN_WEATHER_RAINSTORM,
	CLOCK_SKIN_WEATHER_SNOW,
	CLOCK_SKIN_WEATHER_RAINSNOW,
	CLOCK_SKIN_WEATHER_HOT,
	CLOCK_SKIN_WEATHER_COLD,
	CLOCK_SKIN_WEATHER_WINDY,
	CLOCK_SKIN_WEATHER_SUNNY_NIGHT,
	CLOCK_SKIN_WEATHER_CLOUDY_NIGHT,
} weather_type_enum;

int watch_face_get_year(void);
uint8_t watch_face_get_month(void);

uint8_t watch_face_get_hour(void);

uint8_t watch_face_get_min(void);

uint8_t watch_face_get_sec(void);

uint8_t watch_face_get_batt_cap(void);

uint8_t watch_face_get_weather_type(void);

uint8_t watch_face_get_day(void);

uint8_t watch_face_get_heart_rate(void);

int16_t watch_face_get_temp(void);

uint8_t watch_face_get_week(void);

uint32_t watch_face_get_steps(void);

uint16_t watch_face_get_calories(void);

uint8_t watch_face_get_am_pm_type(void);

void posix_time_update(void);

#endif
