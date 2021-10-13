/*==============================================================================
* Edit History
*
* This section contains comments describing changes made to the module. Notice
* that changes are listed in reverse chronological order. Please use ISO format
* for dates.
*
* when       who       what, where, why
* ---------- ---       -----------------------------------------------------------
* 2017-01-10 jc        - - Modify  light_leak_test Read Data to FIFO Mode ..
* 2016-11-21 jc        - - Combine 8112ES/ET PC /ET DI Setting..
* 2016-09-26 bh        - Initial revision.
==============================================================================*/

#include "pah_verify.h"

#include "pah_verify_8112_reg.h"
#include "pah_verify_8112_option.h"
#include "pah_8112_internal.h"

// pah
#include "pah_comm.h"

// platform support
#include "pah_platform_functions.h"
//#include "system_clock.h"

//============================================================================
// PRIVATE FUNCTION DECLARATIONS
//============================================================================
static pah_ret _pah8112_start_tg(void);
static pah_ret _pah8112_read_adc_data(pah_verify_adc_data_s *adc_data);
static pah_ret _pah8112_write_settings_and_read_adc_data(const uint8_t reg_array[][2], uint32_t length,
														 pah_verify_adc_data_s *adc_data,
														 pah_verify_expo_time_s *expo_time);
static pah_ret _pah8112_read_adc_data_fifo(pah_verify_adc_data_s *adc_data);
static pah_ret _pah8112_write_settings_and_read_adc_data_fifo(const uint8_t reg_array[][2], uint32_t length,
															  pah_verify_adc_data_s *adc_data,
															  pah_verify_expo_time_s *expo_time);

// static pah_ret _pah8112_write_settings_int(const uint8_t reg_array[][2], uint32_t length);
// static pah_ret _pah8112_read_adc_data_fifo_int(pah_verify_adc_data_s *adc_data);

static pah_ret _pah8112_expo_update(pah_verify_expo_time_s *expo_time);

pah_verify_expo_time_s expo_time_data_null;
//============================================================================
// PUBLIC FUNCTION DEFINITIONS
//============================================================================
pah_ret pah_global_reset(void)
{
	pah_ret ret = pah_err_unknown;

	//
	if (!pah_comm_write(0x7F, 0x04)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_write(0x69, 0x00)) // PowerDn_EnH Disable
	{
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_write(0x64, 0x00)) // Global Reset
	{
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	// wait for clk
	delay_ms(10);
	if (!pah_comm_write(0x64, 0x01)) // Global Reset
	{
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	delay_ms(10);

	return pah_success;

FAIL:
	debug_printf("_pah8011_start_tg() failed. ret = %d \n", ret);
	return ret;
}

pah_ret pah_verify_init(void)
{
	pah_ret ret = pah_err_unknown;

	debug_printf("pah_verify_init() \n");

	expo_time_data_null.expo_time_enabled_verify = false;
	expo_time_data_null.ch_a_expo_time = 0; // 8112ES Update
	expo_time_data_null.ch_b_expo_time = 0;
	expo_time_data_null.ch_c_expo_time = 0;

	ret = pah_8112_verify_product_id();
	if (PAH_FAILED(ret)) {
		ret = pah_8112_startup();
		if (PAH_FAILED(ret))
			goto FAIL;
		ret = pah_8112_verify_product_id();
		if (PAH_FAILED(ret))
			goto FAIL;
	}

	ret = pah_8112_device_init();
	if (PAH_FAILED(ret))
		goto FAIL;

	return pah_success;

FAIL:
	debug_printf("pah_verify_init() failed. ret = %d \n", ret);
	return ret;
}

uint32_t pah_verify_get_i2c_slave_addr(void)
{
	return PAH_I2C_SLAVE_ID;
}

pah_ret pah_verify_light_leak_read_adc_data(pah_verify_adc_data_s *adc_data)
{
	pah_ret ret = pah_err_unknown;

	debug_printf("pah_verify_light_leak_read_adc_data() \n");

	if (!adc_data) {
		ret = pah_err_invalid_argument;
		goto FAIL;
	}

	memset(adc_data, 0, sizeof(*adc_data));
	ret = _pah8112_write_settings_and_read_adc_data_fifo(pah8112_verify_light_leak_array,
														 PAH_ARRAY_SIZE(pah8112_verify_light_leak_array), adc_data,
														 &expo_time_data_null);

	if (PAH_FAILED(ret))
		goto FAIL;

		// t
#if defined(PAH_LIGHT_LEAK_CH_T_MIN)
	adc_data->ch_adc_data[pah_verify_ch_t].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_t].min = PAH_LIGHT_LEAK_CH_T_MIN;
#endif
#if defined(PAH_LIGHT_LEAK_CH_T_MAX)
	adc_data->ch_adc_data[pah_verify_ch_t].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_t].max = PAH_LIGHT_LEAK_CH_T_MAX;
#endif

	// a
#if defined(PAH_LIGHT_LEAK_CH_A_MIN)
	adc_data->ch_adc_data[pah_verify_ch_a].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_a].min = PAH_LIGHT_LEAK_CH_A_MIN;
