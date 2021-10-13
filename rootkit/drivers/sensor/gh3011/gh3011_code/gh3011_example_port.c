/**
 * @copyright (c) 2003 - 2020, Goodix Co., Ltd. All rights reserved.
 *
 * @file    gh3011_example_port.c
 *
 * @brief   example code for gh3011 (condensed  hbd_ctrl lib)
 *
 */


#include "gh3011_example_common.h"
#include "gh3011_i2c.h"


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
#include "gpio/gpio_apollo3p.h"
#include "drivers_ext/sensor_priv.h"
#include "sys/printk.h"
#include "hr_srv.h"

static const struct device *sensor_dev;
/* gh30x i2c interface */

#include "device.h"
#include "i2c/i2c-message.h"

#define BUS_DEV "I2C_2"
#define SLAVE_ADDR 0x14
const struct device *i2c_dev = NULL;
/// i2c for gh30x init
void hal_gh30x_i2c_init(void)
{
	// code implement by user
	i2c_dev = device_get_binding(BUS_DEV);	
}

/// i2c for gh30x wrtie
uint8_t hal_gh30x_i2c_write(uint8_t device_id, const uint8_t write_buffer[], uint16_t length)
{
	uint8_t ret = GH30X_EXAMPLE_OK_VAL;
	//ret = gh3011_i2c_write(device_id, write_buffer, length);	
	if (i2c_dev != NULL) {
		
			struct i2c_message msg = {
			.msg =
				{
					.flags = I2C_MSG_WRITE,
					.len = length-2,
					.buf = (uint8_t *)(write_buffer + 2),
				},
			.reg_addr = (write_buffer[0] << 8) + write_buffer[1],
			.reg_len = 2,
			};
			return i2c_transfer(i2c_dev, &msg.msg, 1, SLAVE_ADDR);
		
	} else {
		return -1;
	}
	// code implement by user
	return ret;
}

/// i2c for gh30x read
uint8_t hal_gh30x_i2c_read(uint8_t device_id, const uint8_t write_buffer[], uint16_t write_length, uint8_t read_buffer[], uint16_t read_length)
{
	uint8_t ret = GH30X_EXAMPLE_OK_VAL;
	// ret = gh3011_i2c_read(device_id, write_buffer, write_length, read_buffer, read_length);
	// code implement by user
	if (i2c_dev != NULL) {
		
			struct i2c_message msg = {
			.msg =
				{
					.flags = I2C_MSG_READ,
					.len = read_length ,
					.buf = (uint8_t *)(read_buffer),
				},
			.reg_addr = (write_buffer[0] << 8) + write_buffer[1],
			.reg_len = 2,
			};
			return i2c_transfer(i2c_dev, &msg.msg, 1, SLAVE_ADDR);
		
	} else {
		return -1;
	}
	return ret;
}

/* gh30x spi interface */

/// spi for gh30x init
void hal_gh30x_spi_init(void)
{
	// code implement by user
}

/// spi for gh30x wrtie
uint8_t hal_gh30x_spi_write(const uint8_t write_buffer[], uint16_t length)
{
	uint8_t ret = 1;
	// code implement by user
	return ret;
}

/// spi for gh30x read
uint8_t hal_gh30x_spi_read(uint8_t read_buffer[], uint16_t length)
{
	uint8_t ret = 1;
	// code implement by user
	return ret;
}

/// spi cs set low for gh30x
void hal_gh30x_spi_cs_set_low(void)
{
	// code implement by user
}

/// spi cs set high for gh30x
void hal_gh30x_spi_cs_set_high(void)
{
	// code implement by user
}


/* delay */

/// delay us
void hal_gh30x_delay_us(uint16_t us_cnt)
{
	// code implement by user
	k_usleep(us_cnt);
}

/* gsensor driver */

/// gsensor motion detect mode flag
bool gsensor_drv_motion_det_mode = false;

/// gsensor init
int8_t gsensor_drv_init(void)
{
	int8_t ret = GH30X_EXAMPLE_OK_VAL;
	gsensor_drv_motion_det_mode = false;
	sensor_dev = device_get_binding("ICM42607");
	if (sensor_dev) {
		return GH30X_EXAMPLE_OK_VAL;
	} else {
		return GH30X_EXAMPLE_ERR_VAL;
	}

	// code implement by user
	/* if enable all func equal 25Hz, should config > 25Hz;
	but if enable have 100hz, should config to > 100hz. if not, feeback to GOODIX!!!
	*/
	return ret;
}

