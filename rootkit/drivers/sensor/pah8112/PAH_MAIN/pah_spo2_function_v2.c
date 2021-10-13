// gsensor
//#include "accelerometer.h"

// platform support
//#include "system_clock.h"

//#include "log_printf.h"

// C library
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "pah_spo2_function.h"

// Platform support
#include "system_clock.h"

// pah8112_Driver
#include "pah8series_config.h"
#include "pah_driver_types.h"
#include "pah_driver.h"
#include "pah_comm.h"

// HR algorithm
#include "pxialg.h"
// spo2 algorithm
#include "pxialg_spo2.h"

typedef struct {
	void *pBuffer;
	void *hr_pBuffer;
} pxialg_spo2_state;

typedef struct {
	uint8_t HRD_Data[13];
	float MEMS_Data[3];
} ppg_mems_data_t;

/*============================================================================
STATIC VARIABLES
============================================================================*/
static main_state_s _state;
static pah_flags_s flags;
static pxialg_spo2_state _spo2_state;
pah8series_data_t pxialg_data_spo2_hr;

ppg_mems_data_t ppg_mems_data;
static uint32_t acc_buf_count = 0;
static uint8_t Frame_Cnt = 0;
static SpO2_info_t _spo2_detect_item;

static uint32_t count_time = 0; // add item for V560
uint32_t effective = 0;			// add item for V560
float FirstSpO2 = 0.0f;
uint8_t first_falg = 0;
uint8_t reset_flag = 0;		// add item for Flow
uint8_t over_mems_flag = 0; // add item for Flow
uint8_t time_out_flag = 0;	// add item for Flow
float HR_temp = 0.0f;

static SpO2_mems_t mems_data_raw[PPG_BUF_MAX_SPO2]; // add item
// V2.5
// static uint32_t last_report_time=0;
static uint32_t timing_tuning_time = 0;
static uint32_t timing_tuning_num = 0;

static int index_count = 0;
static uint8_t alg_cal_en = 0;
static uint32_t data_buf_count = 0;
static uint32_t data_buf_time = 0;

static uint32_t data_buf_count_spo2_alg = 0;
static uint32_t data_buf_num = 0;
static uint32_t data_buf_time_all = 0;

static bool need_log_header = true;

static float myHR = 0.0f;
static float myHR_opt = 0.0f;
static float mySpO2 = 0.0f;

static uint32_t Expo_time_backup[3] = {0};
static uint8_t LEDDAC_backup[3] = {0};

static uint8_t _ppg_data_100hz[PPG_BUF_MAX_SPO2 * 2 * 4];

static float hr_grader = 0.0f;
static uint8_t mySPO2_ReadyFlag = 0;

static uint8_t mySPO2_ReadyFlag_out = 0;

/*============================================================================
STATIC FUNCTION PROTOTYPES
============================================================================*/
#if defined(PPG_MODE_ONLY) || defined(ALWAYS_TOUCH)
static void start_healthcare_ppg(void);
#endif
static void start_healthcare_ppg_touch(void);
static void start_healthcare_touch_only(void);
static void stop_healthcare(void);
static void log_pah8series_data_header(uint32_t ch_num, uint32_t ir_ch_num, uint32_t g_sensor_mode);
static void log_pah8series_data(const pah8series_data_t *data);
static void log_pah8series_no_touch(void);
static void pah8112_ae_info_check(pah8series_data_t *pxialg_data, bool Tuning); // V2.5
static void report_fifo_data_spo2(uint64_t timestamp, uint8_t *fifo_data, uint32_t fifo_data_num_per_ch,
								  uint32_t ch_num, bool is_touched);
static bool spo2_algorithm_process(pah8series_data_t *pxialg_data, uint32_t ch_num); // V2.5
static uint8_t spo2_algorithm_calculate_v2(pah8series_data_t *pxialg_data);
static void spo2_algorithm_close(void);
static void Error_Handler(void);

void pah8series_ppg_dri_SPO2_init(void)
{
	memset(&_state, 0, sizeof(_state));
	// PAH
	{
		pah_flags_default(&flags);
		flags.stream = pah_stream_dri;
		flags.alg_mode = pah_alg_mode_SPO2;
		if (!pah_init_with_flags(&flags)) {
			debug_printf("pah_init_with_flags() fail. \n");
			Error_Handler();
		}
	}
}

void pah8series_ppg_polling_SPO2_init(void)
{
	memset(&_state, 0, sizeof(_state));
	// PAH
	{
		pah_flags_default(&flags);
		flags.stream = pah_stream_polling;
		flags.alg_mode = pah_alg_mode_SPO2;
		if (!pah_init_with_flags(&flags)) {
			debug_printf("pah_init_with_flags() fail. \n");
			Error_Handler();
		}
	}
}