#endif
#if defined(PAH_LIGHT_LEAK_CH_A_MAX)
	adc_data->ch_adc_data[pah_verify_ch_a].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_a].max = PAH_LIGHT_LEAK_CH_A_MAX;
#endif

	// b
#if defined(PAH_LIGHT_LEAK_CH_B_MIN)
	adc_data->ch_adc_data[pah_verify_ch_b].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_b].min = PAH_LIGHT_LEAK_CH_B_MIN;
#endif
#if defined(PAH_LIGHT_LEAK_CH_B_MAX)
	adc_data->ch_adc_data[pah_verify_ch_b].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_b].max = PAH_LIGHT_LEAK_CH_B_MAX;
#endif

	// c
#if defined(PAH_LIGHT_LEAK_CH_C_MIN)
	adc_data->ch_adc_data[pah_verify_ch_c].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_c].min = PAH_LIGHT_LEAK_CH_C_MIN;
#endif
#if defined(PAH_LIGHT_LEAK_CH_C_MAX)
	adc_data->ch_adc_data[pah_verify_ch_c].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_c].max = PAH_LIGHT_LEAK_CH_C_MAX;
#endif

	return pah_success;

FAIL:
	debug_printf("pah_verify_light_leak_read_adc_data() failed. ret = %d \n", ret);
	return ret;
}

pah_ret pah_verify_touch_value_read_adc_data(pah_verify_adc_data_s *adc_data)
{
	pah_ret ret = pah_err_unknown;

	//    debug_printf("pah_verify_touch_value_read_adc_data() \n");

	if (!adc_data) {
		ret = pah_err_invalid_argument;
		goto FAIL;
	}

	memset(adc_data, 0, sizeof(*adc_data));
	ret = _pah8112_write_settings_and_read_adc_data_fifo(pah8112_verify_light_leak_array,
														 PAH_ARRAY_SIZE(pah8112_verify_light_leak_array), adc_data,
														 &expo_time_data_null);

	if (PAH_FAILED(ret))
		goto FAIL;

		// t
#if defined(PAH_TOUCH_VALUE_CH_T_MIN)
	adc_data->ch_adc_data[pah_verify_ch_t].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_t].min = PAH_TOUCH_VALUE_CH_T_MIN;
#endif

#if defined(PAH_TOUCH_VALUE_CH_T_MAX)
	adc_data->ch_adc_data[pah_verify_ch_t].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_t].max = PAH_TOUCH_VALUE_CH_T_MAX;
#endif

	return pah_success;

FAIL:
	debug_printf("pah_verify_touch_value_read_adc_data() failed. ret = %d \n", ret);
	return ret;
}