/// gsensor enter normal mode
void gsensor_drv_enter_normal_mode(void)
{
	// code implement by user
	gsensor_drv_motion_det_mode = false;
}

/// gsensor enter fifo mode
void gsensor_drv_enter_fifo_mode(void)
{
	// code implement by user
	gsensor_drv_motion_det_mode = false;
}

/// gsensor enter motion det mode
void gsensor_drv_enter_motion_det_mode(void)
{
	// code implement by user
	/* if enable motion det mode that call @ref hal_gsensor_drv_int1_handler when motion generate irq
		e.g. 1. (hardware) use gsensor motion detect module with reg config
			 2. (software) gsensor enter normal mode, then define 30ms timer get gsensor rawdata,
			 	if now total acceleration(sqrt(x*x+y*y+z*z)) - last total acceleration >= 30 (0.05g @512Lsb/g) as motion
				generate that call @ref hal_gsensor_drv_int1_handler
	*/
	gsensor_drv_motion_det_mode = true;
}

#include <sensor/icm42607/icm42607.h>
static ACC_FIFO_T acc_fifo;
static struct sensor_value accel[3];
/// gsensor get fifo data
void gsensor_drv_get_fifo_data(ST_GS_DATA_TYPE gsensor_buffer[], uint16_t *gsensor_buffer_index, uint16_t gsensor_max_len)
{	
	if (NULL != sensor_dev) {
		sensor_channel_get(sensor_dev, SENSOR_CHAN_ALL, &acc_fifo);
		if (acc_fifo.count == MAX_ACC_FIFO_SIZE) {
			memcpy(gsensor_buffer, &acc_fifo.array[acc_fifo.writeP], 
				(MAX_ACC_FIFO_SIZE - acc_fifo.writeP) * sizeof(ST_GS_DATA_TYPE));	
			memcpy(gsensor_buffer + (MAX_ACC_FIFO_SIZE - acc_fifo.writeP), &acc_fifo.array[0], acc_fifo.writeP * sizeof(ST_GS_DATA_TYPE));
		}else {
			memcpy(gsensor_buffer, &acc_fifo.array[0], acc_fifo.count * sizeof(ST_GS_DATA_TYPE));
		}	
		*gsensor_buffer_index = acc_fifo.count;
	}

	// code implement by user
}

/// gsensor clear buffer 
void gsensor_drv_clear_buffer(ST_GS_DATA_TYPE gsensor_buffer[], uint16_t *gsensor_buffer_index, uint16_t gsensor_buffer_len)
{
    if ((gsensor_buffer != NULL) && (gsensor_buffer_index != NULL))
    {
        memset(gsensor_buffer, 0, sizeof(ST_GS_DATA_TYPE) * gsensor_buffer_len);
        *gsensor_buffer_index = 0;
    }
}

void sensor_accel_convert_to_4g(ST_GS_DATA_TYPE *data_4g, struct sensor_value *accel)
{
	data_4g->sXAxisVal = accel[0].val1 >> 2;
	data_4g->sYAxisVal = accel[1].val1 >> 2;
	data_4g->sZAxisVal = accel[2].val2 >> 2;
}
/// gsensor get data
void gsensor_drv_get_data(ST_GS_DATA_TYPE *gsensor_data_ptr)
{
	// code implement by user	
	if (NULL != sensor_dev) {
		sensor_channel_get(sensor_dev, SENSOR_CHAN_ACCEL_XYZ, accel);
		if (gsensor_data_ptr != NULL) {
			sensor_accel_convert_to_4g(gsensor_data_ptr, accel);
		}
	}
}
/* int */

/// gh30x int handler
void hal_gh30x_int_handler(void)
{
    gh30x_int_msg_handler();
}



