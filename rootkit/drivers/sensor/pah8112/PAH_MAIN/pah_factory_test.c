



// c
#include <string.h>
#include <stdlib.h>
// pah
#include "pah_factory_test.h"
#include "pah_verify.h"
#include "pah_comm.h"
#include "pah8series_config.h"
#include "pah_driver_types.h"
#include "pah_driver.h"

/*============================================================================
STATIC FUNCTION PROTOTYPES
============================================================================*/
static bool demo_factory_test(factory_test_e factory_test,bool expo_en,uint8_t expo_ch_a, uint8_t expo_ch_b,uint8_t expo_ch_c);	//8112ES Update
static void demo_factory_test_print_adc_data(const pah_verify_adc_data_s *adc_data);
static void demo_factory_test_print_adc_data_verify_failed(const pah_verify_adc_data_s *adc_data);

//static void gpio_btn_interrupt_handler(GPIO_BTN gpio);
static void Error_Handler(void);

void factory_test_reset(void)
{
              
    pah_global_reset();

}


void factory_test_mode(factory_test_e test_type,bool expo_en,uint8_t expo_ch_a, uint8_t expo_ch_b,uint8_t expo_ch_c)	//8112ES Update
{
              
    demo_factory_test((factory_test_e)test_type,expo_en,expo_ch_a,expo_ch_b,expo_ch_c);		//8112ES Update

}

static bool demo_factory_test(factory_test_e factory_test,bool expo_en,uint8_t expo_ch_a,uint8_t expo_ch_b,uint8_t expo_ch_c)	//8112ES Update
{
    pah_ret                 ret = pah_err_unknown;
    pah_verify_adc_data_s   adc_data;
		int temp = 0;

    debug_printf("\n\n");
    
    ret = pah_verify_init();
    if (PAH_FAILED(ret))
        Error_Handler();
	pah_verify_expo_time_s  expo_time_data;
	expo_time_data.expo_time_enabled_verify = expo_en ;
	expo_time_data.ch_a_expo_time = expo_ch_a;	//8112 Update
	expo_time_data.ch_b_expo_time = expo_ch_b;
    expo_time_data.ch_c_expo_time = expo_ch_c;
	debug_printf("\n Setting Version %d\n", pah8112_setting_version());
    switch (factory_test)
    {
    case factory_test_light_leak:
        debug_printf("===== Factory test: Light Leak ===== \n");
        ret = pah_verify_light_leak_read_adc_data(&adc_data);
        if (PAH_FAILED(ret))
            Error_Handler();
        break;
    case factory_test_touch_value:
        debug_printf("===== Factory test: Touch Value ===== \n");
        ret = pah_verify_touch_value_read_adc_data(&adc_data);
        if (PAH_FAILED(ret))
            Error_Handler();
        break;
	case factory_test_INT_high:
        debug_printf("===== Factory test: INT_high ===== \n");
        ret = pah_verify_INT_high();
        if (PAH_FAILED(ret))
            Error_Handler();
        temp=1;
        //return true;
        //NOT REACHED
        break;
	case factory_test_INT_low:
        debug_printf("===== Factory test: INT_low ===== \n");
        ret = pah_verify_INT_low();
        if (PAH_FAILED(ret))
            Error_Handler();
        temp=1;
        //return true;
        //NOT REACHED
        break;	
	case factory_test_led_target_sample:
        debug_printf("===== Factory test: LED target sample ===== \n");
        ret = pah_verify_led_target_sample_read_adc_data(&adc_data);
        if (PAH_FAILED(ret))
            Error_Handler();
        break;
    case factory_test_led_golden_only:
        debug_printf("===== Factory test: LED golden only ===== \n");
		
        ret = pah_verify_led_golden_only_read_adc_data(&adc_data,&expo_time_data);
        if (PAH_FAILED(ret))
            Error_Handler();
        break;
    case factory_test_power_noise:	//V2.5
        debug_printf("===== Factory test: Power noise ===== \n");
        ret = pah_verify_power_noise();
        if (PAH_FAILED(ret))
        	Error_Handler();
        temp=1;
        break;
				
    default:
        debug_printf("Not implemented yet \n");
        return false;
    }
    if(temp)
		return true;
    
    demo_factory_test_print_adc_data(&adc_data);
        
    ret = pah_verify_adc_data_test(&adc_data);
		
		
	if (PAH_FAILED(ret))
    {
        demo_factory_test_print_adc_data_verify_failed(&adc_data);
        return false;
    }
    
    return true;
}

static void demo_factory_test_print_adc_data(const pah_verify_adc_data_s *adc_data)
{
    int ch = 0;
    
    if (!adc_data)
        Error_Handler();
    
    for (ch = 0; ch < pah_verify_ch_num; ++ch)
    {
        if (adc_data->ch_adc_data[ch].min_enabled_verify && adc_data->ch_adc_data[ch].max_enabled_verify)
        {
            debug_printf("factory test. ch = %d, (min, value, max) = (%d, %d, %d) \n",
                ch,
                adc_data->ch_adc_data[ch].min,
                adc_data->ch_adc_data[ch].value,
                adc_data->ch_adc_data[ch].max);
        }
        else if (adc_data->ch_adc_data[ch].min_enabled_verify)
        {
            debug_printf("factory test. ch = %d, (min, value) = (%d, %d) \n",
                ch,
                adc_data->ch_adc_data[ch].min,
                adc_data->ch_adc_data[ch].value);
        }
        else if (adc_data->ch_adc_data[ch].max_enabled_verify)
        {
            debug_printf("factory test. ch = %d, (value, max) = (%d, %d) \n",
                ch,
                adc_data->ch_adc_data[ch].value,
                adc_data->ch_adc_data[ch].max);
        }
    }
}

static void demo_factory_test_print_adc_data_verify_failed(const pah_verify_adc_data_s *adc_data)
{
    int ch = 0;
    
    if (!adc_data)
        Error_Handler();
    
    for (ch = 0; ch < pah_verify_ch_num; ++ch)
    {
        // min
        if (adc_data->ch_adc_data[ch].min_enabled_verify && !adc_data->ch_adc_data[ch].min_verified_success)
            debug_printf("factory test failed. ch = %d, value = %d, min = %d \n", ch, adc_data->ch_adc_data[ch].value, adc_data->ch_adc_data[ch].min);
        
        // max
        if (adc_data->ch_adc_data[ch].max_enabled_verify && !adc_data->ch_adc_data[ch].max_verified_success)
            debug_printf("factory test failed. ch = %d, value = %d, max = %d \n", ch, adc_data->ch_adc_data[ch].value, adc_data->ch_adc_data[ch].max);
    }
}

static void Error_Handler(void)
{
    debug_printf("GOT ERROR !!! \n");
    while (1)
    {
    }
}