void pah8series_ppg_SPO2_start(void)
{
#if defined(PPG_MODE_ONLY)
	start_healthcare_ppg();
#else
	start_healthcare_touch_only();
#endif
	_state.status = main_status_start_healthcare;
	_state.pxialg_has_init = false;
}
// V2.5
void pah8series_ppg_SPO2_stop(void)
{
	// log
	log_pah8series_no_touch();
	_state.alg_status = alg_status_close;
	_state.Tuning = 0;
	stop_healthcare();
	_state.status = main_status_idle;
}
// V2.5
void spo2_alg_task(void)
{
	switch (_state.alg_status) {
	case alg_status_process: {
		spo2_algorithm_process(&_state.pxialg_data, _state.pxialg_data.nf_ppg_channel);
		_state.alg_status = alg_status_idle;
	} break;

	case alg_status_close: {
		spo2_algorithm_close();
		_state.alg_status = alg_status_idle;
		debug_printf("alg_status_close. \n");
	} break;

	case alg_status_idle: {
	} break;

	default:
		break;
	}
}
// V2.5
void pah8series_read_task_spo2_dri(volatile bool *has_interrupt_pah, volatile bool *has_interrupt_button,
								   volatile uint64_t *interrupt_pah_timestamp)
{
	pah_ret ret = pah_err_unknown;
	if (_state.status == main_status_start_healthcare) {
		if (*has_interrupt_pah) {
			*has_interrupt_pah = false;

			ret = pah_task();
			if (ret == pah_success) {

				if (pah_is_ppg_mode()) {
					uint8_t ppg_mode_flag = 0;
#ifdef PPG_MODE_ONLY
					ppg_mode_flag = 1;
#endif
					bool is_touched = pah_is_touched();
					debug_printf("Touch or not = %d \n", is_touched);
					if (is_touched || ppg_mode_flag) {
						// uint32_t timestamp = get_tick_count();
						uint8_t *fifo_data = pah_get_fifo_data();
						uint32_t fifo_data_num_per_ch = pah_fifo_data_num_per_ch();
						uint32_t fifo_ch_num = pah_get_fifo_ch_num();
						bool is_touched = pah_is_touched();
						// debug_printf("\n*interrupt_pah_timestamp == %d\n",*interrupt_pah_timestamp);
						report_fifo_data_spo2(*interrupt_pah_timestamp, fifo_data, fifo_data_num_per_ch, fifo_ch_num,
											  is_touched);
					}

					else {
						// log
						log_pah8series_no_touch();
						extern struct pah8112_data pah8112_data;
						if (pah8112_data.spo2_data_rdy_flag != 0) {
							pah8112_data.spo2_data_rdy_flag = 0;
						}
						if (pah8112_data.hr_data_rdy_flag != 0) {
							pah8112_data.hr_data_rdy_flag = 0;
						}
						// V2.5
						_state.alg_status = alg_status_close;
						_state.Tuning = 0;
						start_healthcare_touch_only();
					}

				} else if (pah_touch_mode == pah_query_mode()) {
					if (pah_is_touched()) {
						start_healthcare_ppg_touch();
					}
				}
			} else if (ret == pah_pending) {
				// ignore
			} else {
				debug_printf("pah_task fail, ret = %d \n", ret);
				Error_Handler();
			}
		}

	} else if (_state.status == main_status_idle) {
	}
}
// V2.5
void pah8series_read_task_spo2_polling(volatile bool *has_interrupt_pah, volatile bool *has_interrupt_button)
{
	// unused
	pah_ret ret = pah_err_unknown;
	if (*has_interrupt_pah)
		*has_interrupt_pah = false;

	if (_state.status == main_status_start_healthcare) {
		ret = pah_task();
		if (ret == pah_success) {
			if (pah_is_ppg_mode()) {

				uint8_t ppg_mode_flag = 0;
#ifdef PPG_MODE_ONLY
				ppg_mode_flag = 1;
#endif
				bool is_touched = pah_is_touched();
				debug_printf("Touch or not = %d \n", is_touched);
				if (is_touched || ppg_mode_flag) {
					uint32_t timestamp = get_tick_count();
					uint8_t *fifo_data = pah_get_fifo_data();
					uint32_t fifo_data_num_per_ch = pah_fifo_data_num_per_ch();
					uint32_t fifo_ch_num = pah_get_fifo_ch_num();
					// bool is_touched = pah_is_touched();
					report_fifo_data_spo2(timestamp, fifo_data, fifo_data_num_per_ch, fifo_ch_num, is_touched);

				}

				else {
					// log
					log_pah8series_no_touch();
					// V2.5
					_state.alg_status = alg_status_close;
					_state.Tuning = 0;
					start_healthcare_touch_only();
				}

			} else if (pah_touch_mode == pah_query_mode()) {
				if (pah_is_touched()) {
					start_healthcare_ppg_touch();
				}
			}
		} else if (ret == pah_pending) {
			// ignore
		} else {
			debug_printf("pah_task fail, ret = %d \n", ret);
			Error_Handler();
		}
	} else if (_state.status == main_status_idle) {
	}
}
// V2.5
static void log_pah8series_data_header(uint32_t ch_num, uint32_t ir_ch_num, uint32_t g_sensor_mode)
{
	// log pah8series data header
	// (1)Using total channel numbers;
	// (2)reserved;
	// (3)reserved;
	// (4)IR channel number;
	// (5)MEMS mode 0:2G, 1:4G, 2:8G
	log_printf("PPG CH#, %d, %d, %d, %d, %d\n", ch_num, pah8112_setting_version_SPO2(), 0, ir_ch_num, g_sensor_mode);
}