#define GH3011_PIN(pin) (pin & 31)
void call_hal_gh30x_int_handler(int irq, void *func);
/// gh30x int pin init, should config as rise edge trigger
void hal_gh30x_int_init(void)
{
	// code implement by user
    // must register func hal_gh30x_int_handler as callback
	const struct device *irq_gpio_dev = device_get_binding("GPIO_1");
	if (irq_gpio_dev != NULL) {
		gpio_apollo3p_request_irq(irq_gpio_dev, GH3011_PIN(32), call_hal_gh30x_int_handler, NULL, GPIO_INT_TRIG_HIGH, true);
	} 
}

/// gsensor int handler
void hal_gsensor_drv_int1_handler(void)
{
// code implement by user
	if (gsensor_drv_motion_det_mode)
	{
		gsensor_motion_has_detect();
	}
	else
	{
		/* if using gsensor fifo mode, should get data by fifo int 
			* e.g. gsensor_read_fifo_data();   
		*/
		gsensor_read_fifo_data(); // got fifo int
	}
}

/// gsensor int1 init, should config as rise edge trigger
void hal_gsensor_int1_init(void)
{
	// code implement by user
	// must register func hal_gsensor_drv_int1_handler as callback
	hal_gsensor_drv_int1_handler();
    
	/* if using gsensor fifo mode,
	and gsensor fifo depth is not enough to store 1 second data,
	please connect gsensor fifo interrupt to the host,
	or if using gsensor motion detect mode(e.g  motion interrupt response by 0.5G * 5counts),
	and implement this function to receive gsensor interrupt.
	*/ 
}

#include "pah_platform_types.h"
extern struct gh3011_data gh3011_data;
/* handle algorithm result */
extern uint8_t HR;
extern uint8_t SPO2;

/// handle hb mode algorithm result
void handle_hb_mode_result(uint8_t hb_val, uint8_t hb_lvl_val, uint8_t wearing_state_val, uint16_t rr_val, int32_t rawdata_ptr[][DBG_MCU_MODE_PKG_LEN], 
									uint16_t rawdata_len)
{
	// code implement by user	
	gh3011_data.hr_data = (int32_t)hb_val;
	// HR = hb_val;
	put_hr_to_heartrate_srv(hb_val);
}

/// handle spo2 mode algorithm result
void handle_spo2_mode_result(uint8_t spo2_val, uint8_t spo2_lvl_val, uint8_t hb_val, uint8_t hb_lvl_val, uint16_t rr_val[4], uint8_t rr_lvl_val, uint8_t rr_cnt, 
									uint16_t spo2_r_val, uint8_t wearing_state_val, int32_t rawdata_ptr[][DBG_MCU_MODE_PKG_LEN], uint16_t rawdata_len)
{
	// code implement by user
	gh3011_data.spo2_data = (int32_t)spo2_val;
	// SPO2 = spo2_val;
	put_spo2_to_heartrate_srv(spo2_val);
}

/// handle hrv mode algorithm result
void handle_hrv_mode_result(uint16_t rr_val_arr[HRV_MODE_RES_MAX_CNT], uint8_t rr_val_cnt, uint8_t rr_lvl, int32_t rawdata_ptr[][DBG_MCU_MODE_PKG_LEN], uint16_t rawdata_len)
{
	// code implement by user
}

/// handle hsm mode algorithm result
void handle_hsm_mode_result(uint8_t hb_val, uint8_t sleep_state_val, uint8_t respiratory_rate_val, uint16_t asleep_time_val, 
									uint16_t wakeup_time_val, int32_t rawdata_ptr[][DBG_MCU_MODE_PKG_LEN], uint16_t rawdata_len)
{
	// code implement by user
}

/// handle bpd mode algorithm result
void handle_bpd_mode_result(uint16_t sbp_val, uint16_t dbp_val, int32_t rawdata_ptr[][DBG_MCU_MODE_PKG_LEN], uint16_t rawdata_len)
{
	// code implement by user
}

/// handle pfa mode algorithm result
void handle_pfa_mode_result(uint8_t pressure_level_val, uint8_t fatigue_level_val, uint8_t body_age_val, 
									int32_t rawdata_ptr[][DBG_MCU_MODE_PKG_LEN], uint16_t rawdata_len)
{
	// code implement by user
}

/// handle wear status result
void handle_wear_status_result(uint8_t wearing_state_val)
{
	// code implement by user
}

