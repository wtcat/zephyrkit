/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "icm42607.h"
#include "icm42607_reg.h"
#include "icm42607_spi.h"

LOG_MODULE_DECLARE(ICM42607, CONFIG_SENSOR_LOG_LEVEL);

int icm42607_set_fs(const struct device *dev, uint16_t a_sf, uint16_t g_sf)
{
	uint8_t databuf;
	int result;

	result = inv_spi_read(REG_ACCEL_CONFIG0, &databuf, 1);
	if (result) {
		return result;
	}
	databuf &= ~BIT_ACCEL_FSR;

	databuf |= a_sf;

	result = inv_spi_single_write(REG_ACCEL_CONFIG0, &databuf);

	result = inv_spi_read(REG_GYRO_CONFIG0, &databuf, 1);

	if (result) {
		return result;
	}

	databuf &= ~BIT_GYRO_FSR;
	databuf |= g_sf;

	result = inv_spi_single_write(REG_GYRO_CONFIG0, &databuf);

	if (result) {
		return result;
	}

	return 0;
}

int icm42607_set_odr(const struct device *dev, uint16_t a_rate, uint16_t g_rate)
{
	uint8_t databuf;
	int result;

	if (a_rate > 8000 || g_rate > 8000 ||
	    a_rate < 1 || g_rate < 12) {
		LOG_ERR("Not supported frequency");
		return -ENOTSUP;
	}

	result = inv_spi_read(REG_ACCEL_CONFIG0, &databuf, 1);

	if (result) {
		return result;
	}

	databuf &= ~BIT_ACCEL_ODR;

	if (a_rate > 4000) {
		databuf |= BIT_ACCEL_ODR_8000;
	} else if (a_rate > 2000) {
		databuf |= BIT_ACCEL_ODR_4000;
	} else if (a_rate > 1000) {
		databuf |= BIT_ACCEL_ODR_2000;
	} else if (a_rate > 500) {
		databuf |= BIT_ACCEL_ODR_1000;
	} else if (a_rate > 200) {
		databuf |= BIT_ACCEL_ODR_500;
	} else if (a_rate > 100) {
		databuf |= BIT_ACCEL_ODR_200;
	} else if (a_rate > 50) {
		databuf |= BIT_ACCEL_ODR_100;
	} else if (a_rate > 25) {
		databuf |= BIT_ACCEL_ODR_50;
	} else if (a_rate > 12) {
		databuf |= BIT_ACCEL_ODR_25;
	} else if (a_rate > 6) {
		databuf |= BIT_ACCEL_ODR_12;
	} else if (a_rate > 3) {
		databuf |= BIT_ACCEL_ODR_6;
	} else if (a_rate > 1) {
		databuf |= BIT_ACCEL_ODR_3;
	} else {
		databuf |= BIT_ACCEL_ODR_1;
	}

	result = inv_spi_single_write(REG_ACCEL_CONFIG0, &databuf);

	if (result) {
		return result;
	}

	LOG_DBG("Write Accel ODR 0x%X", databuf);

	result = inv_spi_read(REG_GYRO_CONFIG0, &databuf, 1);

	if (result) {
		return result;
	}

	databuf &= ~BIT_GYRO_ODR;

	if (g_rate > 4000) {
		databuf |= BIT_GYRO_ODR_8000;
	} else if (g_rate > 2000) {
		databuf |= BIT_GYRO_ODR_4000;
	} else if (g_rate > 1000) {
		databuf |= BIT_GYRO_ODR_2000;
	} else if (g_rate > 500) {
		databuf |= BIT_GYRO_ODR_1000;
	} else if (g_rate > 200) {
		databuf |= BIT_GYRO_ODR_500;
	} else if (g_rate > 100) {
		databuf |= BIT_GYRO_ODR_200;
	} else if (g_rate > 50) {
		databuf |= BIT_GYRO_ODR_100;
	} else if (g_rate > 25) {
		databuf |= BIT_GYRO_ODR_50;
	} else if (g_rate > 12) {
		databuf |= BIT_GYRO_ODR_25;
	} else {
		databuf |= BIT_GYRO_ODR_12;
	}

	LOG_DBG("Write GYRO ODR 0x%X", databuf);

	result = inv_spi_single_write(REG_GYRO_CONFIG0, &databuf);
	if (result) {
		return result;
	}

	return result;
}

