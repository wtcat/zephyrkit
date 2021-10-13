/*==============================================================================
* Edit History
* 
* This section contains comments describing changes made to the module. Notice
* that changes are listed in reverse chronological order. Please use ISO format
* for dates.
* 
* when       who       what, where, why
* ---------- ---       -----------------------------------------------------------
* 2016-12-30 bh        - Move Setting config. and  MODE DEFINITION
* 2016-09-26 bh        - Initial revision.
==============================================================================*/

#ifndef __pah_driver_types_h__
#define __pah_driver_types_h__


#include "pah_platform_types.h"
#include "pah8series_config.h"

#if defined(ENABLE_PXI_ALG_HRD)
#include "pah8series_api_c.h"
#else
typedef struct {
    uint8_t                     frame_count;
    uint32_t                    time;
    uint8_t                     touch_flag;
    uint32_t                    nf_ppg_channel;
    uint32_t                    nf_ppg_per_channel;
    int32_t                     *ppg_data;
    uint32_t                    nf_mems;
    int16_t                     *mems_data;
} pah8series_data_t;
#endif

typedef enum {

    pah_alg_mode_default,
    pah_alg_mode_HR = pah_alg_mode_default,    // HR Detect
    pah_alg_mode_VP ,                     // VP
	pah_alg_mode_SPO2 ,                     // SPO2
	pah_alg_mode_HRV ,                     // HRV
	pah_alg_mode_RR ,                     // RR
    pah_alg_mode_RRI ,                        // RRI
    pah_alg_mode_RMSSD ,                         // SQ_RMSSD
    pah_alg_mode_IHB ,                          // IHB	
} pah_alg_mode_e;


typedef enum {

    pah_stream_default,
    pah_stream_dri = pah_stream_default,    // Data ready interrupt
    pah_stream_polling,                     // Polling

} pah_stream_e;


typedef enum {

    pah_intshape_pulse_type_default,
    pah_intshape_pulse_type_edge = pah_intshape_pulse_type_default,   // Edge-sensitive
    pah_intshape_pulse_type_level,                                    // Level-sensitive

} pah_intshape_pulse_type_e;


typedef enum {

    pah_ppg_led_on_default,
    pah_ppg_led_on_instantly = pah_ppg_led_on_default,    // Turn on LED when enabled PPG
    pah_ppg_led_on_deferred,                              // Turn on LED when enabled PPG and detected first touch 

} pah_ppg_led_on_e;



typedef enum {

    pah_none,                   // Not used

    pah_stop_mode,              // Stop mode

    pah_touch_mode,             // Touch mode
	
    pah_ppg_mode,               // PPG mode
    
    pah_ppg_touch_mode,         // PPG + Touch mode
    
} pah_mode;


typedef enum {

    pah_device_ppg,             // PPG device
    pah_device_touch,           // Touch device

} pah_device;


typedef enum {

    pah_success,                // Success
    pah_pending,                // Do nothing. Usually happens when task() detected no interrupt in dri mode or fifo number is not enough.

    pah_err_unknown,            // Unknown error
    pah_err_invalid_argument,   // Invalid argument to function
    pah_err_invalid_operation,  // Invalid operation to function
    pah_err_not_implement,      // Not implemented in this driver
    pah_err_platform_fail,      // Platform function failed
    pah_err_comm_fail,          // Communication(I2C, SPI, ...) failed

    pah_err_invalid_program,    // Generally this indicates a bug in the driver

    pah_err_verify_device_fail, // Verify device failed

    pah_err_fifo_checksum_fail, // Failed to compare check sum with fifo data
                                // Usually happens to bad data communication

    pah_err_fifo_overflow,      // Fifo in device is overflow
                                // Usually happens when pah_task() was too late to be called
    
    pah_err_fifo_underflow,     // Fifo in device is underflow

} pah_ret;
#define PAH_SUCCEEDED(ret)              (ret == pah_success)
#define PAH_SUCCEEDED_OR_PENDING(ret)   (ret == pah_success || ret == pah_pending)
#define PAH_FAILED(ret)                 (ret != pah_success)

#define pah_no_interrupt    pah_pending     // deprecated name


typedef struct {

    pah_stream_e                stream;
    pah_intshape_pulse_type_e   intshape_pulse_type;
    pah_ppg_led_on_e            ppg_led_on;
	pah_alg_mode_e              alg_mode;

} pah_flags_s;


typedef struct {

    uint8_t*    data;
    uint8_t     touch_flag;

} pah_report_fifo;

typedef void(*pah_report_fifo_handle)(void* user_data, pah_report_fifo* fifo);


typedef enum
{
    main_status_idle,
    main_status_start_healthcare,
} main_status_e;
//V2.5
typedef enum
{
	alg_status_idle,
    alg_status_process,
	alg_status_close,
	alg_status_finish,
	alg_status_data_weak,
} alg_status_e;
//V2.5
typedef struct {
		uint32_t    Wear_FC_total;		
		uint32_t    Wear_Num;
}wIndex_data_t;
//V2.5
typedef struct{
		uint8_t		trigger_event;
		uint8_t		view_hr_event;
		uint8_t		enable_timer;
}preprocessing;
//V2.5
typedef struct
{
    // status
    main_status_e       status;

    // pxialg
    void*				pxialg_buffer;
	void*               hr_pBuffer;
    pah8series_data_t	pxialg_data;
    bool                pxialg_has_init;
    uint64_t			last_report_time;
    float				vp_calibration_ready;
#if defined(ENABLE_PXI_ALG_HRD)
    wIndex_data_t   	wIndex_data;
#endif	
    // gsensor
#if defined(ENABLE_MEMS_ZERO)
    int16_t				mems_data[MAX_MEMS_SAMPLE_NUM * 3];
    
#else
    int16_t				mems_data[MAX_MEMS_SAMPLE_NUM * 3];
#endif
    uint32_t            Expo_time[3];
    uint8_t             LEDDAC[3];
    alg_status_e    	alg_status;
    uint8_t             Tuning;
    uint16_t            Touch_data;
} main_state_s;


enum{
    
	VP_Calibration = 0,
	VP_Phy_Calibration,		//V117
	VP_Estimation,
	VP_Phy_Estimation,		//V117
	VP_WithoutCal_Estimation,		//Without calibration
	VP_Suspend,
};

#endif  // header guard
