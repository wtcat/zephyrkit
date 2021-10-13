#ifndef __PAH8SERIES_CONFIG_C_H__
#define __PAH8SERIES_CONFIG_C_H__

#include <stdint.h>

/*============================================================================
SOME DEFINITION
============================================================================*/
#define PPG_IR_CH_NUM 0

//#define ENABLE_MEMS_ZERO
#define MAX_MEMS_SAMPLE_NUM 100

#define ENABLE_PXI_ALG_HRD
#define ENABLE_PXI_LINK_HRD_REG
#define _WEAR_INDEX_EN

//#define ENABLE_PXI_ALG_VP
//#define ENABLE_PXI_LINK_VP_REG

#define ENABLE_PXI_ALG_SPO2
#define ENABLE_PXI_LINK_SPO2_REG
#define PPG_BUF_MAX_SPO2 150

#define Optimize_SPO2_flow		   // add item for V569
#define MEMS_RMS_MAX_TH 2200	   // MEMS Motion RMS Threshold general
#define MEMS_RMS_MAX_BREAK_TH 5500 // MEMS Motion RMS Threshold Maximum

#define COUNT_TIME_LESS 13
#define MEMS_ODR_MAX 600 // add item  //need set MEMS_ODR+1

#define ENABLE_PXI_ALG_HRV
#define ENABLE_PXI_LINK_HRV_REG
#define HRV_COLLECTION_SECONDS 180

#define ENABLE_PXI_ALG_STRESS
#define ENABLE_PXI_LINK_STRESS_REG
#define STRESS_COLLECTION_SECONDS 60

//#define ENABLE_PXI_ALG_RR
//#define ENABLE_PXI_LINK_RR_REG

#define ALG_GSENSOR_MODE 3 // 0:2G, 1:4G, 2:8G, 3:16G

//******Always Run PPG Mode******//
//#define PPG_MODE_ONLY

//******Lock sensor report Rate to target******//
#define Timing_Tuning

//******Without IR LED ,Use Green to Detect Touch******//
//#define Touch_Detect_Green_Led

//****** Demo PAH8series_8112 mode ******//
//#define demo_ppg_dri_en
//#define demo_ppg_polling_en
//#define demo_HRD_testpattern_en

//#define demo_touch_mode_polling_en
//#define demo_touch_mode_dri_en

//#define demo_ppg_dri_en_VP
//#define demo_ppg_polling_en_VP

#define demo_ppg_dri_en_SPO2
//#define demo_ppg_polling_en_SPO2
//#define demo_SPO2_testpattern_en

//#define demo_ppg_dri_en_HRV
//#define demo_ppg_polling_en_HRV
//#define demo_HRV_testpattern_en

//#define demo_ppg_dri_en_STRESS
//#define demo_ppg_polling_en_STRESS

//#define demo_ppg_dri_en_RR
//#define demo_ppg_polling_en_RR
//#define demo_RR_testpattern_en

//#define demo_preprocessing_en
//#define demo_factory_mode_en

//******NON USE HEAP SIZE , USE STATIC RAM FOR ALL PAH ALG******//
#define _ALGO_NON_USE_HEAP_SIZE
#define PAH_ALG_MAX_SIZE 0x7000
/*============================================================================
VALUE MACRO DEFINITIONS
============================================================================*/
#define PAH_DRIVER_8112_VERSION 1009

#define DEFAULT_REPORT_SAMPLE_NUM_PER_CH 20

#define MAX_SAMPLES_PER_READ 150
#define MAX_CH_NUM 3
#define BYTES_PER_SAMPLE 4

#define PPG_FRAME_INTERVAL_MS 60 // 50 * 120%

#define I2C_SLAVE_ID 0x15 // I2C 7-bit ID

/*============================================================================
OPTION MACRO DEFINITIONS
============================================================================*/
#define ENABLE_FIFO_CHECKSUM

/*============================================================================
Sensor & Cover Type DEFINITION
============================================================================*/

//-------Sensor IC Type---------//
#define __PAH8112ES

//-------Cover Type---------//
#define _DI_WALL_COVER

//-------IO Interface---------//
#define _I2C
//#define _SPI

//-------WITH/WO RUBBER---------//
//#define _WO_RUBBER
//#define _WITH_RUBBER

//-------WITH/WO NOTCH---------//
//#define _WO_NOTCH
//#define _WITH_NOTCH

#if defined(__PAH8112ES)
#define FACTORY_TEST_ES

#if defined(_DI_COVER)
#define __PAH8112ES_DI_COVER
#define SETTING_VERSION 0x01
#endif
#if defined(_DI_WALL_COVER)
#define __PAH8112ES_DI_WALL_COVER
#define SETTING_VERSION 0x11
#endif
#endif

#if defined(ENABLE_PXI_ALG_HRD)
#if defined(_WEAR_INDEX_EN)
#define WEAR_INDEX_EN
#endif
#endif

#endif // __PAH8SERIES_CONFIG_C_H__
