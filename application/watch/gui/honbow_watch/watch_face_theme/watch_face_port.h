#ifndef __WATCH_FACE_PORT_H
#define __WATCH_FACE_PORT_H
#include "watch_face_theme.h"
#include <stdint.h>

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

typedef void (*wf_port_get_value)(void *data);
void wf_port_init(void);
void wf_port_time_update(void);
void wf_reg_data_func(wf_port_get_value func, element_type_enum type);
wf_port_get_value wf_get_data_func(element_type_enum type);
#endif
