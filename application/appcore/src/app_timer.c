//#include "ydy_boards_config.h"
//#if (DEBUG_PART_ENABLE ==1)
//#define DEBUG_INFO_ENABLE			1
//#define DEBUG_ERROR_ENABLE		1
//#define DEBUG_INFO_ENABLE		1
//#define DEBUG_FUN_ENABLE			0
//#endif


//#define DEBUG_STRING "[APP TIMER]"

#define TIMER_MERGE_ENABLE 

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <zephyr.h>

#include "app_timer.h"



#define ERROR_CODE_SUCCESS 0
#define ERROR_CODE_INVALID_PARAM 1
#define ERROR_CODE_INVALID_STATE 2

#define DEBUG_INFO(...)
#define APP_ERROR_CHECK_BOOL(...)

//hrtimer 的接口单位为us
#define HRTIMER_MSEC(MS) (MS * 1000)

static void app_timer_callback(struct k_timer *ttimer);

typedef struct
{
    app_timer_timeout_handler_t timeout_handler;
    void *                      p_context;
} app_timer_event_t;

#ifdef TIMER_MERGE_ENABLE
#include "timer_patch.h"
#endif

static void timeout_handler_scheduled_exec(void * p_event_data, uint16_t event_size)
{
    APP_ERROR_CHECK_BOOL(event_size == sizeof(app_timer_event_t));
    app_timer_event_t const * p_timer_event = (app_timer_event_t *)p_event_data;
	
    p_timer_event->timeout_handler(p_timer_event->p_context);
}

static void app_timer_callback(struct k_timer *ttimer)
{
    app_timer_t *ptimer = (app_timer_t*)(ttimer);

    if(ptimer->active == false)
    {
        return ;
    }
    
    if(ptimer->func == NULL)
    {
        return ;
    }

    if(ptimer->active)
    {
    	if (!ptimer->isr_context) {
			/*
			 * Task Context
			 */
	        ptimer->active = (ptimer->single_shot) ? false : true;
	        app_timer_event_t timer_event;
	        timer_event.p_context = ptimer->argument;
	        timer_event.timeout_handler = ptimer->func;

        DEBUG_INFO("app_timer_callback = %X",ptimer->func);

        uint32_t err_code;

	        //err_code = app_sched_event_put(&timer_event, sizeof(timer_event), timeout_handler_scheduled_exec);
	        //APP_ERROR_CHECK(err_code);
    	} else {
			/*
			 * Interrupt Context
			 */
			ptimer->func(ptimer->argument);
    	}

        if (!ptimer->single_shot)
        {
      #ifdef TIMER_MERGE_ENABLE
			abstract_timer_start(ptimer, ptimer->timeout);
	  #else
            k_timer_start(&ptimer->timer, K_MSEC(ptimer->timeout), K_NO_WAIT);
			//hrtimer_start(&ptimer->timer, HRTIMER_MSEC(ptimer->timeout), 0);
	  #endif
        }
    }
}

uint32_t app_timer_init()
{
    return ERROR_CODE_SUCCESS;
}

uint32_t app_timer_create(app_timer_id_t const *p_timer_id,
	                          app_timer_mode_t mode,
    	                      app_timer_timeout_handler_t timeout_handler)
{
    app_timer_t *ptimer = (app_timer_t*)(*p_timer_id);
    uint32_t    err_code = ERROR_CODE_SUCCESS;

    if((timeout_handler == NULL) || (p_timer_id == NULL))
    {
        return ERROR_CODE_INVALID_PARAM;
    }
    if(ptimer->active)
    {
        return ERROR_CODE_INVALID_STATE;
    }

	memset(ptimer, 0, sizeof(app_timer_t));

	ptimer->single_shot = (APP_TIMER_MODE(mode) == APP_TIMER_MODE_SINGLE_SHOT);
	ptimer->isr_context = !!(mode & APP_TIMER_ISR_CONTEXT);
	ptimer->func = timeout_handler;
 
    k_timer_init(&ptimer->timer, app_timer_callback, NULL);
	// hrtimer_init(&ptimer->timer,
	// 			 app_timer_callback,
	// 			 NULL);

    return err_code;
}


uint32_t app_timer_start(app_timer_id_t timer_id,
								uint32_t timeout_ms,
								void * p_context)
{
    app_timer_t * ptimer = (app_timer_t*)(timer_id);

    ptimer->timeout = timeout_ms;
    ptimer->argument = p_context;
    ptimer->active = true;
#ifdef TIMER_MERGE_ENABLE
	abstract_timer_reset(ptimer, timeout_ms);
	abstract_timer_start(ptimer, timeout_ms);
	abstract_timer_post(ptimer);
#else
    k_timer_start(&ptimer->timer, K_MSEC(timeout_ms), K_NO_WAIT);
	//hrtimer_start(&ptimer->timer, HRTIMER_MSEC(timeout_ms), 0);
#endif
    return ERROR_CODE_SUCCESS;
}

uint32_t app_timer_is_valide(app_timer_id_t timer_id)
{
	if(timer_id == NULL)
    {
        return 0;
    };
	
	return 1;
}

uint32_t app_timer_stop(app_timer_id_t timer_id)
{
    app_timer_t * ptimer = (app_timer_t*)(timer_id);
#ifdef TIMER_MERGE_ENABLE
	abstract_timer_stop(ptimer);
#else
    k_timer_stop(&ptimer->timer);
	//hrtimer_stop(&ptimer->timer);
#endif

    return ERROR_CODE_SUCCESS;
}