pah_ret pah_verify_INT_high(void)
{
	pah_ret ret = pah_err_unknown;

	ret = pah_8112_startup();
	if (PAH_FAILED(ret))
		goto FAIL;

	// write settings
	ret =
		pah_8112_write_reg_array(pah8112_verify_int_pin_high_array, PAH_ARRAY_SIZE(pah8112_verify_int_pin_high_array));
	if (PAH_FAILED(ret))
		goto FAIL;

	return pah_success;

FAIL:
	debug_printf("pah_verify_INT_high failed. ret = %d \n", ret);
	return ret;
}

pah_ret pah_verify_INT_low(void)
{
	pah_ret ret = pah_err_unknown;

	ret = pah_8112_startup();
	if (PAH_FAILED(ret))
		goto FAIL;

	// write settings
	ret = pah_8112_write_reg_array(pah8112_verify_int_pin_low_array, PAH_ARRAY_SIZE(pah8112_verify_int_pin_low_array));
	if (PAH_FAILED(ret))
		goto FAIL;

	return pah_success;

FAIL:
	debug_printf("pah_verify_INT_low failed. ret = %d \n", ret);
	return ret;
}
// V2.5
pah_ret pah_verify_power_noise(void)
{
	pah_ret ret = pah_err_unknown;

	ret = pah_8112_startup();
	if (PAH_FAILED(ret))
		goto FAIL;

	// write settings
	ret = pah_8112_write_reg_array(pah8112_verify_led_expoT_DAC_MAX, PAH_ARRAY_SIZE(pah8112_verify_led_expoT_DAC_MAX));
	if (PAH_FAILED(ret))
		goto FAIL;

	return pah_success;

FAIL:
	debug_printf("pah_verify_power_noise failed. ret = %d \n", ret);
	return ret;
}

pah_ret pah_verify_led_golden_only_read_adc_data(pah_verify_adc_data_s *adc_data,
												 pah_verify_expo_time_s *expo_time_data)
{
	pah_ret ret = pah_err_unknown;

	debug_printf("pah_verify_led_golden_only_read_adc_data() \n");

	if (!adc_data) {
		ret = pah_err_invalid_argument;
		goto FAIL;
	}

	memset(adc_data, 0, sizeof(*adc_data));

	ret = _pah8112_write_settings_and_read_adc_data(pah8112_verify_led_brightness_array,
													PAH_ARRAY_SIZE(pah8112_verify_led_brightness_array), adc_data,
													expo_time_data);
	if (PAH_FAILED(ret))
		goto FAIL;

		// a
#if defined(PAH_LED_GOLDEN_ONLY_CH_A_MIN)
	adc_data->ch_adc_data[pah_verify_ch_a].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_a].min = PAH_LED_GOLDEN_ONLY_CH_A_MIN;
#endif
#if defined(PAH_LED_GOLDEN_ONLY_CH_A_MAX)
	adc_data->ch_adc_data[pah_verify_ch_a].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_a].max = PAH_LED_GOLDEN_ONLY_CH_A_MAX;
#endif

	// b
#if defined(PAH_LED_GOLDEN_ONLY_CH_B_MIN)
	adc_data->ch_adc_data[pah_verify_ch_b].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_b].min = PAH_LED_GOLDEN_ONLY_CH_B_MIN;
#endif
#if defined(PAH_LED_GOLDEN_ONLY_CH_B_MAX)
	adc_data->ch_adc_data[pah_verify_ch_b].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_b].max = PAH_LED_GOLDEN_ONLY_CH_B_MAX;
#endif

	// c
#if defined(PAH_LED_GOLDEN_ONLY_CH_C_MIN)
	adc_data->ch_adc_data[pah_verify_ch_c].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_c].min = PAH_LED_GOLDEN_ONLY_CH_C_MIN;
#endif
#if defined(PAH_LED_GOLDEN_ONLY_CH_C_MAX)
	adc_data->ch_adc_data[pah_verify_ch_c].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_c].max = PAH_LED_GOLDEN_ONLY_CH_C_MAX;
#endif

	// t
#if defined(PAH_LED_GOLDEN_ONLY_CH_T_MIN)
	adc_data->ch_adc_data[pah_verify_ch_t].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_t].min = PAH_LED_GOLDEN_ONLY_CH_T_MIN;
