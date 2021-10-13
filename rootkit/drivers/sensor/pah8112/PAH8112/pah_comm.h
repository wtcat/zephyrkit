/*==============================================================================
* Edit History
*
* This section contains comments describing changes made to the module. Notice
* that changes are listed in reverse chronological order. Please use ISO format
* for dates.
*
* when       who       what, where, why
* ---------- ---       -----------------------------------------------------------
* 2016-04-12 bh        Add license information and revision information
* 2016-04-07 bh        Initial revision.
==============================================================================*/

#ifndef __pah_comm_h__
#define __pah_comm_h__

#include "pah_platform_types.h"
#define NRF_LOG_MODULE_NAME "APP"
#if 0
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_delay.h"
#endif
#include "pah8series_config.h"

#ifdef _ALGO_NON_USE_HEAP_SIZE
extern uint32_t pah_alg_buffer[PAH_ALG_MAX_SIZE / sizeof(uint32_t)];
#endif

/*****************************************
 * Global Function Declaration
 *****************************************/
/**
 * @brief This API write data from I2C module.
 *
 * This API provides an interface to write data to the I2C device.
 *
 * @return Return ture is success. (Need be return ture.)
 */
bool pah_comm_write(uint8_t addr, uint8_t data);

/*****************************************
 * Global Function Declaration
 *****************************************/
/**
 * @brief This API read data from I2C module.
 *
 * This API provides an interface to read data to the I2C device.
 *
 * @return Return ture is success. (Need be return ture.)
 */
bool pah_comm_read(uint8_t addr, uint8_t *data);

/*****************************************
 * Global Function Declaration
 *****************************************/
/**
 * @briefThis API reads data from I2C module.
 *
 * This API provides an interface to reads data to the I2C device.
 * Read data maximum 255.
 *
 * @return Return ture is success. (Need be return ture.)
 */
bool pah_comm_burst_read(uint8_t addr, uint8_t *data, uint8_t num);

/*****************************************
 * Global Function Declaration
 *****************************************/
/**
 * @briefThis API reads data from I2C module.
 *
 * This API provides an interface to reads data to the I2C device .
 *
 * @return Return ture is success. (Need be return ture.)
 */
bool pah_comm_burst_read_fifo(uint8_t addr, uint8_t *data, uint16_t num, uint8_t *cks_out);

/**
 * @brief Macro to be used in a formatted string to a pass float number to the log.
 *
 * Macro should be used in formatted string instead of the %f specifier together with
 * @ref LOG_FLOAT macro.
 * Example: LOG_INFO("My float number" LOG_FLOAT_MARKER "\r\n", LOG_FLOAT(f)))
 */
#define LOG_FLOAT_MARKER "%d.%05d"

/**
 * @brief Macro for dissecting a float number into two numbers (integer and residuum).
 */
#define LOG_FLOAT(val) (int32_t)(val), (int32_t)(((val > 0) ? (val) - (int32_t)(val) : (int32_t)(val) - (val)) * 100000)

// V2.5
#if 0
#define debug_printf(...) printk(__VA_ARGS__)
#else
#define debug_printf(...)                                                                                              \
	do {                                                                                                               \
	} while (0)
#endif

#if 0
#define log_printf(...) printk(__VA_ARGS__)
#else
#define log_printf(...)                                                                                                \
	do {                                                                                                               \
	} while (0)
#endif

#endif // header guard
