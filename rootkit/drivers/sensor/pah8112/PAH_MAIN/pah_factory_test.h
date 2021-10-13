#ifndef __FACTORY_TEST_H__
#define __FACTORY_TEST_H__

#include <stdbool.h>
#include <stdint.h>
//V2.5_fac
#include "pah_driver_types.h"
//V2.5_fac
typedef enum
{
	factory_test_light_leak,
	factory_test_led_golden_only,
	factory_test_led_target_sample,
	factory_test_touch_value,
	factory_test_INT_high,
	factory_test_INT_low,
	factory_test_power_noise,
	factory_test_num,
} factory_test_e;
//V2.5_fac
void factory_test_mode(factory_test_e test_type,bool expo_en,uint8_t expo_ch_a, uint8_t expo_ch_b,uint8_t expo_c);
void factory_test_reset(void);
#endif
