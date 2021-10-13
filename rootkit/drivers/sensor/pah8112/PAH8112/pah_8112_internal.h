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

#ifndef __pah_8112_internal_h__
#define __pah_8112_internal_h__


#include "pah_platform_types.h"
#include "pah_driver_types.h"


//============================================================================
// TYPE DEFINITIONS
//============================================================================
typedef enum {

    device_package_et = 0,
    device_package_es = 1,

} device_package_e;


//============================================================================
// VALUE DEFINITIONS
//============================================================================
#define PAH_PRODUCT_ID    0x11

#define PAH_I2C_SLAVE_ID  0x15   //I2C 7-bit ID


//============================================================================
// EXPRESSION MACRO DEFINITIONS
//============================================================================
#define PAH_ARRAY_SIZE(arr)         (sizeof(arr) / sizeof(arr[0]))

#define PAH_SET_BIT(var,pos)    do { ((var) |= (1 << (pos))); }while(0)
#define PAH_CLEAR_BIT(var,pos)  do { ((var) &= ~(1 << (pos))); }while(0)
#define PAH_CHECK_BIT(var,pos)  (((var) >> (pos)) & 1)

//============================================================================
// PUBLIC FUNCTION DECLARATIONS
//============================================================================
pah_ret     pah_8112_verify_product_id(void);

pah_ret     pah_8112_write_touch_package_setting(void);
pah_ret     pah_8112_write_reg_array(const uint8_t reg_array[][2], uint32_t length);
pah_ret     pah_8112_update_flag(void);

pah_ret     pah_8112_device_init(void);
pah_ret     pah_8112_startup(void);
pah_ret     pah_8112_shutdown(void);

pah_ret     pah_8112_read_device_package(device_package_e *device_package);


#endif  // header guard