#endif
#if defined(PAH_LED_GOLDEN_ONLY_CH_T_MAX)
	adc_data->ch_adc_data[pah_verify_ch_t].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_t].max = PAH_LED_GOLDEN_ONLY_CH_T_MAX;
#endif
	return pah_success;

FAIL:
	debug_printf("pah_verify_led_golden_only_read_adc_data() failed. ret = %d \n", ret);
	return ret;
}

pah_ret pah_verify_led_target_sample_read_adc_data(pah_verify_adc_data_s *adc_data)
{
	pah_ret ret = pah_err_unknown;

	debug_printf("pah_verify_led_target_sample_read_adc_data() \n");

	if (!adc_data) {
		ret = pah_err_invalid_argument;
		goto FAIL;
	}

	memset(adc_data, 0, sizeof(*adc_data));

	ret = _pah8112_write_settings_and_read_adc_data(pah8112_verify_led_brightness_array,
													PAH_ARRAY_SIZE(pah8112_verify_led_brightness_array), adc_data,
													&expo_time_data_null);
	if (PAH_FAILED(ret))
		goto FAIL;

		// a
#if defined(PAH_LED_TARGET_SAMPLE_CH_A_MIN)
	adc_data->ch_adc_data[pah_verify_ch_a].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_a].min = PAH_LED_TARGET_SAMPLE_CH_A_MIN;
#endif
#if defined(PAH_LED_TARGET_SAMPLE_CH_A_MAX)
	adc_data->ch_adc_data[pah_verify_ch_a].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_a].max = PAH_LED_TARGET_SAMPLE_CH_A_MAX;
#endif

	// b
#if defined(PAH_LED_TARGET_SAMPLE_CH_B_MIN)
	adc_data->ch_adc_data[pah_verify_ch_b].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_b].min = PAH_LED_TARGET_SAMPLE_CH_B_MIN;
#endif
#if defined(PAH_LED_TARGET_SAMPLE_CH_B_MAX)
	adc_data->ch_adc_data[pah_verify_ch_b].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_b].max = PAH_LED_TARGET_SAMPLE_CH_B_MAX;
#endif

	// c
#if defined(PAH_LED_TARGET_SAMPLE_CH_C_MIN)
	adc_data->ch_adc_data[pah_verify_ch_c].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_c].min = PAH_LED_TARGET_SAMPLE_CH_C_MIN;
#endif
#if defined(PAH_LED_TARGET_SAMPLE_CH_C_MAX)
	adc_data->ch_adc_data[pah_verify_ch_c].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_c].max = PAH_LED_TARGET_SAMPLE_CH_C_MAX;
#endif
	// t
#if defined(PAH_LED_TARGET_SAMPLE_CH_T_MIN)
	adc_data->ch_adc_data[pah_verify_ch_t].min_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_t].min = PAH_LED_TARGET_SAMPLE_CH_T_MIN;
#endif
#if defined(PAH_LED_TARGET_SAMPLE_CH_T_MAX)
	adc_data->ch_adc_data[pah_verify_ch_t].max_enabled_verify = true;
	adc_data->ch_adc_data[pah_verify_ch_t].max = PAH_LED_TARGET_SAMPLE_CH_T_MAX;
#endif
	return pah_success;

FAIL:
	debug_printf("pah_verify_led_target_sample_read_adc_data() failed. ret = %d \n", ret);
	return ret;
}