static void log_pah8series_data(const pah8series_data_t *pxialg_data)
{
	int i = 0;
	uint32_t *ppg_data = (uint32_t *)pxialg_data->ppg_data;
	int16_t *mems_data = pxialg_data->mems_data;
	int data_num = pxialg_data->nf_ppg_channel * pxialg_data->nf_ppg_per_channel;
	log_printf("Frame Count, %d \n", pxialg_data->frame_count);
	log_printf("Time, %d \n", pxialg_data->time);
	log_printf("PPG, %d, %d, ", pxialg_data->touch_flag, data_num);
	for (i = 0; i < data_num; ++i) {
		log_printf("%d, ", *ppg_data);
		ppg_data++;
	}
	log_printf("\n");
	log_printf("MEMS, %d, ", pxialg_data->nf_mems);
	for (i = 0; i < pxialg_data->nf_mems * 3; ++i) {
		log_printf("%d, ", *mems_data);
		mems_data++;
	}
	log_printf("\n");
}

static void log_pah8series_no_touch(void)
{
	static const int DUMMY_PPG_DATA_NUM = 20;
	static const int DUMMY_PPG_CH_NUM = 2;
	int i = 0;

	log_printf("Frame Count, 0 \n");
	log_printf("Time, %d \n", DUMMY_PPG_DATA_NUM * 50); // 20Hz = 50ms
	log_printf("PPG, 0, %d, ", DUMMY_PPG_DATA_NUM * DUMMY_PPG_CH_NUM);
	for (i = 0; i < DUMMY_PPG_DATA_NUM * DUMMY_PPG_CH_NUM; i++) {
		log_printf("0, ");
	}
	log_printf("\n");
	log_printf("MEMS, %d, ", DUMMY_PPG_DATA_NUM);
	for (i = 0; i < DUMMY_PPG_DATA_NUM * 3; ++i) {
		log_printf("0, ");
	}
	log_printf("\n");
}
// V2.5
static void report_fifo_data_spo2(uint64_t timestamp, uint8_t *fifo_data, uint32_t fifo_data_num_per_ch,
								  uint32_t ch_num, bool is_touched)
{
	// gsensor
#if defined(ENABLE_MEMS_ZERO)
	_state.pxialg_data.mems_data = _state.mems_data;
	_state.pxialg_data.nf_mems = fifo_data_num_per_ch;
#else
	accelerometer_get_fifo(&_state.pxialg_data.mems_data, &_state.pxialg_data.nf_mems);
#endif // ENABLE_MEMS_ZERO

	_state.pxialg_data.touch_flag = is_touched ? 1 : 0;
	_state.pxialg_data.time = (uint32_t)(timestamp - _state.last_report_time);
	_state.pxialg_data.ppg_data = (int32_t *)fifo_data;
	_state.pxialg_data.nf_ppg_channel = ch_num;
	_state.pxialg_data.nf_ppg_per_channel = fifo_data_num_per_ch;
	++_state.pxialg_data.frame_count;

	_state.last_report_time = timestamp;
#ifdef Timing_Tuning
	timing_tuning_num = timing_tuning_num + _state.pxialg_data.nf_ppg_per_channel;
	timing_tuning_time = timing_tuning_time + _state.pxialg_data.time;
	if ((_state.Tuning) && (timing_tuning_num > 100)) {
		pah_do_timing_tuning(timing_tuning_time, timing_tuning_num, 40.0f);
		timing_tuning_num = 0;
		timing_tuning_time = 0;
	}
	_state.Tuning = 1;
#endif
	debug_printf("_state.pxialg_data.time == %d\n", _state.pxialg_data.time);

	pah8112_ae_info_read(_state.Expo_time, _state.LEDDAC, &_state.Touch_data);
	_state.alg_status = alg_status_process;
}

