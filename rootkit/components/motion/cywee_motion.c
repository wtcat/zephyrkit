#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <init.h>
#include <sys/byteorder.h>
#include <stdio.h>
#include <stdlib.h>
#include "cwm_lib.h"
#include "simulate_signal.h"
#include "gpio/gpio_apollo3p.h"
#include "cywee_motion.h"
#include <string.h>
#include <logging/log.h>
#include "fs/fs_interface.h"
#include <sys/_timeval.h>
#include <fs/fs.h>
#include <sys/time.h>
#include "drivers/counter.h"


static Sport_cfg_t sport_cfg;
struct cywee_motion cywee_param;
static const struct device *sensor_dev;


#define MOTION_STACK   4096

static int m_req_sensor_state = 0;

static K_HEAP_DEFINE(cywee_mpool, 9216);

uint64_t CWM_OS_GetTimeNs(void)
{
    uint64_t timeNow_ms = 0;
    timeNow_ms = k_uptime_get();
    return (timeNow_ms * 1000000);
}

void* CWM_OS_malloc(int size)
{
    // static int allocated_size = 0;
    void *ptr;

    if (size == 0)
        size = 1;
    ptr = k_heap_alloc(&cywee_mpool, size, K_NO_WAIT);
    // if (!ptr) 
    //     printf("%s(): No more memory\n", __func__);
    // allocated_size += size;
    // printf("%s(): Allocated %d bytes(Total: %d)\n", __func__, size,  allocated_size);
    return ptr;
}

void CWM_OS_free(void *ptr)
{
    if (ptr == NULL)
        return;
    k_heap_free(&cywee_mpool, ptr);
}

int CWM_OS_dbgOutput(const char * format)
{
    printf("%s", format);
    return 0;
}

int getReqSensor(int index) {
    if (m_req_sensor_state & (1<<index))
        return 1;
    return 0;
}

void sendentary_msg_send(void)
{
    printf("***sedentary\n");
}

void handup_down_msg_send(int msg)
{
    if(msg == 1) {
        printf("***hand up\n");
    }
    else if(msg == 2) {
        printf("***hand down\n");
    }    
}

void CWM_AP_SensorListen(pSensorEVT_t sensorEVT) 
{   
    send_motion_msg_to_queue(sensorEVT);
}

void sensor_accel_convert(struct sensor_value *acc_data, 
                CustomSensorData *data, uint16_t sensitivity_x10)
{
    float conv_val = 0;
    //sensitivity_x10 = 11;
    for(int i = 0; i < 3; i++) {
        conv_val = (acc_data[i].val1 * SENSOR_G) >> sensitivity_x10;
        data[0].fData[i] = conv_val / 1000000;
    }
    data[0].sensorType = CUSTOM_ACC;
}

void sensor_gyro_convert(struct sensor_value *gyro_data, 
                CustomSensorData *data, uint16_t sensitivity_x10)
{
    int64_t conv_val = 0;

    for (int i =0; i < 3; i++) {
         conv_val = (gyro_data[i].val1 * SENSOR_PI * 10) / 
                    (sensitivity_x10 * 180U);
        data[0].fData[i]  = conv_val / 1000000;  
    }
    data[0].sensorType = CUSTOM_GYRO;
}

