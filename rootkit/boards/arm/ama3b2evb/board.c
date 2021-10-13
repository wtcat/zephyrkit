/*
 * Copyright (c) 2018 Yurii Hamann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/sys_io.h>


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

#define _IO_ITEM(_name, _pin, _ival, _cfg) {\
    .name = _name, \
    .pin = _pin, \
    .ival = _ival, \
    .cfg = _cfg }

/*
 * GPIO configure for UART 
 */
static const am_hal_gpio_pincfg_t k_uart0_tx __unused = {
    .uFuncSel       = AM_HAL_PIN_22_UART0TX,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA
};

static const am_hal_gpio_pincfg_t k_uart0_rx __unused = {
    .uFuncSel       = AM_HAL_PIN_23_UART0RX
};

static const am_hal_gpio_pincfg_t k_uart1_tx __unused = {
    .uFuncSel		= AM_HAL_PIN_71_UART1TX,
    .eDriveStrength	= AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA
};

static const am_hal_gpio_pincfg_t k_uart1_rx __unused = {
    .uFuncSel		= AM_HAL_PIN_72_UART1RX
};


/*
 * GPIO configure for interrupt 
 */
static const am_hal_gpio_pincfg_t k_gpio_intr __unused = {
    .uFuncSel = 3,
    .eIntDir = AM_HAL_GPIO_PIN_INTDIR_HI2LO,
    .eGPInput = AM_HAL_GPIO_PIN_INPUT_ENABLE,
};	

/*
 * GPIO congiure for output
 */
static const am_hal_gpio_pincfg_t k_gpio_output __unused = {
    .uFuncSel       = 3,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL
};

/*
 * GPIO configure for MSPI0
 */
static const am_hal_gpio_pincfg_t k_mspi0_ce0 __unused = {
    .uFuncSel            = AM_HAL_PIN_10_NCE10,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0,
    .uNCE                = 0,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
};

