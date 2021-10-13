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

#ifndef __pah_spo2_function_h__
#define __pah_spo2_function_h__


#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "pah_driver_types.h"

void pah8series_ppg_dri_SPO2_init(void);
void pah8series_read_task_spo2_dri(volatile bool* has_interrupt_pah,volatile bool* has_interrupt_button,volatile uint64_t* interrupt_pah_timestamp);


void pah8series_ppg_polling_SPO2_init(void);
void pah8series_read_task_spo2_polling(volatile bool* has_interrupt_pah,volatile bool* has_interrupt_button);

//void pah8series_ppg_SPO2_task(void);

void pah8series_ppg_SPO2_start(void);
void pah8series_ppg_SPO2_stop(void);

/**
 * @brief Alg cal function
 *
 * When FIFO Data read success, please call this function.
 * This Function will run pixart hrd alg.
 *
 * @return None.
 */
void spo2_alg_task(void);	//V2.5

//bool spo2_algorithm_process_run(void);
void spo2_algorithm_calculate_run(void);
#endif  // header guard
