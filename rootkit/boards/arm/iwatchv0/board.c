/*
 * Copyright (c) 2018 Yurii Hamann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/sys_io.h>
#include <string.h>


#include "am_mcu_apollo.h"
#include "am_hal_gpio.h"


#define IO_NONE 0
#define IO_HIGH 1
#define IO_LOW  2

struct io_config {
    const char *name;
    uint16_t	pin;
    uint16_t	ival;
    const am_hal_gpio_pincfg_t *cfg;	
};

struct io_array {
    const struct io_config *io;
};

#define _BOARD_IO(obj) {obj}
#define _NULL_IO _IO_ITEM(NULL, 0, 0, NULL)
#define _IO_ITEM(_name, _pin, _ival, _cfg) {\
    .name = _name, \
    .pin = _pin, \
    .ival = _ival, \
    .cfg = _cfg }

/*
 * GPIO configure for UART 
 */
static const am_hal_gpio_pincfg_t k_uart0_tx = {
    .uFuncSel       = AM_HAL_PIN_48_UART0TX,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA
};

static const am_hal_gpio_pincfg_t k_uart0_rx = {
    .uFuncSel       = AM_HAL_PIN_49_UART0RX
};


/*
 * GPIO configure for interrupt 
 */
static const am_hal_gpio_pincfg_t k_gpio_intr = {
    .uFuncSel = 3,
    .eIntDir = AM_HAL_GPIO_PIN_INTDIR_HI2LO,
    .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,
};	

/*
 * GPIO congiure for output
 */
static const am_hal_gpio_pincfg_t k_gpio_output = {
    .uFuncSel       = 3,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL
};

static const am_hal_gpio_pincfg_t k_gpio_output_max = {
    .uFuncSel       = 3,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL
};

static const am_hal_gpio_pincfg_t k_gpio_disable __unused = {
    .uFuncSel       = 3,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_DISABLE
};
/*
 * MSPI0 (LCD)
 *
 * @CE: GP_19
 * @D0: GP_22
 * @D1: GP_26
 * @D2: GP_4
 * @D3: GP_23
 * @SCK: GP_24
 */
static const am_hal_gpio_pincfg_t k_lcd_mspi0_ce0 = {
    .uFuncSel            = AM_HAL_PIN_19_NCE19,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0,
    .uNCE                = 0,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
};

