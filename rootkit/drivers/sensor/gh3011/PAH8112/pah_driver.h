/*==============================================================================
* Edit History
* 
* This section contains comments describing changes made to the module. Notice
* that changes are listed in reverse chronological order. Please use ISO format
* for dates.
* 
* when       who       what, where, why
* ---------- ---       -----------------------------------------------------------
* 2016-12-30 jc        - Add pah_do_timing_tuning(),pah8112_setting_version pah8112_ae_info_read.
* 2016-09-20 bh        - Add pah_intshape_pulse_type_e flag.
* 2016-09-08 bh        - Add functions: pah_init_with_flags().
*                      - Add pah_ppg_led_on_e flag.
* 2016-09-05 bh        - Add polling mode.
*                      - Add functions: pah_init_with_flags(), pah_get_fifo_ch_num().
* 2016-06-07 bh        - Add functions: pah_set_mode(), pah_run_device().
*                      - Add enum: pah_device.
*                      - Add comments.
* 2016-04-29 bh        - Add PPG 200Hz modes.
*                      - Add helper functions: pah_is_ppg_mode(), pah_is_ppg_20hz_mode(), pah_fifo_data_num_per_ch().
*                      - Add pah_stop_mode
*                      - Remove pah_suspend_mode.
*                      - Fix setting pah_set_report_sample_num_per_ch() after enter_mode() causes bad behavior.
* 2016-04-20 bh        - Add pah_stop_mode. pah_none can be regarded as pah_stop_mode.
* 2016-04-12 bh        - Add license information and revision information.
* 2016-04-07 bh        - Initial revision.
==============================================================================*/

#ifndef __pah_driver_h__
#define __pah_driver_h__


#include "pah_platform_types.h"
#include "pah_driver_types.h"


/**
 * @brief Get default pah_flags_s.
 * 
 * @param[out] flags        Refer to struct pah_flags_s.
 */
void        pah_flags_default(pah_flags_s *flags);

/**
 * @brief Initialize the driver.
 * 
 * It should been called once before any other function calls.
 *
 * @return True if successful.
 */
bool        pah_init(void);

/**
 * @brief Initialize the driver.
 * 
 * It should been called once before any other function calls.
 *
 * @param[in]  flags        Refer to struct pah_flags_s.
 *
 * @return True if successful.
 */
bool        pah_init_with_flags(const pah_flags_s *flags);

/**
 * @brief De-initialize the driver.
 *
 * @return None.
 */
void        pah_deinit(void);


/**
 * @brief Set device into a given pah mode and run device.
 * 
 * It change device into a specific mode(refer to enum pah_mode) and then enable device to run if the mode depends on device.
 * Equal to the combination of pah_set_mode() and pah_run(), i.e
 *     pah_set_mode(pah_ppg_mode);
 *     pah_run(pah_device_ppg, true);
 *
 * @param[in]  mode         Refer to enum pah_mode.
 *
 * @return True if setting successfully.
 */
bool        pah_enter_mode(pah_mode mode);


/**
 * @brief Set device into a given pah mode.
 * 
 * It change device into a specific mode(refer to enum pah_mode).
 *
 * @param[in]  mode         Refer to enum pah_mode.
 *
 * @return True if setting successfully.
 */
bool        pah_set_mode(pah_mode mode);

/**
 * @brief Enable/Disable device to run.
 * 
 * It returns false if the current mode isn't activable, such as pah_stop_mode.
 *
 * @param[in]  device       Refer to enum pah_device.
 * @param[in]  enable       Enable/Disable the device.
 *
 * @return True if setting successfully.
 */
bool        pah_run_device(pah_device device, bool enable);


/**
 * @brief Query current mode.
 *
 * @return Refer to enum pah_mode.
 */
pah_mode    pah_query_mode(void);

/**
 * @brief Check if the device is in PPG mode.
 *
 * @return True if the device is in PPG mode.
 */
bool        pah_is_ppg_mode(void);

/**
 * @brief Check if the device is in PPG(20Hz) mode.
 *
 * @return True if the device is in PPG(20Hz) mode.
 */
bool        pah_is_ppg_20hz_mode(void);


/**
 * @brief Process tasks after INT1 interrupt.
 *
 * Whenever the device raises INT1 interrupt, this function must be called once to process tasks.
 * This return pah_ret stands for a erorr code, normally it should been pah_success.
 *
 * If it returns pah_success, you can handle with some tasks according to current mode.
 *
 * Else if it returns pah_no_interrupt, it means that the device didn't raise INT1 interrupt yet.
 * Generally there is an extrinsic factor raising the interrupt which has nothing to do with the device.
 *
 * @return pah_success if the open operation was done successfully.
 *         pah_no_interrupt if the device didn't raise interrupt yet.
 *         pah_err_fifo_overflow if pah_task() was too late to be called.
 *         Otherwise to indicate an error has occurred.
 */
pah_ret     pah_task(void);


/**
 * @brief Access current fifo data.
 * 
 * This information is updated after a successful pah_task().
 *
 * @return Location where fifo data is stored.
 */