// V2.5
static bool spo2_algorithm_process(pah8series_data_t *pxialg_data, uint32_t ch_num)
{
	int i;
	uint8_t *ppg_data = (uint8_t *)_state.pxialg_data.ppg_data;

	// log header
	if (need_log_header) {
		need_log_header = false;
		log_pah8series_data_header(ch_num, PPG_IR_CH_NUM, ALG_GSENSOR_MODE);
	}
	pah8112_ae_info_check(&_state.pxialg_data, _state.Tuning);

	log_pah8series_data(&_state.pxialg_data);

	//    for( i = 0 ; i <(_state.pxialg_data.nf_ppg_channel*_state.pxialg_data.nf_ppg_per_channel*4) ; i ++)
	//    {
	//        _ppg_data_100hz[data_buf_count*_state.pxialg_data.nf_ppg_channel*4+i]=ppg_data[i];

	//    }
	//    data_buf_count = data_buf_count + _state.pxialg_data.nf_ppg_per_channel ;
	//    data_buf_time  = data_buf_time  + _state.pxialg_data.time ;
	data_buf_count_spo2_alg += _state.pxialg_data.nf_ppg_per_channel;

	//    if((acc_buf_count + _state.pxialg_data.nf_mems) > (MEMS_ODR_MAX - 1))
	//    {
	//    acc_buf_count = 0;
	//    }
	//    for(i=0;i<_state.pxialg_data.nf_mems;i++)
	//    {
	//        mems_data_raw[(i+acc_buf_count)].data[0]=_state.pxialg_data.mems_data[i*3];
	//        mems_data_raw[(i+acc_buf_count)].data[1]=_state.pxialg_data.mems_data[i*3+1];
	//        mems_data_raw[(i+acc_buf_count)].data[2]=_state.pxialg_data.mems_data[i*3+2];
	//    }
	//    acc_buf_count = acc_buf_count + _state.pxialg_data.nf_mems ;
	//
	//    debug_printf("acc_buf_count = %d \n", acc_buf_count);
	//    if(data_buf_count_spo2_alg>=25)
	//    {
	alg_cal_en = 1;

	//    }
	//    else
	//    {
	//        alg_cal_en = 0 ;
	//    }
	debug_printf("data_buf_count = %d data_buf_count_spo2_alg = %d  \n", data_buf_count, data_buf_count_spo2_alg);

	return true;
}

void spo2_algorithm_calculate_run(void)
{
	if (alg_cal_en == 1) {
		alg_cal_en = 0;
		data_buf_num = data_buf_count;
		data_buf_time_all = data_buf_time;
		uint32_t acc_buf_num = acc_buf_count;

		data_buf_count = 0;
		data_buf_time = 0;
		acc_buf_count = 0;
		uint32_t qq1 = get_tick_count();
		// spo2_algorithm_calculate((int32_t*)_ppg_data_100hz, _state.pxialg_data.nf_ppg_channel,
		// data_buf_num,data_buf_time_all, 1/*_touch_flag*/,mems_data_raw,acc_buf_num);
		spo2_algorithm_calculate_v2(&_state.pxialg_data);
		uint32_t qq2 = get_tick_count();
		debug_printf("Cal time = %d data_buf_num = %d  \n", qq2 - qq1, data_buf_num);
	}
}
uint8_t Frame_Cnt_buf;