static const am_hal_gpio_pincfg_t k_lcd_mspi0_d0 = {
    .uFuncSel            = AM_HAL_PIN_22_MSPI0_0,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_lcd_mspi0_d1 = {
    .uFuncSel            = AM_HAL_PIN_26_MSPI0_1,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_lcd_mspi0_d2 = {
    .uFuncSel            = AM_HAL_PIN_4_MSPI0_2,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_lcd_mspi0_d3 = {
    .uFuncSel            = AM_HAL_PIN_23_MSPI0_3,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_lcd_mspi0_sck = {
    .uFuncSel            = AM_HAL_PIN_24_MSPI0_8,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

/*
 * MSPI1 (FLASH)
 *
 * @CE: GP_15
 * @D0: GP_51
 * @D1: GP_52
 * @D2: GP_53
 * @D3: GP_54
 * @SCK: GP_59
 */
static const am_hal_gpio_pincfg_t k_flash_mspi1_ce0 = {
    .uFuncSel            = AM_HAL_PIN_15_NCE15,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1,
    .uNCE                = 0,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
};

static const am_hal_gpio_pincfg_t k_flash_mspi1_d0 = {
    .uFuncSel            = AM_HAL_PIN_51_MSPI1_0,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};

static const am_hal_gpio_pincfg_t k_flash_mspi1_d1 = {
    .uFuncSel            = AM_HAL_PIN_52_MSPI1_1,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};

static const am_hal_gpio_pincfg_t k_flash_mspi1_d2 = {
    .uFuncSel            = AM_HAL_PIN_53_MSPI1_2,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};

static const am_hal_gpio_pincfg_t k_flash_mspi1_d3 = {
    .uFuncSel            = AM_HAL_PIN_54_MSPI1_3,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};


static const am_hal_gpio_pincfg_t k_flash_mspi1_sck = {
    .uFuncSel            = AM_HAL_PIN_59_MSPI1_8,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};
    
/*
 * MSPI2 (PSRAM)
 *
 * @CE: GP_63
 * @D0: GP_64
 * @D1: GP_65
 * @D2: GP_66
 * @D3: GP_67
 * @SCK: GP_68
 */
static const am_hal_gpio_pincfg_t k_psram_mspi2_ce0 = {
    .uFuncSel            = AM_HAL_PIN_63_NCE63,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 2,
    .uNCE                = 0,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
};

static const am_hal_gpio_pincfg_t k_psram_mspi2_d0 = {
    .uFuncSel            = AM_HAL_PIN_64_MSPI2_0,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 2
};

static const am_hal_gpio_pincfg_t k_psram_mspi2_d1 = {
    .uFuncSel            = AM_HAL_PIN_65_MSPI2_1,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 2
};

static const am_hal_gpio_pincfg_t k_psram_mspi2_d2 = {
    .uFuncSel            = AM_HAL_PIN_66_MSPI2_2,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 2
};

static const am_hal_gpio_pincfg_t k_psram_mspi2_d3 = {
    .uFuncSel            = AM_HAL_PIN_67_MSPI2_3,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 2
};

static const am_hal_gpio_pincfg_t k_psram_mspi2_sck = {
    .uFuncSel            = AM_HAL_PIN_68_MSPI2_4,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 2
};

/*
 * I/O Master4 (Touch Pannel)
 *
 * @I2C_SDA: GP_40
 * @I2C_SCL: GP_39
 */
static const am_hal_gpio_pincfg_t k_tp_iom4_scl = {
    .uFuncSel            = AM_HAL_PIN_39_M4SCL,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 4
};

static const am_hal_gpio_pincfg_t k_tp_iom4_sda = {
    .uFuncSel            = AM_HAL_PIN_40_M4SDAWIR3,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 4
};

/*
 * I/O Master3 (ChargeIC)
 *
 * @I2C_SDA: GP_43
 * @I2C_SCL: GP_42
 */
static const am_hal_gpio_pincfg_t k_charge_iom3_scl = {
    .uFuncSel            = AM_HAL_PIN_42_M3SCL,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 3
};

static const am_hal_gpio_pincfg_t k_charge_iom3_sda = {
    .uFuncSel            = AM_HAL_PIN_43_M3SDAWIR3,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 3
};

/*
 * I/O Master2 (HeartRate sensor)
 *
 * @I2C_SDA: GP_25
 * @I2C_SCL: GP_27
 */
static const am_hal_gpio_pincfg_t k_hr_iom2_scl = {
    .uFuncSel            = AM_HAL_PIN_27_M2SCL,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 2
};

static const am_hal_gpio_pincfg_t k_hr_iom2_sda = {
    .uFuncSel            = AM_HAL_PIN_25_M2SDAWIR3,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 2
}; 

/*
 * I/O Master1 (Magnetic sensor)
 *
 * @I2C_SDA: GP_9
 * @I2C_SCL: GP_8
 */
static const am_hal_gpio_pincfg_t k_mag_iom1_scl = {
    .uFuncSel            = AM_HAL_PIN_8_M1SCL,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 1
};

static const am_hal_gpio_pincfg_t k_mag_iom1_sda = {
    .uFuncSel            = AM_HAL_PIN_9_M1SDAWIR3,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 1
}; 


/*
 * I/O Master (6D-G-sensor)
 *
 * @MISO: GP_6
 * @MOSI: GP_7
 * @SCK: GP_5
 * @CS: GP_3
 */
static const am_hal_gpio_pincfg_t k_6d_iom0_miso = {
    .uFuncSel            = AM_HAL_PIN_6_M0MISO,
    .bIomMSPIn           = 1,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_6d_iom0_mosi = {
    .uFuncSel            = AM_HAL_PIN_7_M0MOSI,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .bIomMSPIn           = 1,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_6d_iom0_sck = {
    .uFuncSel            = AM_HAL_PIN_5_M0SCK,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .bIomMSPIn           = 1,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_6d_iom0_cs __unused = {
    .uFuncSel            = AM_HAL_PIN_3_NCE3,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 1,
    .uIOMnum             = 0,
    .uNCE                = 0,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
};
    

/* 
 * LCD 
 */
static const struct io_config lcd_gpios[] = {
	_IO_ITEM("lcd_vbus_en", 56, IO_LOW, &k_gpio_output_max),
	_IO_ITEM("lcd_vcc_en", 55, IO_HIGH, &k_gpio_output),
	_IO_ITEM("lcd_te", 28, IO_LOW, &k_gpio_intr),
	_IO_ITEM("lcd_id0", 58, IO_LOW, NULL),
	_IO_ITEM("lcd_det", 72, IO_LOW, &k_gpio_output),
	_IO_ITEM("lcd_swire", 14, IO_NONE, &k_gpio_disable),

	_IO_ITEM("lcd_ce0", 19, IO_NONE, &k_lcd_mspi0_ce0),
	_IO_ITEM("lcd_d0", 22, IO_NONE, &k_lcd_mspi0_d0),
	_IO_ITEM("lcd_d1", 26, IO_NONE, &k_lcd_mspi0_d1),
	_IO_ITEM("lcd_d2", 4, IO_NONE, &k_lcd_mspi0_d2),
	_IO_ITEM("lcd_d3", 23, IO_NONE, &k_lcd_mspi0_d3),
	_IO_ITEM("lcd_sck", 24, IO_NONE, &k_lcd_mspi0_sck),
	_NULL_IO,
};

/* 
 * Touch pannel 
 */
static const struct io_config tp_gpios[] = {
#if defined(CONFIG_BOARD_PATCH_V0)
	_IO_ITEM("tp_vbus_en", 60, IO_LOW, &k_gpio_output_max),
#else
    _IO_ITEM("tp_vbus_en", 60, IO_HIGH, &k_gpio_output_max),
#endif
	_IO_ITEM("tp_reset", 61, IO_HIGH, &k_gpio_output),
	_IO_ITEM("tp_en", 57, IO_HIGH, &k_gpio_output_max),
	_IO_ITEM("tp_sda", 40, IO_NONE, &k_tp_iom4_sda),
	_IO_ITEM("tp_scl", 39, IO_NONE, &k_tp_iom4_scl),
	_IO_ITEM("tp_int", 46, IO_NONE, &k_gpio_intr),
	_NULL_IO,
};

/* 
 * Flash 
 */
static const struct io_config flash_gpios[] = {
    _IO_ITEM("flash_en", 2, IO_LOW, &k_gpio_output),
    _IO_ITEM("flash_ce0", 15, IO_NONE, &k_flash_mspi1_ce0),
    _IO_ITEM("flash_d0",  51, IO_NONE, &k_flash_mspi1_d0),
    _IO_ITEM("flash_d1",  52, IO_NONE, &k_flash_mspi1_d1),
    _IO_ITEM("flash_d2",  53, IO_NONE, &k_flash_mspi1_d2),
    _IO_ITEM("flash_d3",  54, IO_NONE, &k_flash_mspi1_d3),
    _IO_ITEM("flash_sck", 59, IO_NONE, &k_flash_mspi1_sck),
    _NULL_IO  
};

/* 
 * PSRAM 
 */
static const struct io_config psram_gpios[] = {
    _IO_ITEM("psram_en", 62, IO_LOW, &k_gpio_output),
    _IO_ITEM("psram_ce0", 63, IO_NONE, &k_psram_mspi2_ce0),
    _IO_ITEM("psram_d0",  64, IO_NONE, &k_psram_mspi2_d0),
    _IO_ITEM("psram_d1",  65, IO_NONE, &k_psram_mspi2_d1),
    _IO_ITEM("psram_d2",  66, IO_NONE, &k_psram_mspi2_d2),
    _IO_ITEM("psram_d3",  67, IO_NONE, &k_psram_mspi2_d3),
    _IO_ITEM("psram_sck", 68, IO_NONE, &k_psram_mspi2_sck),
    _NULL_IO    
};

/* 
 * 6D G-SENSOR 
 */
static const struct io_config gsensor_gpios[] = {
    _IO_ITEM("gs_en",   1, IO_LOW, &k_gpio_output),
    _IO_ITEM("gs_intr1",  18, IO_NONE,   &k_gpio_intr),
    _IO_ITEM("gs_intr2",  17, IO_NONE,   &k_gpio_intr),
    _IO_ITEM("gs_cs",  3, IO_NONE,   &k_gpio_output), //TODO: Is gpio?
    _IO_ITEM("gs_mosi", 7, IO_NONE, &k_6d_iom0_mosi),
    _IO_ITEM("gs_miso", 6, IO_NONE, &k_6d_iom0_miso),
    _IO_ITEM("gs_sck",  5, IO_NONE,    &k_6d_iom0_sck),
    _NULL_IO  
};

/*
 * HeartRate
 */
static const struct io_config heartrate_gpios[] = {
    _IO_ITEM("hrled_en", 50, IO_NONE, &k_gpio_output),
    _IO_ITEM("hr1_8v_en", 33, IO_NONE, &k_gpio_output),
    _IO_ITEM("hr_intr", 32, IO_NONE, &k_gpio_intr),
    _IO_ITEM("hr_sda", 25, IO_NONE, &k_hr_iom2_sda),
    _IO_ITEM("hr_scl", 27, IO_NONE, &k_hr_iom2_scl),
    _NULL_IO    
};

/*
 * Magnetic
 */
static const struct io_config magnetic_gpios[] = {
    _IO_ITEM("mag_en",  13, IO_LOW, &k_gpio_output),
    _IO_ITEM("mag_sda", 9, IO_NONE, &k_mag_iom1_sda),
    _IO_ITEM("mag_scl", 8, IO_NONE, &k_mag_iom1_scl),
    _NULL_IO   
};

/*
 * ChargeIC
 */
static const struct io_config charge_gpios[] = {
    _IO_ITEM("charge_sda", 43, IO_NONE, &k_charge_iom3_sda),
    _IO_ITEM("charge_scl", 42, IO_NONE, &k_charge_iom3_scl),
    _NULL_IO    
};

/*
 * Battery ADC
 */
static const struct io_config battery_gpios[] = {
    _IO_ITEM("bat_adc", 12, IO_NONE, NULL),
    _NULL_IO   
};

/* 
 * UART 1
 */
static const struct io_config uart0_gpios[] = {
    _IO_ITEM("uart0_tx",  48, IO_NONE, &k_uart0_tx),
    _IO_ITEM("uart0_rx",  49, IO_NONE, &k_uart0_rx),
    _NULL_IO    
};

/*
 * GPIO Buttons
 */
static const struct io_config button_gpios[] = {
    _IO_ITEM("key2",  71, IO_NONE, &k_gpio_intr),
    _IO_ITEM("power_key", 16, IO_NONE, &k_gpio_intr),
    _NULL_IO    
};

/*
 * Motor 
 */
static const struct io_config motor_gpios[] = {
    _IO_ITEM("motor",  45, IO_LOW, &k_gpio_output),
    _NULL_IO    
};
/*
 * Board GPIO configure information
 */
static const struct io_array board_gpio_array[] = {
    _BOARD_IO(lcd_gpios),
    _BOARD_IO(tp_gpios),
    _BOARD_IO(flash_gpios),
    _BOARD_IO(psram_gpios),
    _BOARD_IO(gsensor_gpios),
    _BOARD_IO(heartrate_gpios),
    _BOARD_IO(magnetic_gpios),
    _BOARD_IO(charge_gpios),
    _BOARD_IO(battery_gpios),
    _BOARD_IO(uart0_gpios),
    _BOARD_IO(button_gpios),
    _BOARD_IO(motor_gpios),

     {NULL} /* End */
};

static int find_pin(const struct io_config *io, const char *name)
{
    while (io->name) {
        if (!strcmp(io->name, name))
            return io->pin;
        io++;
    }
    return -EINVAL;
}

/*
 * Export (soc.h)
 */
int soc_get_pin(const char *name)
{
    const struct io_array *ioa;
    if (name == NULL)
        return -EINVAL;
    
    ioa = &board_gpio_array[0];
    while (ioa->io) {
        if (ioa->io->name[0] == name[0] &&
            ioa->io->name[1] == name[1])
            return find_pin(ioa->io, name);
        ioa++;
    }
    return -EINVAL;
}

static void config_io_array(const struct io_config *io)
{
    while (io->name) {
        am_hal_gpio_pinconfig(io->pin, *io->cfg);
        switch (io->ival) {
        case IO_LOW:
            am_hal_gpio_output_clear(io->pin);
            break;
        case IO_HIGH:
            am_hal_gpio_output_set(io->pin);
            break;
        default:
            break;
        }
        io++;
    }    
}

static void apollo3p_gpio_init(void)
{
    const struct io_array *ioa = &board_gpio_array[0];
    while (ioa->io)	{
        config_io_array(ioa->io);
        ioa++;
    }
}

static int apollo3p_board_init(const struct device *port)
{
    am_hal_burst_avail_e burst;
    (void) port;
       
    /* Set the clock frequency */
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);

    /* Set the default cache configuration */
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    /* Configure the board for low power. */
    am_hal_pwrctrl_low_power_init();
    am_hal_burst_mode_initialize(&burst);
    
    if (burst == AM_HAL_BURST_AVAIL) {
        am_hal_burst_mode_e burst_state;
        am_hal_burst_mode_disable(&burst_state);
        am_hal_burst_mode_enable(&burst_state);
    }
    
    apollo3p_gpio_init();
    am_hal_flash_delay( FLASH_CYCLES_US(1000) );
    am_hal_interrupt_master_enable();
    return 0;
}

SYS_INIT(apollo3p_board_init, 
        PRE_KERNEL_1, 2);
