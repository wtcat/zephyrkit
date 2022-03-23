#ifndef APP_TIMER_H
#define APP_TIMER_H


//*****************************************************************************
//	app_timer.h
//		应用定时器
//
// Copyright (c) 2018, Idoo
// All rights reserved.
//
// 修订历史：
//		2018.6.22, Yannick: 创建，根据 nordic 移植 FreeRtos 代码
//		2018.7.21, yannick: 修订，优化为 eventgroup 方式
//      2021.4.10, jiangbin: 修订，适配成zephyr实现，并适当优化
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <zephyr.h>
//#include <drivers/hrtimer.h>

#include "timer_ii.h"

/* Convert milliseconds to timer ticks. */
#define APP_TIMER_TICKS(MS) MS

//Application time-out handler type.
typedef void(*app_timer_timeout_handler_t) (void * p_context);

/**@brief This structure keeps information about osTimer.*/
typedef struct
{
	union {
    	struct k_timer          timer;
		struct timer_struct     timer_ii;
	};
    void *						argument;

    //time out(ms)
    uint32_t                    timeout;
    app_timer_timeout_handler_t	func;
    /*保证定时器是在运行状态下，绕过 FreeRTOS 一些问题 */
	uint8_t                     active:1;
	uint8_t                     single_shot:1;
	uint8_t					    isr_context:1;
	uint8_t                     is_timer_ii:1;
}app_timer_t;

/* Timer ID type.
 * Never declare a variable of this type, but use the macro @ref APP_TIMER_DEF instead.*/
typedef app_timer_t * app_timer_id_t;

/* Create a timer identifier and statically allocate memory for the timer.
 *
 * @param timer_id Name of the timer identifier variable that will be used to control the timer.
 */
#define APP_TIMER_DEF(timer_id)                                  \
    static app_timer_t timer_id##_data = {0};                  \
    static const app_timer_id_t timer_id = &timer_id##_data

/**@brief Timer modes. */
	
#define APP_TIMER_ISR_CONTEXT 0x80
#define APP_TIMER_MODE_MASK   0x0F
#define APP_TIMER_MODE(m) ((m) & APP_TIMER_MODE_MASK)
typedef enum 
{
    APP_TIMER_MODE_SINGLE_SHOT, 				/**< The timer will expire only once. */
    APP_TIMER_MODE_REPEATED, 					/**< The timer will restart each time it expires. */	    
    APP_TIMER_MODE_SINGLE_SHOT_ISR = APP_TIMER_MODE_SINGLE_SHOT | APP_TIMER_ISR_CONTEXT,
    APP_TIMER_MODE_REPEATED_ISR    = APP_TIMER_MODE_REPEATED | APP_TIMER_ISR_CONTEXT
} app_timer_mode_t;

/****************************************************************************/
/*	int app_timer_init(void)												*/
/*	模块初始化																	*/
/*	返回值是ERROR_CODE_SUCCESS或其它出错代码													*/
/****************************************************************************/
uint32_t app_timer_init(void);

/**@brief Function for creating a timer instance.
 *
 * @param[in]  p_timer_id        Pointer to timer identifier.
 * @param[in]  mode              Timer mode.
 * @param[in]  timeout_handler   Function to be executed when the timer expires.
 *
 * @retval     ERROR_CODE_SUCCESS               If the timer was successfully created.
 * @retval     ERROR_CODE_INVALID_PARAM   If a parameter was invalid.
 * @retval     ERROR_CODE_INVALID_STATE   If the application timer module has not been initialized or
 *                                       the timer is running.
 *
 * @note This function does the timer allocation in the caller's context. It is also not protected
 *       by a critical region. Therefore care must be taken not to call it from several interrupt
 *       levels simultaneously.
 * @note The function can be called again on the timer instance and will re-initialize the instance if
 *       the timer is not running.
 * @attention The FreeRTOS and RTX app_timer implementation does not allow app_timer_create to
 *       be called on the previously initialized instance.
 */
uint32_t app_timer_create(app_timer_id_t const *p_timer_id,
                       				app_timer_mode_t mode,
                          			app_timer_timeout_handler_t timeout_handler);

/**@brief Function for starting a timer.
 *
 * @param[in]		timer_id	  Timer identifier.
 * @param[in]		timeout_ms    Number of ms to time-out event
 * @param[in]		p_context	  General purpose pointer. Will be passed to the time-out handler when
 *								  the timer expires.
 *
 * @retval	   ERROR_CODE_SUCCESS				 If the timer was successfully started.
 * @retval	   ERROR_CODE_INVALID_PARAM	 If a parameter was invalid.
 * @retval	   ERROR_CODE_INVALID_STATE	 If the application timer module has not been initialized or the timer
 *										 has not been created.
 * @retval	   ERROR_CODE_NO_MEM 		 If the timer operations queue was full.
 *
 * @note For multiple active timers, time-outs occurring in close proximity to each other (in the
 *		 range of 1 to 3 ticks) will have a positive jitter of maximum 3 ticks.
 * @note When calling this method on a timer that is already running, the second start operation
 *		 is ignored.
 */
uint32_t app_timer_start(app_timer_id_t timer_id, uint32_t timeou_ms, void * p_context);

/**@brief Function for stopping the specified timer.
 *
 * @param[in]  timer_id 				 Timer identifier.
 *
 * @retval	   ERROR_CODE_SUCCESS				 If the timer was successfully stopped.
 * @retval	   ERROR_CODE_INVALID_PARAM	 If a parameter was invalid.
 * @retval	   ERROR_CODE_INVALID_STATE	 If the application timer module has not been initialized or the timer
 *										 has not been created.
 * @retval	   ERROR_CODE_NO_MEM 		 If the timer operations queue was full.
 */
uint32_t app_timer_stop(app_timer_id_t timer_id);

/**@brief Function for stopping all running timers.
 *
 * @retval	   ERROR_CODE_SUCCESS				 If all timers were successfully stopped.
 * @retval	   ERROR_CODE_INVALID_STATE	 If the application timer module has not been initialized.
 * @retval	   ERROR_CODE_NO_MEM 		 If the timer operations queue was full.
 */
uint32_t app_timer_stop_all(void);

/**@brief Function for get timer is valide.
 *
 * @param[in]  timer_id 				 Timer identifier.
 *
 * @retval	   1	valid
 * @retval	   0	invalid
 */
uint32_t app_timer_is_valide(app_timer_id_t timer_id);

#endif
