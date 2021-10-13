/*==============================================================================
* Edit History
* 
* This section contains comments describing changes made to the module. Notice
* that changes are listed in reverse chronological order. Please use ISO format
* for dates.
* 
* when       who       what, where, why
* ---------- ---       -----------------------------------------------------------
* 2016-07-07 bh        Initial revision.
==============================================================================*/

#ifndef __pah_driver_8112_reg_array_h__
#define __pah_driver_8112_reg_array_h__


#include "pah_platform_types.h"


typedef enum {
    pah8112_ppg_ch_a,
    pah8112_ppg_ch_b,
    pah8112_ppg_ch_c,
    pah8112_ppg_ch_num,
} pah8112_ppg_ch;


typedef enum {

    pah8112_reg_array_mode_none,
    pah8112_reg_array_mode_touch,
    pah8112_reg_array_mode_ppg,

} pah8112_reg_array_mode_e;

typedef enum {
	
    pah8112_alg_HR,
    pah8112_alg_VP,  
	pah8112_alg_SPO2,
	pah8112_alg_HRV,
	pah8112_alg_RR,
	pah8112_alg_RRI,
    pah8112_alg_RMSSD,
    pah8112_alg_IHB,
} pah8112_alg_mode;

bool        pah8112_reg_array_init(uint8_t mode);
bool        pah8112_reg_array_load_init(void);
bool        pah8112_reg_array_load_mode(pah8112_reg_array_mode_e reg_array_mode);

void        pah8112_get_ppg_ch_enabled(uint8_t ppg_ch_enabled[pah8112_ppg_ch_num]);
uint32_t    pah8112_get_ppg_ch_num(void);
uint8_t pah8112_get_setting_version(void);
uint8_t pah8112_get_setting_version_VP(void);    //V2.5
uint8_t pah8112_get_setting_version_SPO2(void);    //V2.5
uint8_t pah8112_get_setting_version_G100HZ(void);    //V2.6

#endif  // header guard