static uint8_t spo2_algorithm_calculate_v2(pah8series_data_t *pxialg_data)
{

	uint8_t ready_flag = 0;
	int i;
	float correlation = 0.0f;
	float trust_flag = 0.0f;
	float MEMS_RMS = 0.0f;
	float temp = 0.0f;
	uint8_t mySpO2_ReadyFlag = 0;

#if defined(ENABLE_PXI_ALG_SPO2)
	// Initialize algorithm
	if (!_state.pxialg_has_init) {

		int version = 0;
		uint32_t buffer_size = 0;

		memset(&_spo2_state, 0, sizeof(_spo2_state));

		version = SpO2_Version();
		log_printf("SpO2_Version = %d \r\n", version);

		buffer_size = SpO2_OpenSize();
		log_printf("SpO2_Open buffer_size = %d \n", buffer_size);

#ifdef _ALGO_NON_USE_HEAP_SIZE

		if (sizeof(pah_alg_buffer) >= buffer_size) {
			memset(pah_alg_buffer, 0x00, sizeof(pah_alg_buffer));
			_spo2_state.pBuffer = pah_alg_buffer;
		} else {
			log_printf("SpO2_buffer_size_not enough \n");
			return false;
		}
#else
		_spo2_state.pBuffer = malloc(buffer_size);
		if (!_spo2_state.pBuffer) {
			log_printf("SpO2_buffer_size_not enough \n");
			return false;
		}
#endif

		if (SpO2_Open(_spo2_state.pBuffer) != SPO2_MSG_SUCCESS) {
			log_printf("SpO2_algorithm_open_fail \n");
			return false;
		}

		SpO2_SetInputRatioHighBound(93.0f);
		SpO2_SetInputRatioLowBound(87.0f);
		SpO2_SetInputModelScale(0.5f);

		SpO2_SetInputCoefHigh0(-37118.40625f);
		SpO2_SetInputCoefHigh1(1146.76513671875f);
		SpO2_SetInputCoefHigh2(-11.7852153778076f);
		SpO2_SetInputCoefHigh3(0.0403929986059666f);

		SpO2_SetInputCoefMid0(-840.949523925781f);
		SpO2_SetInputCoefMid1(30.6027145385742f);
		SpO2_SetInputCoefMid2(-0.352486997842789f);
		SpO2_SetInputCoefMid3(0.00141100003384054f);

		SpO2_SetInputCoefLow0(-20.6128444671631f);
		SpO2_SetInputCoefLow1(2.0620698928833f);
		SpO2_SetInputCoefLow2(-0.0210389997810125f);
		SpO2_SetInputCoefLow3(0.00012599999899976f);

		SpO2_SetIIRFilterRatio(0.5f);
		SpO2_SetInputCorrelationTh(0.949999988079071f);
		SpO2_SetInputRatioLow(0.05f);

		SpO2_SetInputFlagCheckHr(1);
		SpO2_SetMotionDetectionLevel(2.0f);
		SpO2_Set_Update_Sec(1);

		SpO2_Set_Init_Low_SpO2_Constrain(0.0);
		SpO2_SetOutputHighBound(100.0f);
		SpO2_SetOutputLowBound(60.0f);

		SpO2_Set_Keep_Init_SpO2_Bound(94.80f);	 // add For V571
		SpO2_Set_Keep_Init_SpO2_Count_Th(15.0f); // add For V571
		SpO2_Set_SpO2_Bias_Value(0.5f);			 // add For V571
		SpO2_Set_SpO2_Outlier_Smooth_Coef(5.5f); // add For V571
		SpO2_Set_SpO2_Decrease_Coef(0.5f);		 // add For V571
		// SpO2_SetSkipDataLen(0);

		// static  Hr
		version = PxiAlg_Version();
		log_printf("HR_Version = %d \r\n", version);

		uint32_t Query_Mem_Size = PxiAlg_Query_Mem_Size();
		log_printf("HR_Mem_Size = %d \r\n", Query_Mem_Size);

#ifdef _ALGO_NON_USE_HEAP_SIZE

		if (sizeof(pah_alg_buffer) >= (buffer_size + Query_Mem_Size + 16)) {
			// memset(pah_alg_buffer,0x00,sizeof(pah_alg_buffer));
			_spo2_state.hr_pBuffer = &pah_alg_buffer[(buffer_size + 16) / 4];
		} else {
			log_printf("SpO2_buffer_size_not enough \n");
			return false;
		}
#else
		_spo2_state.hr_pBuffer = malloc(Query_Mem_Size);
		if (!_spo2_state.hr_pBuffer) {
			log_printf("SpO2_buffer_size_not enough \n");
			return false;
		}
#endif

		PxiAlg_Open_Mem(_spo2_state.hr_pBuffer);

		PxiAlg_SetQualityThreshold(0.35f);
		PxiAlg_SetHRSmoothWeight(0.9f);
		PxiAlg_SetProcWindow(5.0f, 5.0f);

		/***********************Algorithm default setting start***********************************/
		// Algorithm default is ?2G scale,
		// If users want to use default Algorithm setting, don?? write these API functions.

		/***********************Algorithm default setting end***********************************/
		PxiAlg_SetMemsScale(1); // MEMS 2/4/8/16G : 0/1/2/3
		PxiAlg_SetSkipSampleNum(10);
		PxiAlg_SetMemsScaleBase(1, 0); // MEMS 2/4/8/16G : 0/1/2/3
		PxiAlg_SetForceOutputLink(15, 90, 60, 1, 0.9f);
		PxiAlg_SetNormalOutput(200, 40, 1);
		PxiAlg_SetForceRipple(3, 1);
		// PxiAlg_SetForceOutput(30, 120, 45);
		// PxiAlg_SetForceOutputTime(10);

		PxiAlg_SetPackage(0, 1);

		Frame_Cnt_buf = Frame_Cnt;

		_state.pxialg_has_init = true;
	}

	int ret_value = 0;

	ret_value = PxiAlg_Entrance(pxialg_data);

	if (PxiAlg_GetHrUpdateFlag()) {
		PxiAlg_HrGet(&myHR);
		ready_flag = PxiAlg_GetReadyFlag();
		PxiAlg_GetSigGrade(&hr_grader);
	}

	//      if( ret_value == FLAG_DATA_READY)
	//      {
	//        PxiAlg_HrGet(&myHR);
	//      }

	//      ready_flag = PxiAlg_GetReadyFlag();
	//      PxiAlg_GetSigGrade(&hr_grader);
	if (hr_grader >= 50) {
		HR_temp = myHR;
		myHR_opt = myHR;
	} else {
		myHR_opt = HR_temp;
	}

	for (i = 0; i < pxialg_data->nf_mems; i++) {
		mems_data_raw[i].data[0] = _state.pxialg_data.mems_data[i * 3];
		mems_data_raw[i].data[1] = _state.pxialg_data.mems_data[i * 3 + 1];
		mems_data_raw[i].data[2] = _state.pxialg_data.mems_data[i * 3 + 2];
	}

	SpO2_SetHrValue(((int)myHR));
	SpO2_SetMems(mems_data_raw, pxialg_data->nf_mems);
	ret_value = SpO2_Process(1, pxialg_data->nf_ppg_channel, pxialg_data->ppg_data, pxialg_data->nf_ppg_per_channel);
	SpO2_GetSpO2(&mySpO2);
	SpO2_GetReadyFlag(&mySPO2_ReadyFlag);
	SpO2_GetInfo(&_spo2_detect_item);
	correlation = _spo2_detect_item.Noise;
	trust_flag = _spo2_detect_item.Output[3];
	MEMS_RMS = _spo2_detect_item.RMS;

	if ((ret_value == 0) && (data_buf_count_spo2_alg >= 25)) {
		data_buf_count_spo2_alg = data_buf_count_spo2_alg - 25;
		mySPO2_ReadyFlag_out = alg_status_process;
		if (mySPO2_ReadyFlag > 0) {
			log_printf("Ready = %d ", mySPO2_ReadyFlag);
			log_printf(",myHR = " LOG_FLOAT_MARKER " ", LOG_FLOAT(myHR));
			// log_printf(",myHR_opt = "LOG_FLOAT_MARKER" ",LOG_FLOAT(myHR_opt));
			log_printf(",HR_Grade = " LOG_FLOAT_MARKER " ", LOG_FLOAT(hr_grader));
			log_printf(",mySpO2 = " LOG_FLOAT_MARKER " ", LOG_FLOAT(mySpO2));
			log_printf(",trust_flag = " LOG_FLOAT_MARKER " ", LOG_FLOAT(trust_flag));
			log_printf(",MEMS_RMS    = " LOG_FLOAT_MARKER " ", LOG_FLOAT(MEMS_RMS));

			extern struct pah8112_data pah8112_data;
			pah8112_data.spo2_data = (int32_t)mySpO2;
			pah8112_data.hr_data = (int32_t)myHR;
			pah8112_data.hr_data_rdy_flag = 1;
			pah8112_data.spo2_data_rdy_flag = 1;

			// log_printf("  ,count_time = %d ", count_time);
		} else {
			log_printf("Ready = %d ", mySPO2_ReadyFlag);
			log_printf(",myHR = " LOG_FLOAT_MARKER " ", LOG_FLOAT(myHR));
			// log_printf(",myHR_opt = "LOG_FLOAT_MARKER" ",LOG_FLOAT(myHR_opt));
			log_printf(",HR_Grade = " LOG_FLOAT_MARKER " ", LOG_FLOAT(hr_grader));
			log_printf(",mySpO2 = " LOG_FLOAT_MARKER " ", LOG_FLOAT(mySpO2));
			log_printf(",trust_flag = " LOG_FLOAT_MARKER " ", LOG_FLOAT(trust_flag));
			log_printf(",MEMS_RMS    = " LOG_FLOAT_MARKER " ", LOG_FLOAT(MEMS_RMS));
			// log_printf("  ,count_time : % d ", count_time);
			extern struct pah8112_data pah8112_data;
			pah8112_data.spo2_data = (int32_t)mySpO2;
			pah8112_data.hr_data = (int32_t)myHR;
			pah8112_data.hr_data_rdy_flag = 0;
			pah8112_data.spo2_data_rdy_flag = 0;
		}
	} else if (ret_value != 0) {
		mySPO2_ReadyFlag_out = 0;
		switch (ret_value) {
		case SPO2_MSG_ALG_NOT_OPEN:
			debug_printf("Algorithm is not initialized. \n");
			break;
		case SPO2_MSG_ALG_REOPEN:
			debug_printf("Algorithm is re initialized. \n");
			break;
		case SPO2_MSG_ALG_NOT_SUPPORT:
			debug_printf("Algorithm is not support. \n");
			break;
		default:
			debug_printf("Algorithm unhandle error = %d \n", ret_value);
			break;
		}
		return 1;
	}
	float new_spo2 = mySpO2;
	count_time++;
#if defined(Optimize_SPO2_flow)
	if (new_spo2 > 0) {
		if (first_falg == 0) {
			FirstSpO2 = new_spo2;
			first_falg = 1;
		}
		if (FirstSpO2 >= 90) {
			if (effective < 3) {
				// n+=1;
				if (trust_flag == 1 && MEMS_RMS <= MEMS_RMS_MAX_TH) {
					effective += 1;
					log_printf("  ,Opt_SPO2    = " LOG_FLOAT_MARKER "  ", LOG_FLOAT(new_spo2));
				} else {
					if (MEMS_RMS >= MEMS_RMS_MAX_BREAK_TH) //&& new_spo2>0)
					{
						over_mems_flag = 1;
						log_printf("  over_mems_flag = %d,", over_mems_flag);
						log_printf("  Detect serious motion please stop meassurment ");
					} else {
						log_printf("  ,Opt_SPO2    = " LOG_FLOAT_MARKER "  ", LOG_FLOAT(new_spo2));
					}
				}
			} else {

				if (trust_flag == 1 && MEMS_RMS <= MEMS_RMS_MAX_TH) {
					effective += 1;
					log_printf("  ,Opt_SPO2    = " LOG_FLOAT_MARKER "  ", LOG_FLOAT(new_spo2));
				} else {
					if (MEMS_RMS >= MEMS_RMS_MAX_BREAK_TH) {
						over_mems_flag = 1;
						log_printf("  over_mems_flag = %d,", over_mems_flag);
						log_printf("  Detect serious motion please stop meassurment ");
					} else {
						log_printf("  ,Opt_SPO2    = " LOG_FLOAT_MARKER "  ", LOG_FLOAT(new_spo2));
					}
				}
			}

			if (count_time >= 60 && effective < 3) {
				time_out_flag = 1;
				log_printf("  timeout_flag = %d,", time_out_flag);
				log_printf("  No_effective_value_stop_measurement_spo2_give_reference ");
			}
			log_printf("\n");
		} else {
			if (effective < 5) {
				// n+=1;
				if (trust_flag == 1 && MEMS_RMS <= MEMS_RMS_MAX_TH) {
					effective += 1;
				} else {
					if (MEMS_RMS >= MEMS_RMS_MAX_BREAK_TH) //&& new_spo2>0)
					{
						over_mems_flag = 1;
						log_printf("  over_mems_flag = %d,", over_mems_flag);
						log_printf("  Detect serious motion please stop meassurment ");
					}
				}
			} else {

				if (trust_flag == 1 && MEMS_RMS <= MEMS_RMS_MAX_TH) {
					effective += 1;
					log_printf("  ,Opt_SPO2    = " LOG_FLOAT_MARKER "  ", LOG_FLOAT(new_spo2));
				} else {
					if (MEMS_RMS >= MEMS_RMS_MAX_BREAK_TH) {
						over_mems_flag = 1;
						log_printf("  over_mems_flag = %d,", over_mems_flag);
						log_printf("  Detect serious motion please stop meassurment ");
					} else {
						log_printf("  ,Opt_SPO2    = " LOG_FLOAT_MARKER "  ", LOG_FLOAT(new_spo2));
					}
				}
			}

			if (count_time >= 60 && effective < 5) {
				time_out_flag = 1;
				log_printf("  timeout_flag = %d,", time_out_flag);
				log_printf("  No_effective_value_stop_measurement_spo2_give_reference ");
			}

			if (reset_flag == 0 && new_spo2 >= 90) {
				reset_flag = 1;
				effective = 0;
				log_printf("  reset_flag = %d,", reset_flag);
				log_printf("  Detect_ascending_rest_flow ");
			}

			log_printf("\n");
		}
	} else {
		if (MEMS_RMS >= MEMS_RMS_MAX_BREAK_TH) {
			over_mems_flag = 1;
			log_printf("  over_mems_flag = %d,", over_mems_flag);
			log_printf("  Before_get_spo2_Detect serious motion please stop meassurment ");
		}

		if (count_time >= 60) {
			time_out_flag = 1;
			log_printf("  timeout_flag = %d,", time_out_flag);
			log_printf("  No_SpO2_value_stop_measurement ");
		}
		log_printf("\n");
	}

#endif

#endif
	return true;
}