void input_motion_data(struct k_timer *timer_id)
{
    struct sensor_value accel[3];
    CustomSensorData data;

	sensor_channel_get(sensor_dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    memset(&data, 0, sizeof(data));
    sensor_accel_convert(accel, &data, 11);
    // LOG_INF("int put accel x: %f, y : %f, z: %f, time : %d\n", 
    //                data.fData[0], data.fData[1], data.fData[2], k_uptime_get());
    // printf("int put accel x: %f, y : %f, z: %f, time : %d\n", 
    //               data.fData[0], data.fData[1], data.fData[2], k_uptime_get());
    CWM_CustomSensorInput(&data);   
}

void enable_sedentary(uint8_t enable)
{
    printf("set sedentary enable : %d\n", enable);
    cywee_param.sedentary.enable = enable;
    cywee_param.change |= 1 << SCL_SEDENTARY;
}

void enable_handup_down(uint8_t enable)
{
    printf("set handup_down value : %d\n", enable);

    cywee_param.handup_down.enable = enable;
    cywee_param.change |= 1 << SCL_HAND_UPDOWN_CONFIG;   
}

int body_info_change(struct body_info *user)
{
    if (user == NULL) {
        return 1;
    }
    cywee_param.body.age = user->age;
    cywee_param.body.gender = user->gender;
    cywee_param.body.height = user->height;
    cywee_param.body.weight = user->weight;
    cywee_param.body.wearing_hand = user->wearing_hand;
    cywee_param.change |= 1 << SCL_USER_INFO;
    return 0;
}

int set_sport_config(Sport_cfg_t *cfg)
{    
    if (cfg == NULL) {
        return 1;
    }
    memcpy (&sport_cfg, cfg, sizeof(sport_cfg ));    
    cywee_param.change |= 1 << SCL_SET_ACTIVITY_MODE;
    return 0;
}

// void num_to_file(int num, char* fname)
// {
//     if(fname != NULL) {
//         memset(fname, 0, NAME_LEN);
//         sprintf(fname, "%s_%d", SPORT_FILE, num);
//     }
// }

// void num_to_sfile(int num, char* fname)
// {
//     if(fname != NULL) {
//         memset(fname, 0, NAME_LEN);
//         sprintf(fname, "%s_%d", SLEEP_FILE, num);
//     }
// }

void cywee_motion_cfg_change(Sport_status_t mode)
{
    SettingControl_t scl;
    switch (sport_cfg.status) {
    case  SPORT_END:        
        scl.iData[0] = 1;
        scl.iData[1] = ACTIVITY_MODE_NORMAL;
        CWM_SettingControl(SCL_SET_ACTIVITY_MODE, &scl);
        cywee_param.sport_cfg.mode = ACTIVITY_MODE_NORMAL;
        cywee_param.sport_cfg.status = sport_cfg.status;
        cywee_param.sport_cfg.goal_calorise = 0;
        cywee_param.sport_cfg.goal_distance = 0;
        cywee_param.sport_cfg.goal_time = 0;
       // sport_data.tm_end = motion_get_time();
       // save_sport_data();
        break;
    case SPORT_PAUSE:
        scl.iData[0] = 1;
        scl.iData[1] = ACTIVITY_MODE_NORMAL;
        CWM_SettingControl(SCL_SET_ACTIVITY_MODE, &scl);
        cywee_param.sport_cfg.mode = ACTIVITY_MODE_NORMAL;
        cywee_param.sport_cfg.status = sport_cfg.status;
        //save_last_sport_data();
        break;
    case SPORT_START:     
        scl.iData[0] = 1;
        scl.iData[1] = sport_cfg.mode;
        CWM_SettingControl(SCL_SET_ACTIVITY_MODE, &scl);
        cywee_param.sport_cfg.mode = sport_cfg.mode;
        cywee_param.sport_cfg.status = sport_cfg.status;
        cywee_param.sport_cfg.goal_calorise = sport_cfg.goal_calorise;
        cywee_param.sport_cfg.goal_distance = sport_cfg.goal_distance;
        cywee_param.sport_cfg.goal_time = sport_cfg.goal_time;
        break;
    default : 
        break;
    }
   
}

int motion_get_time(void)
{   
    const struct device *rtc = device_get_binding("apollo3p_rtc");
	int ticks;
    counter_get_value(rtc, &ticks);
    return ticks;
}

void check_cywee_param_change(void)
{
    SettingControl_t scl;
    if (cywee_param.change & (1 << SCL_SET_ACTIVITY_MODE)) {
        if (sport_cfg.status != cywee_param.sport_cfg.status) {
            cywee_motion_cfg_change(sport_cfg.status);
        }
        cywee_param.change &= ~(1 << SCL_SET_ACTIVITY_MODE);
    }
    else if (cywee_param.change & (1 << SCL_USER_INFO)) {
        memset(&scl, 0, sizeof(scl));
        scl.iData[0] = 1;
        scl.iData[1] = cywee_param.body.age;                  //age
        scl.iData[2] = cywee_param.body.gender;               //gender 1:male, 0:female
        scl.iData[3] = cywee_param.body.weight;               //weight (kg)
        scl.iData[4] = cywee_param.body.height;               //height (cm)
        CWM_SettingControl(SCL_USER_INFO, &scl);
        cywee_param.change &= ~(1 << SCL_USER_INFO);
    }
    // else if (cywee_param.change & (1 << SCL_HAND_UPDOWN_CONFIG)) {
    //     if(cywee_param.handup_down.enable == true){
    //         CWM_Sensor_Enable(IDX_ALGO_WATCH_HANDUP);            
    //     }else {
    //         CWM_Sensor_Disable(IDX_ALGO_WATCH_HANDUP);
    //     }
    //     cywee_param.change &= ~(1 << SCL_HAND_UPDOWN_CONFIG);
    // }
    // else if (cywee_param.change  & (1 << SCL_SEDENTARY)) {
    //     if(cywee_param.sedentary.enable == true) {
    //         CWM_Sensor_Enable(IDX_ALGO_SEDENTARY);
    //         CWM_Sensor_Disable(IDX_ALGO_SLEEP);
    //     }else {
    //         CWM_Sensor_Disable(IDX_ALGO_SEDENTARY);
    //         CWM_Sensor_Enable(IDX_ALGO_SLEEP);
    //     }
    //     cywee_param.change &= ~(1 << SCL_SEDENTARY);
    // }
    else {
        if (cywee_param.change) {
            cywee_param.change = 0;
        }
    }
}

void enable_sleep_fuction(bool enable)
{
    if (enable){
        CWM_Sensor_Disable(IDX_ALGO_SEDENTARY);
        CWM_Sensor_Enable(IDX_ALGO_SLEEP);
    } else {
        CWM_Sensor_Enable(IDX_ALGO_SEDENTARY);
        CWM_Sensor_Disable(IDX_ALGO_SLEEP);
    }
    cywee_param.sleep.status = enable;
    printf("set cywee_param.sleep.status = %d\n", enable);
}

void cywee_param_init(void)
{
    /*read from flash*/
    cywee_param.body.age = 32;
    cywee_param.body.gender = 1;
    cywee_param.body.height = 170;
    cywee_param.body.weight = 68;
    cywee_param.body.wearing_hand = 0;

    cywee_param.biking.enable_extra_packet = true;

    cywee_param.adjust.floor_height = 380;
    cywee_param.adjust.anti = 0;
    cywee_param.adjust.pace_dist = 4;
    cywee_param.adjust.enable_extra = 1;
    cywee_param.adjust.treadmill_steps = 0;
    cywee_param.adjust.treadmill_step_freq = 0;
    cywee_param.adjust.treadmill_dist = 0;
    cywee_param.adjust.treadmill_truth_dist = 0;

    cywee_param.handup_down.enable = true;

    cywee_param.sedentary.trigger_time = 60;
    cywee_param.sedentary.is_reminder_on = 0;
    cywee_param.sedentary.reminder_time = 5;
    cywee_param.sedentary.release_level = 0;
    cywee_param.sedentary.enable = false;

    cywee_param.sport_cfg.mode = ACTIVITY_MODE_NORMAL;
    cywee_param.sport_cfg.status = SPORT_END;
    cywee_param.sport_cfg.goal_calorise = 0;
    cywee_param.sport_cfg.goal_distance = 0;
    cywee_param.sport_cfg.goal_time = 0;

    cywee_param.sleep.enable = true;
    cywee_param.sleep.check_time = 60;
    cywee_param.sleep.use_hr = 0;
    cywee_param.sleep.sleeping_level = 0;
    cywee_param.sleep.wake_level = 0;
    cywee_param.sleep.status = 0;
}

void cywee_motion_thread(void)
{
    SettingControl_t scl;
    char chipInfo[64];
    int64_t next_duration;
    struct timeval tv;
    time_t now;
    struct tm *tm_now; 

    OsAPI device_func =
    {
        .malloc       = CWM_OS_malloc,
        .free         = CWM_OS_free,
        .GetTimeNs    = CWM_OS_GetTimeNs,
        .dbgOutput    = CWM_OS_dbgOutput,
    };
    cywee_param_init();  
    sensor_dev = device_get_binding("ICM42607");
    if (!sensor_dev) {
		printf("Failed to find sensor ICM42607\n");
		return;
	}

    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;
    CWM_SettingControl(SCL_GET_LIB_INFO, &scl);
    printf("version:%d.%d.%d.%d product:%d\n", scl.iData[1], 
        scl.iData[2], scl.iData[3], scl.iData[4], scl.iData[5]);

    CWM_LibPreInit(&device_func);

    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;
    scl.iData[1] = 0;
    scl.iData[2] = 0;
    scl.iData[3] = 0;
    CWM_SettingControl(SCL_CHIP_VENDOR_CONFIG, &scl);

    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;
    scl.iData[1] = 0;
    //CWM_SettingControl(SCL_LIB_DEBUG, &scl);

    CWM_LibPostInit(CWM_AP_SensorListen);

    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;
    scl.iData[1] = 1;
    scl.iData[2] = (int)chipInfo;
    scl.iData[3] = sizeof(chipInfo);
    scl.iData[4] = 0;
    scl.iData[5] = 0;
    scl.iData[6] = 0;
    CWM_SettingControl(SCL_GET_CHIP_INFO, &scl);
    printf("have_security = %d.%d ret_buff_size = %d  chipInfo = %s\n",
            scl.iData[5], scl.iData[6], scl.iData[4], chipInfo);
    printf("chip_settings = %d, %d, %d\n", 
            scl.iData[9], scl.iData[10], scl.iData[11]);

    CWM_Sensor_Enable(IDX_REQUEST_SENSOR);

    gettimeofday(&tv, NULL);
    now = tv.tv_sec;
    tm_now = gmtime(&now);
    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;
    scl.iData[1] = tm_now->tm_year+1900;                    //year 100
    scl.iData[2] = tm_now->tm_mon+1;                     //mon  0
    scl.iData[3] = tm_now->tm_mday;                    //day  1
    scl.iData[4] = tm_now->tm_hour;                    //hour 0
    scl.iData[5] = tm_now->tm_min;                     //min  0
    scl.iData[6] = tm_now->tm_sec;                     //second
    CWM_SettingControl(SCL_DATE_TIME, &scl);
    printf("set CWM time %d/%d/%d %d:%d:%d\n", 
            tm_now->tm_year, 
            tm_now->tm_mon,
            tm_now->tm_mday,
            tm_now->tm_hour,
            tm_now->tm_min,
            tm_now->tm_sec);

    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;                   //always set 1
    scl.iData[1] = cywee_param.sedentary.trigger_time;         //trigger time (30 min)
    scl.iData[2] = cywee_param.sedentary.is_reminder_on;       //1:turn on; 0: turn off;
    scl.iData[3] = cywee_param.sedentary.reminder_time;        //reminder time (5 min)
    scl.iData[4] = cywee_param.sedentary.release_level;
    CWM_SettingControl(SCL_SEDENTARY, &scl);

    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;
    scl.iData[1] = cywee_param.body.age;                  //age
    scl.iData[2] = cywee_param.body.gender;               //gender 1:male, 0:female
    scl.iData[3] = cywee_param.body.weight;               //weight (kg)
    scl.iData[4] = cywee_param.body.height;               //height (cm)
    CWM_SettingControl(SCL_USER_INFO, &scl);

    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;
    scl.iData[1] = 5;
    scl.iData[2] = 5;
    scl.iData[3] = 5;
    CWM_SettingControl(SCL_HAND_UPDOWN_CONFIG, &scl);

    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;
    scl.iData[1] = cywee_param.sleep.check_time;
    scl.iData[2] = cywee_param.sleep.use_hr;
    scl.iData[3] = cywee_param.sleep.sleeping_level;
    scl.iData[4] = cywee_param.sleep.wake_level;
    CWM_SettingControl(SCL_SLEEP_CONFIG, &scl);
    enable_sleep_fuction(true);
    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;
    scl.iData[1] = 1;
    CWM_SettingControl(SCL_BIKING_CONFIG, &scl);


    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;
    scl.iData[4] = 1;
    CWM_SettingControl(SCL_PEDO_CONFIG, &scl);

    CWM_Sensor_Enable(IDX_ALGO_ACTIVITY_OUTPUT); 
    if (cywee_param.handup_down.enable) {
        CWM_Sensor_Enable(IDX_ALGO_WATCH_HANDUP);
    }
    if (cywee_param.sedentary.enable) {
        CWM_Sensor_Enable(IDX_ALGO_SEDENTARY);
    }

    CustomSensorData data;
    memset(&data, 0, sizeof(data));    
    data.fData[0] = 1.0;
    data.sensorType = CUSTOM_OFFBODY_DETECT;
    CWM_CustomSensorInput(&data);

    memset(&scl, 0, sizeof(scl));
    scl.iData[0] = 1;
    scl.iData[1] = ACTIVITY_MODE_NORMAL; 
    CWM_SettingControl(SCL_SET_ACTIVITY_MODE, &scl);

    struct k_timer motion_timer;
    k_timer_init(&motion_timer, input_motion_data, NULL);
    k_timer_start(&motion_timer, K_MSEC(50), K_MSEC(20)); 

    while(1)
    {
       
        if (cywee_param.change) {
            check_cywee_param_change();
        }
      
        next_duration = CWM_GetNextActionDuration_ns();
        if (next_duration > 0) {
        /* Do Low power process here
        */          
            k_msleep(next_duration / 1000000);
        }
        CWM_process();
    }
}

K_THREAD_DEFINE(cywee_motiom, 
			MOTION_STACK,
			cywee_motion_thread, 
			NULL, NULL, NULL,
			5,
			0, 
			100);