pah_ret pah_verify_adc_data_test(pah_verify_adc_data_s *adc_data)
{
	bool verify_failed = false;
	int i = 0;

	for (i = 0; i < pah_verify_ch_num; ++i) {
		adc_data->ch_adc_data[i].min_verified_success = true;
		if (adc_data->ch_adc_data[i].min_enabled_verify) {
			// debug_printf("_pah8112_verify_adc_data(), ch = %d, value = %d, min = %d \n", i,
			// adc_data->ch_adc_data[i].value, adc_data->ch_adc_data[i].min);

			if (adc_data->ch_adc_data[i].value < adc_data->ch_adc_data[i].min) {
				adc_data->ch_adc_data[i].min_verified_success = false;
				verify_failed = true;
			}
		}

		adc_data->ch_adc_data[i].max_verified_success = true;
		if (adc_data->ch_adc_data[i].max_enabled_verify) {
			// debug_printf("_pah8112_verify_adc_data(), ch = %d, value = %d, max = %d \n", i,
			// adc_data->ch_adc_data[i].value, adc_data->ch_adc_data[i].max);

			if (adc_data->ch_adc_data[i].max < adc_data->ch_adc_data[i].value) {
				adc_data->ch_adc_data[i].max_verified_success = false;
				verify_failed = true;
			}
		}
	}

	if (verify_failed) {
		debug_printf("_pah8112_verify_adc_data() failed \n");
		return pah_err_verify_device_fail;
	}

	return pah_success;
}

