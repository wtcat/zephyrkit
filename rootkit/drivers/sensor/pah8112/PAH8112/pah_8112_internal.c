/*==============================================================================
* Edit History
*
* This section contains comments describing changes made to the module. Notice
* that changes are listed in reverse chronological order. Please use ISO format
* for dates.
*
* when       who       what, where, why
* ---------- ---       -----------------------------------------------------------
* 2016-09-26 bh        - Initial revision.
==============================================================================*/

#include "pah_8112_internal.h"

// pah
#include "pah_comm.h"

// platform support
#include "system_clock.h"

//============================================================================
// PUBLIC FUNCTION DEFINITIONS
//============================================================================
pah_ret pah_8112_verify_product_id(void)
{
	pah_ret ret = pah_err_unknown;
	uint8_t data = 0;

	if (!pah_comm_write(0x7F, 0x00)) // bank0
	{
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_read(0x00, &data)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}

	if (data != PAH_PRODUCT_ID) {
		debug_printf("pah_8112_verify_product_id() fail. id = 0x%X \n", data);
		ret = pah_err_verify_device_fail;
		goto FAIL;
	}

	debug_printf("pah_8112_verify_product_id() success. id = 0x%X \n", data);

	return pah_success;

FAIL:
	debug_printf("pah8112_verify_product_id() failed. ret = %d \n", ret);
	return ret;
}

pah_ret pah_8112_write_touch_package_setting(void)
{
	pah_ret ret = pah_err_unknown;
	device_package_e device_package;
	uint8_t Cmd_TouchDetection_Th_Hi_15_8; // Touch upper boundary
	uint8_t Cmd_TouchDetection_Th_Lo_15_8; // Touch lower boundary
	uint8_t Cmd_adc_osr_t;				   // adc oversampling rate
	uint8_t T_ledbias_sel;

	ret = pah_8112_read_device_package(&device_package);
	if (PAH_FAILED(ret))
		goto FAIL;

	switch (device_package) {
	case device_package_et:
		Cmd_TouchDetection_Th_Hi_15_8 = 0x04;
		Cmd_TouchDetection_Th_Lo_15_8 = 0x04;
		Cmd_adc_osr_t = 0x06;
		T_ledbias_sel = 0x00;
		break;

	case device_package_es:
		Cmd_TouchDetection_Th_Hi_15_8 = 0x07;
		Cmd_TouchDetection_Th_Lo_15_8 = 0x06;
		Cmd_adc_osr_t = 0x03;
		T_ledbias_sel = 0x03;
		break;

	default:
		ret = pah_err_not_implement;
		goto FAIL;
	}

	if (!pah_comm_write(0x7F, 0x01)) // bank1
	{
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_write(0x05, Cmd_TouchDetection_Th_Hi_15_8)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_write(0x08, Cmd_TouchDetection_Th_Lo_15_8)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_write(0x14, Cmd_adc_osr_t)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_write(0x75, T_ledbias_sel)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}

	return pah_success;

FAIL:
	debug_printf("pah_8112_set_touch_setting() failed. ret = %d \n", ret);
	return ret;
}

pah_ret pah_8112_write_reg_array(const uint8_t reg_array[][2], uint32_t length)
{
	uint32_t i = 0;

	for (i = 0; i < length; ++i) {
		if (!pah_comm_write(reg_array[i][0], reg_array[i][1])) {
			debug_printf("pah_8112_write_reg_array(). pah_comm_write fail, i = %d \n", i);
			return pah_err_platform_fail;
		}
	}

	return pah_success;
}

pah_ret pah_8112_update_flag(void)
{
	if (!pah_comm_write(0x7F, 0x01)) // bank1
		return pah_err_platform_fail;
	if (!pah_comm_write(0x24, 0x01))
		return pah_err_platform_fail;
	delay_ms(2);
	return pah_success;
}

pah_ret pah_8112_device_init(void)
{
	pah_ret ret = pah_err_unknown;

	if (!pah_comm_write(0x7F, 0x04)) // bank4
	{
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_write(0x34, 0x01)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}

	ret = pah_8112_shutdown();
	if (PAH_FAILED(ret))
		goto FAIL;

	return pah_success;

FAIL:
	debug_printf("pah_8112_device_init() failed. ret = %d \n", ret);
	return ret;
}

pah_ret pah_8112_startup(void)
{
	if (!pah_comm_write(0x7F, 0x04)) // bank4
		return pah_err_platform_fail;
	if (!pah_comm_write(0x69, 0x00))
		return pah_err_platform_fail;

	// When the device wakes up from power down mode, delay is necessary for the internal controller to reset the whole
	// system and then the device is ready for the upcoming operations. Without delay, the driver would be unable to
	// write settings correctly.
	delay_ms(2);

	return pah_success;
}

pah_ret pah_8112_shutdown(void)
{
	if (!pah_comm_write(0x7F, 0x04)) // bank4
		return pah_err_platform_fail;
	if (!pah_comm_write(0x69, 0x01))
		return pah_err_platform_fail;

	return pah_success;
}

pah_ret pah_8112_read_device_package(device_package_e *device_package)
{
	pah_ret ret = pah_err_unknown;
	uint8_t data = 0;

	if (!device_package) {
		ret = pah_err_invalid_program;
		goto FAIL;
	}

	if (!pah_comm_write(0x7F, 0x02)) // bank2
	{
		ret = pah_err_platform_fail;
		goto FAIL;
	}
	if (!pah_comm_read(0x4A, &data)) {
		ret = pah_err_platform_fail;
		goto FAIL;
	}

	data = PAH_CHECK_BIT(data, 0);

	*device_package = (device_package_e)data;

	return pah_success;

FAIL:
	debug_printf("pah_8112_read_device_package() failed. ret = %d \n", ret);
	return ret;
}