static void spo2_algorithm_close(void)
{
#if defined(ENABLE_PXI_ALG_SPO2)
	if (_state.pxialg_has_init) {
		_state.pxialg_has_init = false;
		need_log_header = true;
		PxiAlg_Close();
		data_buf_count = 0;
		data_buf_count_spo2_alg = 0;
		data_buf_count = 0;
		data_buf_time = 0;
		acc_buf_count = 0;
		Frame_Cnt = 0;
		alg_cal_en = 0;
		mySPO2_ReadyFlag_out = 0;
		SpO2_Close();
#ifndef _ALGO_NON_USE_HEAP_SIZE
		free(_spo2_state.pBuffer);
		free(_spo2_state.hr_pBuffer);
#endif
		_spo2_state.pBuffer = NULL;
		_spo2_state.hr_pBuffer = NULL;
		myHR = 0.0f;
		mySpO2 = 0.0f;
		hr_grader = 0.0f;
		mySPO2_ReadyFlag = 0;
		count_time = 0; // add item for V571
		effective = 0;	// add item for V571
		first_falg = 0;
		reset_flag = 0;

		log_printf("spo2_algorithm_close\n");
	}
#endif // ENABLE_PXI_ALG_SPO2
}

#if defined(PPG_MODE_ONLY) || defined(ALWAYS_TOUCH)
static void start_healthcare_ppg()
{
// gsensor
#ifndef ENABLE_MEMS_ZERO
	accelerometer_start();
#endif
	// PAH
	if (!pah_enter_mode(pah_ppg_mode))
		Error_Handler();

	_state.last_report_time = get_tick_count();
}
#endif