uint8_t*    pah_get_fifo_data(void);

/**
 * @brief Read current length of fifo data.
 * 
 * This information is updated after a successful pah_task().
 *
 * @return The length(in bytes) of fifo data.
 */
uint32_t    pah_fifo_data_num_per_ch(void);

/**
 * @brief Check if there are fifo data.
 * 
 * This information is updated after a successful pah_task().
 *
 * @return True if there are fifo data.
 */
bool        pah_has_fifo_data(void);

/**
 * @brief Check fifo channel number.
 * 
 * This information is updated after a successful pah_task().
 *
 * @return Channel number.
 */
uint32_t    pah_get_fifo_ch_num(void);

/**
 * @brief Check if the device is on touch.
 * 
 * This information is updated after a successful pah_task().
 *
 * @return True if the device is on touch.
 */
bool        pah_is_touched(void);


/**
 * @brief Set all values of fifo data to 0.
 *
 * @return None.
 */
void        pah_clear_fifo_data(void);


/**
 * @brief Set report fifo callback.
 *
 * @param[in]  fp_handler   The callback function to be called when pah_task() receives new fifo data.
 * @param[in]  user_data    The user data to be passed as parameter of callback function.
 * 
 * @return None.
 */
void        pah_set_report_fifo_callback(pah_report_fifo_handle fp_handler, void* user_data);


/**
 * @brief Get the specific I2C slave address of the device.
 * 
 * @return I2C slave address of the device.
 */
uint16_t    pah_get_i2c_slave_addr(void);


/**
 * @brief Enable or disable INT2 as touch flag.
 * 
 * Default is disabled.
 *
 * By default, touch/no-touch detection raises a pulse interrupt to INT1 which shared with FIFO interrupt.
 * If INT2 as touch flag is enabled, the device on touch causes INT2 pull-up, otherwise pull-down.
 *
 * @param[in]  enable   True to enable, otherwise to disable.
 * 
 * @return True if setting successfully.
 */
bool        pah_set_int2_as_touch_flag(bool enable);


/**
 * @brief Set report number per channel.
 *
 * The report number determines how many fifo data number the device collects, the device will raise a fifo interrupt.
 * For example, in PPG(20Hz) the device generates one data per 50ms, thus setting report number 20 makes the device raise fifo interrupts each 50ms*20 = 1000ms.
 *
 * @param[in]  report_sample_num_per_ch   The report number per channel.
 * 
 * @return None.
 */
void        pah_set_report_sample_num_per_ch(uint32_t report_sample_num_per_ch);

/**
 * @brief Get report number per channel.
 *
 * @return The report number per channel.
 */
uint32_t    pah_get_report_sample_num_per_ch(void);

/**
 * @brief Get the maximum report number per channel.
 * 
 * Due to the limit of buffer length, the number passing to pah_set_report_sample_num_per_ch() must be less than a maximum value.
 *
 * @return The maximum report number per channel.
 */
uint32_t    pah_get_max_report_sample_num_per_ch(void);

/**
 * @brief Get how many bytes per fifo sample.
 *
 * @return Bytes per fifo sample.
 */
uint32_t    pah_get_bytes_per_sample(void);

/**
 * @brief Check if the driver is valid to the device.
 * 
 * This function reads device registers directly.
 *
 * @return True if the driver is valid to the device.
 */
bool        pah_verify_product_id(void);

/**
 * @brief Check if the device is on touch.
 * 
 * This function reads device registers directly.
 *
 * @param[out]  is_touched   True if the device is on touch.
 *
 * @return pah_success if the open operation was done successfully.
 *         Otherwise to indicate an error has occurred.
 */
pah_ret     pah_read_touch_flag(bool *is_touched);


/**
 * @brief Tuning Timing to make sample period accurate .
 *
 * @return None.
 */
//void pah_do_timing_tuning(int time_diff/*ms*/, int samples_per_ch);
void pah_do_timing_tuning(int time_diff/*ms*/, int samples_per_ch, float _time_target);
/**
 * @brief Get setting version  .
 *
 * @return Setting Version.
 */
uint8_t pah8112_setting_version(void);
#ifdef ENABLE_PXI_LINK_VP_REG
uint8_t pah8112_setting_version_VP(void);
#endif
#ifdef ENABLE_PXI_LINK_SPO2_REG
uint8_t pah8112_setting_version_SPO2(void);
#endif
/**
 * @brief Read AE INFO and touch value with determine the amount of change  .
 *
 * @return AE INFO.
 */
uint8_t pah8112_ae_info_read(uint32_t* Expo_time,uint8_t* LEDDAC, uint16_t *touch_data);


/**
 * @brief Get IR Touch Value
 * 
 * This function reads device registers directly.
 *
 * @param[out]  touch_value   IR Touch Value.
 *
 * @return pah_success if the open operation was done successfully.
 *         Otherwise to indicate an error has occurred.
 */
pah_ret pah_read_touch_value(uint32_t *touch_value);



#endif  // header guard