/// handle wear status result, otp_res: <0=> ok , <1=> err , <2=> para err
void handle_system_test_otp_check_result(uint8_t otp_res)
{
	// code implement by user
}

/// handle wear status result, led_num: {0-2};os_res: <0=> ok , <1=> rawdata err , <2=> noise err , <3=> para err
void handle_system_test_os_result(uint8_t led_num, uint8_t os_res)
{
	// code implement by user
}

/* ble */

/// send value via heartrate profile
void ble_module_send_heartrate(uint32_t heartrate)
{
	// code implement by user
}

/// add value to heartrate profile
void ble_module_add_rr_interval(uint16_t rr_interval_arr[], uint8_t cnt)
{
	// code implement by user
}

/// send string via through profile
uint8_t ble_module_send_data_via_gdcs(uint8_t string[], uint8_t length)
{
	uint8_t ret = GH30X_EXAMPLE_OK_VAL;
	// code implement by user
	return ret;
}

/// recv data via through profile
void ble_module_recv_data_via_gdcs(uint8_t *data, uint8_t length)
{
	gh30x_app_cmd_parse(data, length);
}


/* timer */
struct k_timer fifo_int_timer;
/// gh30x fifo int timeout timer handler
void hal_gh30x_fifo_int_timeout_timer_handler(void)
{
	gh30x_fifo_int_timeout_msg_handler();
}

static void hal_gh30x_fifo_timer(struct k_timer *timer)
{
	hal_gh30x_fifo_int_timeout_timer_handler();
}

/// gh30x fifo int timeout timer start
void hal_gh30x_fifo_int_timeout_timer_start(void)
{
    // code implement by user
	k_timer_start(&fifo_int_timer, K_MSEC(0), K_MSEC(120)); 
}

/// gh30x fifo int timeout timer stop
void hal_gh30x_fifo_int_timeout_timer_stop(void)
{
    // code implement by user
	k_timer_stop(&fifo_int_timer);
}

/// gh30x fifo int timeout timer init 
void hal_gh30x_fifo_int_timeout_timer_init(void)
{
	// code implement by user
	// must register func gh30x_fifo_int_timeout_timer_handler as callback
	/* should setup timer interval with fifo int freq(e.g. 1s fifo int setup 1080ms timer)
	*/
	k_timer_init(&fifo_int_timer, hal_gh30x_fifo_timer, NULL);
    
}

#if ((__USE_GOODIX_APP__) && (__ALGO_CALC_WITH_DBG_DATA__))
/// ble repeat send data timer handler
void ble_module_repeat_send_timer_handler(void)
{
    send_mcu_rawdata_packet_repeat();
}

/// ble repeat send data timer start
void ble_module_repeat_send_timer_start(void)
{
    // code implement by user
}

/// ble repeat send data timer stop
void ble_module_repeat_send_timer_stop(void)
{
    // code implement by user
}

/// ble repeat send data timer init 
void ble_module_repeat_send_timer_init(void)
{
    // code implement by user
    // must register func ble_module_repeat_send_timer_handler as callback
	/* should setup 100ms timer and ble connect interval should < 100ms
	*/
}
#endif

/* uart */

/// recv data via uart
void uart_module_recv_data(uint8_t *data, uint8_t length)
{
	gh30x_uart_cmd_parse(data, length);
}

/// send value via uart
uint8_t uart_module_send_data(uint8_t string[], uint8_t length)
{
	uint8_t ret = GH30X_EXAMPLE_OK_VAL;
	// code implement by user
	return ret;
}


/* handle cmd with all ctrl cmd @ref EM_COMM_CMD_TYPE */
void handle_goodix_communicate_cmd(EM_COMM_CMD_TYPE cmd_type)
{
	// code implement by user
}


/* log */

/// print dbg log
void example_dbg_log(char *log_string)
{
	//printK("%s\n", log_string);
	// code implement by user
}

#if (__HBD_USE_DYN_MEM__)
void *hal_gh30x_memory_malloc(int size)
{
    // code implement by user    
}

void hal_gh30x_memory_free(void *pmem)
{
    // code implement by user
}
#endif

/********END OF FILE********* Copyright (c) 2003 - 2020, Goodix Co., Ltd. ********/