//============================================================================
// PRIVATE FUNCTION DEFINITIONS
//============================================================================
static pah_ret _pah8112_start_tg(void)
{
	pah_ret ret = pah_err_unknown;

	// must sleep between update flag and TG enable.
	delay_ms(1);

	// TG enable
	if (!pah_comm_write(0x7F, 0x01)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_write(0x30, 0x01)) // TG enable
	{
		ret = pah_err_platform_fail;
		goto FAIL;
	}

	// wait for clk
	delay_ms(1);

	return pah_success;

FAIL:
	debug_printf("_pah8112_start_tg() failed. ret = %d \n", ret);
	return ret;
}
static pah_ret _pah8112_read_adc_data(pah_verify_adc_data_s *adc_data)
{
	pah_ret ret = pah_err_unknown;
	uint8_t bank2_ch_addr[pah_verify_ch_num];
	int i = 0;

	if (!adc_data) {
		ret = pah_err_invalid_program;
		goto FAIL;
	}

	bank2_ch_addr[pah_verify_ch_t] = 0x00;
	bank2_ch_addr[pah_verify_ch_a] = 0x15;
	bank2_ch_addr[pah_verify_ch_b] = 0x17;
	bank2_ch_addr[pah_verify_ch_c] = 0x19;

	if (!pah_comm_write(0x7F, 0x02)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	for (i = 0; i < pah_verify_ch_num; ++i) {
		if (!pah_comm_burst_read(bank2_ch_addr[i], (uint8_t *)&adc_data->ch_adc_data[i].value, 2)) {
			ret = pah_err_platform_fail;
			goto FAIL;
		}
	}

	return pah_success;

FAIL:
	debug_printf("_pah8112_read_adc_data() failed. ret = %d \n", ret);
	return ret;
}

static pah_ret _pah8112_write_settings_and_read_adc_data(const uint8_t reg_array[][2], uint32_t length,
														 pah_verify_adc_data_s *adc_data,
														 pah_verify_expo_time_s *expo_time)
{
	pah_ret ret = pah_err_unknown;
	if (!adc_data) {
		ret = pah_err_invalid_program;
		goto FAIL;
	}

	ret = pah_8112_startup();
	if (PAH_FAILED(ret))
		goto FAIL;

	// write settings
	ret = pah_8112_write_reg_array(reg_array, length);
	if (PAH_FAILED(ret))
		goto FAIL;

	// Turn Expo Time
	ret = _pah8112_expo_update(expo_time);
	if (PAH_FAILED(ret))
		goto FAIL;

	ret = pah_8112_update_flag();
	if (PAH_FAILED(ret))
		goto FAIL;

	ret = _pah8112_start_tg();
	if (PAH_FAILED(ret))
		goto FAIL;

	delay_ms(PAH_VERIFY_DELAY_BEFORE_READ);

	ret = _pah8112_read_adc_data(adc_data);
	if (PAH_FAILED(ret))
		goto FAIL;

	ret = pah_8112_shutdown();
	if (PAH_FAILED(ret))
		goto FAIL;

	return pah_success;

FAIL:
	debug_printf("pah_verify_light_leak_read_adc_data() failed. ret = %d \n", ret);
	return ret;
}

static pah_ret _pah8112_read_adc_data_fifo(pah_verify_adc_data_s *adc_data)
{
	pah_ret ret = pah_err_unknown;
	// uint8_t bank2_ch_addr[pah_verify_ch_num];
	int i = 0;
	uint32_t RAW_Data[30];
	if (!adc_data) {
		ret = pah_err_invalid_program;
		goto FAIL;
	}

	// bank2_ch_addr[pah_verify_ch_t] = 0x00;

	if (!pah_comm_write(0x7F, 0x03)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_burst_read(0x00, (uint8_t *)RAW_Data, 120)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_write(0x7F, 0x02)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	// if (!pah_comm_read(0x00, (uint8_t*)&adc_data->ch_adc_data[0].value))
	if (!pah_comm_burst_read(0x00, (uint8_t *)&adc_data->ch_adc_data[0].value, 2)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	for (i = 0; i < (pah_verify_ch_num - 1); ++i) {
		adc_data->ch_adc_data[1 + i].value = (uint16_t)((RAW_Data[27 + i]) >> 6);
	}

	return pah_success;

FAIL:
	debug_printf("_pah8112_read_adc_data() failed. ret = %d \n", ret);
	return ret;
}

static pah_ret _pah8112_write_settings_and_read_adc_data_fifo(const uint8_t reg_array[][2], uint32_t length,
															  pah_verify_adc_data_s *adc_data,
															  pah_verify_expo_time_s *expo_time)
{
	pah_ret ret = pah_err_unknown;
	if (!adc_data) {
		ret = pah_err_invalid_program;
		goto FAIL;
	}

	ret = pah_8112_startup();
	if (PAH_FAILED(ret))
		goto FAIL;

	// write settings
	ret = pah_8112_write_reg_array(reg_array, length);
	if (PAH_FAILED(ret))
		goto FAIL;

	// Turn Expo Time
	ret = _pah8112_expo_update(expo_time);
	if (PAH_FAILED(ret))
		goto FAIL;

	ret = pah_8112_update_flag();
	if (PAH_FAILED(ret))
		goto FAIL;

	ret = _pah8112_start_tg();
	if (PAH_FAILED(ret))
		goto FAIL;

	delay_ms(PAH_VERIFY_DELAY_BEFORE_READ);

	ret = _pah8112_read_adc_data_fifo(adc_data);
	if (PAH_FAILED(ret))
		goto FAIL;

	ret = pah_8112_shutdown();
	if (PAH_FAILED(ret))
		goto FAIL;

	return pah_success;

FAIL:
	debug_printf("pah_verify_light_leak_read_adc_data() failed. ret = %d \n", ret);
	return ret;
}

static pah_ret _pah8112_expo_update(pah_verify_expo_time_s *expo_time)
{
	pah_ret ret = pah_err_unknown;
	if (expo_time->expo_time_enabled_verify) {
		if (!pah_comm_write(0x7F, 0x00)) {
			ret = pah_err_platform_fail;
			goto FAIL;
		}
		// 8112ES Update
		if (expo_time->ch_a_expo_time >= 0x2E) {
			if (!pah_comm_write(0x16, expo_time->ch_a_expo_time)) {
				ret = pah_err_platform_fail;
				goto FAIL;
			}
		}
		if (expo_time->ch_b_expo_time >= 0x2E) {
			if (!pah_comm_write(0x35, expo_time->ch_b_expo_time)) {
				ret = pah_err_platform_fail;
				goto FAIL;
			}
		}
		if (expo_time->ch_c_expo_time >= 0x2E) {
			if (!pah_comm_write(0x54, expo_time->ch_c_expo_time)) {
				ret = pah_err_platform_fail;
				goto FAIL;
			}
		}
	}
	return pah_success;

FAIL:
	debug_printf("_pah8112_expo_update() failed. ret = %d \n", ret);
	return ret;
}