int icm42607_sensor_init(const struct device *dev)
{
	int result = 0;
	uint8_t v;

	result = inv_spi_read(REG_WHO_AM_I, &v, 1);


	if (v != 0x61) {
		return result;
	}
	LOG_DBG("WHO AM I : 0x%X", v);

	//2000dps, 16/du
	inv_spi_read(0x20,&v,1);
	v &= 0x90;//不更改保留位
	v |= 0xa;
	inv_spi_single_write(0x20, &v);

	inv_spi_read(0x23,&v,1);
	v &= 0xF8;//不更改保留位
	v |= 0x0;	
	inv_spi_single_write(0x23, &v);

	k_msleep(10);

	//acc 16g,  4096/g
	inv_spi_read(0x21,&v,1);
	v &= 0x90;//不更改保留位
	v |= 0x0a;
	inv_spi_single_write(0x21, &v);

   	inv_spi_read(0x24,&v,1);
	v &= 0x88;//不更改保留位
	v |= 0x30; 
	inv_spi_single_write(0x24, &v);

	k_msleep(10);

	inv_spi_read(0x22,&v,1);
	v &= 0x87;//不更改保留位
	v |= 0x20;
	inv_spi_single_write(0x22, &v);

	k_msleep(10);

	inv_spi_read(0x1F,&v,1);
	v &= 0x70;//不更改保留位
	v |= 0x1f;
	inv_spi_single_write(0x1F, &v);

	inv_spi_read(0x2d,&v,1);
	v &= 0x00;//不更改保留位
	v |= 0x08;
	inv_spi_single_write(0x2d, &v);

	/***
	//set fifo data format 
	inv_spi_read(0x35,&v,1);
	v &= 0x8c;//不更改保留位
	v |= 0x30;
	inv_spi_single_write(0x35, &v);

	//set fifo mode
	inv_spi_read(0x28,&v,1);
	v &= 0xfc;//不更改保留位
	v |= 0x0;
	inv_spi_single_write(0x28, &v);
	**/
	return 0;

}

int icm42607_turn_on_fifo(const struct device *dev)
{
	const struct icm42607_data *drv_data = dev->data;

	uint8_t int0_en = BIT_INT_UI_DRDY_INT1_EN;
	uint8_t fifo_en = BIT_FIFO_ACCEL_EN | BIT_FIFO_GYRO_EN | BIT_FIFO_WM_TH;
	uint8_t burst_read[3];
	int result;
	uint8_t v = 0;

	v = BIT_FIFO_MODE_BYPASS;
	result = inv_spi_single_write(REG_FIFO_CONFIG, &v);
	if (result) {
		return result;
	}

	v = 0;
	result = inv_spi_single_write(REG_FIFO_CONFIG1, &v);
	if (result) {
		return result;
	}

	result = inv_spi_read(REG_FIFO_COUNTH, burst_read, 2);
	if (result) {
		return result;
	}

	result = inv_spi_read(REG_FIFO_DATA, burst_read, 3);
	if (result) {
		return result;
	}

	v = BIT_FIFO_MODE_STREAM;
	result = inv_spi_single_write(REG_FIFO_CONFIG, &v);
	if (result) {
		return result;
	}

	result = inv_spi_single_write(REG_FIFO_CONFIG1, &fifo_en);
	if (result) {
		return result;
	}

	result = inv_spi_single_write(REG_INT_SOURCE0, &int0_en);
	if (result) {
		return result;
	}

	if (drv_data->tap_en) {
		v = BIT_TAP_ENABLE;
		result = inv_spi_single_write(REG_APEX_CONFIG0, &v);
		if (result) {
			return result;
		}

		v = BIT_DMP_INIT_EN;
		result = inv_spi_single_write(REG_SIGNAL_PATH_RESET, &v);
		if (result) {
			return result;
		}

		v = BIT_BANK_SEL_4;
		result = inv_spi_single_write(REG_BANK_SEL, &v);
		if (result) {
			return result;
		}

		v = BIT_INT_STATUS_TAP_DET;
		result = inv_spi_single_write(REG_INT_SOURCE6, &v);
		if (result) {
			return result;
		}

		v = BIT_BANK_SEL_0;
		result = inv_spi_single_write(REG_BANK_SEL, &v);
		if (result) {
			return result;
		}
	}

	LOG_DBG("turn on fifo done");
	return 0;
}