static void start_healthcare_ppg_touch()
{
	// gsensor
#ifndef ENABLE_MEMS_ZERO
	accelerometer_start();
#endif
	// PAH
	if (!pah_enter_mode(pah_ppg_touch_mode))
		Error_Handler();

	_state.last_report_time = get_tick_count();
}

static void start_healthcare_touch_only(void)
{
	// gsensor
#ifndef ENABLE_MEMS_ZERO
	accelerometer_stop();
#endif

	// PAH
	if (!pah_enter_mode(pah_touch_mode))
		Error_Handler();
}

static void stop_healthcare(void)
{
	// gsensor
#ifndef ENABLE_MEMS_ZERO
	accelerometer_stop();
#endif
	// PAH
	if (!pah_enter_mode(pah_stop_mode))
		Error_Handler();
}
void spo2_algorithm_init(void)
{
#if defined(ENABLE_PXI_ALG_SPO2)
	spo2_algorithm_close();
	memset(&_state, 0, sizeof(_state));
	_state.last_report_time = get_tick_count();
	debug_printf("_state.last_report_time = %d \n", _state.last_report_time);
	need_log_header = true;
#endif // ENABLE_PXI_ALG_SPO2
}
// V2.5
static void pah8112_ae_info_check(pah8series_data_t *pxialg_data, bool Tuning)
{
	uint8_t i;
	uint32_t MIN = 0;
	float VAR_MAX = 0;
	float AE_VAR = 0;
	uint32_t *Expo_time = _state.Expo_time;
	uint8_t *LEDDAC = _state.LEDDAC;
	uint16_t Touch_data = _state.Touch_data;

	for (i = 0; i < 3; i++) {
		if (Expo_time_backup[i] > 0) {
			MIN = (Expo_time_backup[i] >= Expo_time[i]) ? Expo_time[i] : Expo_time_backup[i];
			AE_VAR = ((float)Expo_time_backup[i] - (float)Expo_time[i]);
			AE_VAR = ((AE_VAR >= 0.0f) ? AE_VAR : AE_VAR * (-1)) / (float)MIN;
			VAR_MAX = (AE_VAR >= VAR_MAX) ? AE_VAR : VAR_MAX;
		}
		Expo_time_backup[i] = Expo_time[i];
	}
	for (i = 0; i < 3; i++) {
		if (LEDDAC_backup[i] > 0) {
			MIN = (LEDDAC_backup[i] >= LEDDAC[i]) ? LEDDAC[i] : LEDDAC_backup[i];
			AE_VAR = ((float)LEDDAC_backup[i] - (float)LEDDAC[i]);
			AE_VAR = ((AE_VAR >= 0.0f) ? AE_VAR : AE_VAR * (-1)) / (float)MIN;
			VAR_MAX = (AE_VAR >= VAR_MAX) ? AE_VAR : VAR_MAX;
		}
		LEDDAC_backup[i] = LEDDAC[i];
	}

	log_printf("INFO, %d, %d, %d, %d, %d, %d ,", Expo_time[0], Expo_time[1], Expo_time[2], LEDDAC[0], LEDDAC[1],
			   LEDDAC[2]);
	log_printf("VAR " LOG_FLOAT_MARKER ", Tuning %d, T_Data %d, \n", LOG_FLOAT(VAR_MAX), Tuning, Touch_data);
}

static void Error_Handler(void)
{
	debug_printf("GOT ERROR !!! \n");
	while (1) {
	}
}
