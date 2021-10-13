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

#ifndef __pah_verify_h__
#define __pah_verify_h__


#include "pah_platform_types.h"
#include "pah_driver_types.h"


//============================================================================
// TYPE DEFINITIONS
//============================================================================
typedef enum {

    pah_verify_ch_t,
    pah_verify_ch_a,
    pah_verify_ch_b,
    pah_verify_ch_c,
    pah_verify_ch_num,

} pah_verify_ch_e;


typedef struct {

    uint16_t    value;

    uint16_t    min;
    bool        min_enabled_verify;
    bool        min_verified_success;

    uint16_t    max;
    bool        max_enabled_verify;
    bool        max_verified_success;

} pah_verify_ch_adc_data_s;


typedef struct {

    pah_verify_ch_adc_data_s ch_adc_data[pah_verify_ch_num];

} pah_verify_adc_data_s;


typedef struct {

	uint8_t    ch_a_expo_time;
    uint8_t    ch_b_expo_time;
    uint8_t    ch_c_expo_time;
    bool       expo_time_enabled_verify;
} pah_verify_expo_time_s;

//============================================================================
// PUBLIC FUNCTION DECLARATIONS
//============================================================================
pah_ret    pah_global_reset(void);

pah_ret    pah_verify_init(void);

uint32_t   pah_verify_get_i2c_slave_addr(void);


//-------------------------------------------------------------------
// adc data test
//-------------------------------------------------------------------


pah_ret    pah_verify_led_golden_only_read_adc_data(pah_verify_adc_data_s *adc_data, pah_verify_expo_time_s  *expo_time_data);
pah_ret    pah_verify_led_target_sample_read_adc_data(pah_verify_adc_data_s *adc_data);

pah_ret    pah_verify_adc_data_test(pah_verify_adc_data_s *adc_data);

pah_ret    pah_verify_light_leak_read_adc_data(pah_verify_adc_data_s *adc_data);
pah_ret    pah_verify_touch_value_read_adc_data(pah_verify_adc_data_s *adc_data);

pah_ret    pah_verify_INT_high(void);
pah_ret    pah_verify_INT_low(void);
pah_ret    pah_verify_power_noise(void);	//V2.5

#endif  // header guard