static const am_hal_gpio_pincfg_t k_mspi0_d0 __unused = {
    .uFuncSel            = AM_HAL_PIN_22_MSPI0_0,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_mspi0_d1 __unused = {
    .uFuncSel            = AM_HAL_PIN_26_MSPI0_1,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_mspi0_d2 __unused = {
    .uFuncSel            = AM_HAL_PIN_4_MSPI0_2,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_mspi0_d3 __unused = {
    .uFuncSel            = AM_HAL_PIN_23_MSPI0_3,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

static const am_hal_gpio_pincfg_t k_mspi0_sck __unused = {
    .uFuncSel            = AM_HAL_PIN_24_MSPI0_8,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 0
};

/*
 * I/O Master
 */
static const am_hal_gpio_pincfg_t k_iom2_scl __unused = {
    .uFuncSel            = AM_HAL_PIN_27_M2SCL,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 2
};

static const am_hal_gpio_pincfg_t k_iom2_sda __unused = {
    .uFuncSel            = AM_HAL_PIN_25_M2SDAWIR3,
    .ePullup             = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
    .bIomMSPIn           = 1,
    .uIOMnum             = 2
};

/*
 * MSPI1
 */
static const am_hal_gpio_pincfg_t k_mspi1_ce0 __unused = {
    .uFuncSel            = AM_HAL_PIN_50_NCE50,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1,
    .uNCE                = 0,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
};

static const am_hal_gpio_pincfg_t k_mspi1_d0 __unused = {
    .uFuncSel            = AM_HAL_PIN_51_MSPI1_0,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};

static const am_hal_gpio_pincfg_t k_mspi1_d1 __unused = {
    .uFuncSel            = AM_HAL_PIN_52_MSPI1_1,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};

static const am_hal_gpio_pincfg_t k_mspi1_d2 __unused = {
    .uFuncSel            = AM_HAL_PIN_53_MSPI1_2,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};

static const am_hal_gpio_pincfg_t k_mspi1_d3 __unused = {
    .uFuncSel            = AM_HAL_PIN_54_MSPI1_3,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};


static const am_hal_gpio_pincfg_t k_mspi1_sck __unused = {
    .uFuncSel            = AM_HAL_PIN_59_MSPI1_8,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 0,
    .uIOMnum             = 1
};

/*
 * IOM4 (SPI)
 */
static const am_hal_gpio_pincfg_t k_iom4_spi_miso __unused = {
    .uFuncSel            = AM_HAL_PIN_40_M4MISO,
    .bIomMSPIn           = 1,
    .uIOMnum             = 4
};

static const am_hal_gpio_pincfg_t k_iom4_spi_mosi __unused = {
    .uFuncSel            = AM_HAL_PIN_44_M4MOSI,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .bIomMSPIn           = 1,
    .uIOMnum             = 4
};

static const am_hal_gpio_pincfg_t k_iom4_spi_sck __unused = {
    .uFuncSel            = AM_HAL_PIN_39_M4SCK,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .bIomMSPIn           = 1,
    .uIOMnum             = 4
};

const am_hal_gpio_pincfg_t k_iom4_spi_cs __unused = {
    .uFuncSel            = AM_HAL_PIN_31_NCE31,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .bIomMSPIn           = 1,
    .uIOMnum             = 4,
    .uNCE                = 0,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
};

/*
 * Board GPIO configure information
 */
static const struct io_config board_gpio[] = {
#if defined(CONFIG_BOARD_APOLLO3P)
   /* LCD */
    _IO_ITEM("pwr_1.8v", 69, IO_HIGH, &k_gpio_output),
    _IO_ITEM("pwr_3.3v", 34, IO_HIGH, &k_gpio_output),

    _IO_ITEM("lcd_reset", 28, IO_LOW,  &k_gpio_output),
    //_IO_ITEM("lcd_te",    30, IO_NONE, &k_gpio_intr),

    _IO_ITEM("mspi0_ce0", 10, IO_NONE, &k_mspi0_ce0),
    _IO_ITEM("mspi0_d0",  22, IO_NONE, &k_mspi0_d0),
    _IO_ITEM("mspi0_d1",  26, IO_NONE, &k_mspi0_d1),
    _IO_ITEM("mspi0_d2",  4,  IO_NONE, &k_mspi0_d2),
    _IO_ITEM("mspi0_d3",  23, IO_NONE, &k_mspi0_d3),
    _IO_ITEM("mspi0_sck", 24, IO_NONE, &k_mspi0_sck),

    /* Touch pannel */
    _IO_ITEM("tp_reset", 61, IO_HIGH, &k_gpio_output),
    //_IO_ITEM("tp_int", 62, IO_NONE, &k_gpio_intr),  

    _IO_ITEM("tp_sda", 25, IO_NONE, &k_iom2_sda),
    _IO_ITEM("tp_scl", 27, IO_NONE, &k_iom2_scl), 

    /* Flash */
    _IO_ITEM("flash_1.8v", 46, IO_LOW, &k_gpio_output),
    _IO_ITEM("mspi1_ce0", 50, IO_NONE, &k_mspi1_ce0),
    _IO_ITEM("mspi1_d0",  51, IO_NONE, &k_mspi1_d0),
    _IO_ITEM("mspi1_d1",  52, IO_NONE, &k_mspi1_d1),
    _IO_ITEM("mspi1_d2",  53, IO_NONE, &k_mspi1_d2),
    _IO_ITEM("mspi1_d3",  54, IO_NONE, &k_mspi1_d3),
    _IO_ITEM("mspi1_sck", 59, IO_NONE, &k_mspi1_sck),

    /* G-SENSOR */
    _IO_ITEM("gsensor_1v8",   16, IO_HIGH, &k_gpio_output),
    _IO_ITEM("gsensor_cs",    31, IO_NONE,  &k_iom4_spi_cs),
   //_IO_ITEM("iom4_spi_cs",    13, IO_NONE, &k_iom4_spi_cs),
    _IO_ITEM("iom4_spi_sck",  39, IO_NONE, &k_iom4_spi_sck),
    _IO_ITEM("iom4_spi_mosi", 44, IO_NONE, &k_iom4_spi_mosi),
    _IO_ITEM("iom4_spi_miso", 40, IO_NONE, &k_iom4_spi_miso),
    
    /* UART 1*/
    _IO_ITEM("uart1_tx",  71, IO_NONE, &k_uart1_tx),
    _IO_ITEM("uart1_rx",  72, IO_NONE, &k_uart1_rx),

#elif defined(CONFIG_BOARD_AMA3B2EVB)
    _IO_ITEM("uart0_tx",  22, IO_NONE, &k_uart0_tx),
    _IO_ITEM("uart0_rx",  23, IO_NONE, &k_uart0_rx),

#endif /* CONFIG_WATCH_BOARD_V0 */
};

static void apollo3p_gpio_init(void)
{
    for (int i = 0; i < ARRAY_SIZE(board_gpio); i++) {
        const struct io_config *gpio = board_gpio + i;

        am_hal_gpio_pinconfig(gpio->pin, *gpio->cfg);
        switch (gpio->ival) {
        case IO_LOW:
            am_hal_gpio_output_clear(gpio->pin);
            break;
        case IO_HIGH:
            am_hal_gpio_output_set(gpio->pin);
            break;
        default:
            break;
        }
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
		