int icm42607_turn_off_fifo(const struct device *dev)
{
	const struct icm42607_data *drv_data = dev->data;

	uint8_t int0_en = 0;
	uint8_t burst_read[3];
	int result;
	uint8_t v = 0;

	v = BIT_FIFO_MODE_BYPASS;
	result = inv_spi_single_write(REG_FIFO_CONFIG, &v);
	if (result) {
		return result;
	}

	v = 0;
	result = inv_spi_single_write(REG_FIFO_CONFIG1, &v);
	if (result) {
		return result;
	}

	result = inv_spi_read(REG_FIFO_COUNTH, burst_read, 2);
	if (result) {
		return result;
	}

	result = inv_spi_read(REG_FIFO_DATA, burst_read, 3);
	if (result) {
		return result;
	}

	result = inv_spi_single_write(REG_INT_SOURCE0, &int0_en);
	if (result) {
		return result;
	}

	if (drv_data->tap_en) {
		v = 0;
		result = inv_spi_single_write(REG_APEX_CONFIG0, &v);
		if (result) {
			return result;
		}

		result = inv_spi_single_write(REG_SIGNAL_PATH_RESET, &v);
		if (result) {
			return result;
		}

		v = BIT_BANK_SEL_4;
		result = inv_spi_single_write(REG_BANK_SEL, &v);
		if (result) {
			return result;
		}

		v = 0;
		result = inv_spi_single_write(REG_INT_SOURCE6, &v);
		if (result) {
			return result;
		}

		v = BIT_BANK_SEL_0;
		result = inv_spi_single_write(REG_BANK_SEL, &v);
		if (result) {
			return result;
		}
	}

	return 0;
}

int icm42607_turn_on_sensor(const struct device *dev)
{
	struct icm42607_data *drv_data = dev->data;

	uint8_t v = 0;
	int result = 0;


	if (drv_data->sensor_started) {
		LOG_ERR("Sensor already started");
		return -EALREADY;
	}

	icm42607_set_fs(dev, drv_data->accel_sf, drv_data->gyro_sf);

	icm42607_set_odr(dev, drv_data->accel_hz, drv_data->gyro_hz);

	v |= BIT_ACCEL_MODE_LNM;
	v |= BIT_GYRO_MODE_LNM;

	result = inv_spi_single_write(REG_PWR_MGMT0, &v);
	if (result) {
		return result;
	}

	/* Accelerometer sensor need at least 10ms startup time
	 * Gyroscope sensor need at least 30ms startup time
	 */
	k_msleep(100);

	icm42607_turn_on_fifo(dev);

	drv_data->sensor_started = true;

	return 0;
}

int icm42607_turn_off_sensor(const struct device *dev)
{
	uint8_t v = 0;
	int result = 0;

	result = inv_spi_read(REG_PWR_MGMT0, &v, 1);

	v ^= BIT_ACCEL_MODE_LNM;
	v ^= BIT_GYRO_MODE_LNM;

	result = inv_spi_single_write(REG_PWR_MGMT0, &v);
	if (result) {
		return result;
	}

	/* Accelerometer sensor need at least 10ms startup time
	 * Gyroscope sensor need at least 30ms startup time
	 */
	k_msleep(100);

	icm42607_turn_off_fifo(dev);

	return 0;
}
