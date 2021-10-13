/*==============================================================================
* Edit History
* 
* This section contains comments describing changes made to the module. Notice
* that changes are listed in reverse chronological order. Please use ISO format
* for dates.
* 
* when       who       what, where, why
* ---------- ---       -----------------------------------------------------------
* 2016-04-29 bell      Initial revision.
==============================================================================*/

#ifndef __pah_hrd_function_h__
#define __pah_hrd_function_h__

/*****************************************
 * Header files including
 *****************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "pah_driver_types.h"


/*****************************************
 * Global Function Declaration
 *****************************************/
/**
 * @brief HRD init Function.
 *
 * If sensor power up , please do pah8series_ppg_dri_HRD_init() 
 * This Function link register and set PPG sensor to power down
 *
 * @return None.
 */
void pah8series_ppg_dri_HRD_init(void);

/**
 * @brief PPG HRD task Function.
 * @param has_interrupt_pah pointer to GPIO interrupt handler.
 * @param has_interrupt_button pointer to GPIO(button) interrupt handler.
 * @param interrupt_pah_timestamp pointer to System tick count.
 * @return None.
 */
void pah8series_ppg_dri_HRD_task(volatile bool* has_interrupt_pah,volatile bool* has_interrupt_button ,volatile uint64_t* interrupt_pah_timestamp);

/**
 * @brief Polling HRD init Function.
 *
 * If sensor power up , please do pah8series_ppg_polling_HRD_init() 
 * This Function link register and set PPG sensor to power down
 *
 * @return None.
 */
void pah8series_ppg_polling_HRD_init(void);

/**
 * @brief Polling HRD task Function.
 * @param has_interrupt_pah pointer to GPIO interrupt handler.
 * @param has_interrupt_button pointer to GPIO(button) interrupt handler.
 * @return None.
 */
void pah8series_ppg_polling_HRD_task(volatile bool* has_interrupt_pah,volatile bool* has_interrupt_button );

/**
 * @brief touch mode polling touch status Function.
 * @param has_interrupt_pah pointer to GPIO interrupt handler.
 * @param has_interrupt_button pointer to GPIO(button) interrupt handler.
 * @return None.
 */
void pah8series_touch_mode_polling_task(volatile bool* has_interrupt_pah,volatile bool* has_interrupt_button);

/**
 * @brief touch mode read interrupt to touch status Function.
 * @param has_interrupt_pah pointer to GPIO interrupt handler.
 * @param has_interrupt_button pointer to GPIO(button) interrupt handler.
 * @return None.
 */
void pah8series_touch_mode_dri_task(volatile bool* has_interrupt_pah,volatile bool* has_interrupt_button ,volatile uint64_t* interrupt_pah_timestamp);

/**
 * @brief Preprocessing HRD task Function.
 *
 * If customer want to be fast get the first Heart rate value. 
 * This function using button simulation as event trigger, total of two events.   
 * "Button_1" as trigger start Event.--> Developer can be using button,touch panel and accelerometer used as start event. 
 * "Button_2" as Required to provide HR Event. --> Convert to HR page on the smart hand ring, trigger this event.
 * After that trigger start Event developer Join this Preproc task to task scheduling.
 *  
 * @param has_interrupt_pah pointer to GPIO interrupt handler.
 * @param has_interrupt_button pointer to GPIO(button) interrupt handler.
 * @param interrupt_pah_timestamp pointer to System tick count.
 * @return None.
 */
void pah8series_ppg_preproc_HRD_task(volatile bool*	has_interrupt_pah,volatile bool* has_interrupt_button,volatile uint64_t* interrupt_pah_timestamp);

/**
 * @brief HRD Start Function.
 *
 * If customer want to use PPG Sensor , please call pah8series_ppg_HRD_start()
 * This Function will enable the ppg sensor
 *
 * @return None.
 */
void pah8series_ppg_HRD_start(void);

/**
 * @brief HRD STOP Function.
 *
 * If customer want to stop PPG Sensor , please call pah8series_ppg_HRD_stop()
 * This Function will disable the ppg sensor
 *
 * @return None.
 */
void pah8series_ppg_HRD_stop(void);

/**
 * @brief Only Touch Mode Start Function.
 *
 * If customer want to use PPG Sensor to detect on-hand, please call pah8series_touch_mode_start()
 * This Function will enable the ppg sensor for touch detect
 *
 * @return None.
 */
void pah8series_touch_mode_start (void);

/**
 * @brief Alg cal function
 *
 * When FIFO Data read success, please call this function.
 * This Function will run pixart hrd alg.
 *
 * @return None.
 */
void hrd_alg_task(void);

#endif  // header guard

