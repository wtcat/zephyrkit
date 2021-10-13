#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <init.h>
#include <sys/byteorder.h>
#include <stdio.h>
#include <stdlib.h>
#include <posix/time.h>
#include "cwm_lib.h"
#include "simulate_signal.h"
#include "gpio/gpio_apollo3p.h"
//#include "gui_sport_cmd.h"

#define FILE_REQ    "/home/rec_data"
/*all sport data name is "/home/sport_202104151737"*/
#define SPORT_FILE   "/home/sport_"
#define SLEEP_FILE   "/home/sleep_"
#define NAME_LEN  30

#define MAX_FILE   7
typedef struct {
    uint32_t file_name[MAX_FILE]; //20210518
    uint32_t data_count[MAX_FILE];
    uint8_t file_count;
}File_req_t;

typedef struct body_info {
    /* data */
    int age;            //default: 30
    int gender;         //default: male, 1:male, 0: female
    int weight;         //(kg)default: 70, (10-300)
    int height;         //(cm)default: 175, (30-300)
    int wearing_hand;   //default: left hand, 0:left hand, 1: right hand;  
}BodyInfo_t;

struct sedentary_info {
    bool enable;             //app set,default 1, true:enable, false:disable;
    bool status;             //run status, true:checking false:not checking;
    int trigger_time;       //(min)default: 30; 20-200
    int is_reminder_on;     //default(0) turn off; 2: turn off, 1: turn on
    int reminder_time;      //(min) default: 5; (1-10)
    int release_level;      //default: 5; 0:default, 3: easy release, 4 5: not easy 
};

struct pedo_config {
    int floor_height;           //(cm) default: 380cm=3.8m
    int anti;                   //pedometer adjust level, default 4, 1->7 easy->hard
    int pace_dist;              //pace distance adjust, default 4, 1->7 short->long
    int enable_extra;           //enable extra packet, 0: default:not send, 1:send, 2: not send
    int treadmill_steps;        //steps default:0(don't adjust)
    int treadmill_step_freq;    //(steps/minute) default:0 (don't adjust)
    int treadmill_dist;         //(meters) default:0
    int treadmill_truth_dist;   //(meters)
};

struct biking_config {
    int enable_extra_packet;    //default don't send;
};

struct handup_down_cfg {
    int enable;         //default:0 1:turn on, 2: turn off;
};
 
struct sleep_config {
    int enable;         //default:0 1:turn on, 2: turn off;
    int check_time;     //defaut:0 (60min)  60, 90
    int use_hr;         //defaul:don't use hr  1:use hr, 2:don't use
    int sleeping_level; //default: 4, 1->7 : hard->easy sleep;
    int wake_level;     //default: 4, 1->7 : hard->easy wake;
    int status;
};

typedef enum{
    IN_SLEEP = 0,    
    LINGHT_SLEEP = 1,
    DEEP_SLEEP =2,
    WAKE = 3,
    END_SLEEP = 4,
    FALL_ASLEEP = 6,
    LAST_MSG = 9,
    REM = 12,
}SLEEP_STATUS;

typedef struct sleep_tm {
    uint32_t mon;
    uint32_t day;
    uint32_t hour;
    uint32_t min;
}SleepTm_t;

typedef struct {
    SleepTm_t start;
    SleepTm_t end;
    SLEEP_STATUS type;
}SLEEP_SEGMEMT_TYPE_T;

struct Sleep_segment_Node{
    SLEEP_SEGMEMT_TYPE_T data;
    struct Sleep_segment_Node *next;
};

typedef struct sleep_info {
    SLEEP_STATUS status;
    SleepTm_t start_time;
    SleepTm_t end_time;
    uint32_t light;
    uint32_t rem;
    uint32_t deep_sleep;
    uint32_t awake;
    uint32_t total_time;
    uint32_t segment_num;
    struct Sleep_segment_Node *seg_data;
}SLEEP_DATA_STRUCT_DEF;

typedef enum {
    MOTION_UNKNOWN = 0,
    MOTION_STATIC = 1, 
    MOTION_WALK = 2,
    MOTION_UPSTAIRS = 4,
    MOTION_DOWNSTAIRS = 5,
    MOTION_RUN = 6,
    MOTION_BIKE = 7,
    MOTION_VEHICLE = 8,
}Activity_type_t;

typedef struct {
    float total_steps;
    float total_distance;
    float total_cal;  //Kcal
    float step_freq;
    float step_lenth;
    float pace;   
    float aver_step_freq;
    float aver_step_lenth;
    float aver_pace;
    float max_step_freq;
    float max_pace;
    float VO2max;            //ml/kg/min
}Daily_Activity_t;

typedef enum {
    ACTIVITY_MODE_NORMAL = 1001,
    ACTIVITY_MODE_TREADMILL = 1002,
    ACTIVITY_MODE_OUTDOOR_RUNNING = 1003,
    ACTIVITY_MODE_OUTDOOR_BIKING = 3001,
    NORMAL_EXTRA = 100,
    OUTDOOR_BIKING_EXTRA = 120
}Motion_mode;

typedef enum {
    SPORT_END = 0,
    SPORT_START = 1,
    SPORT_PAUSE = 2
}Sport_status_t;

typedef struct {
    Motion_mode mode;
    Sport_status_t status;
    uint32_t goal_calorise;  // Kcal; default:0
    uint32_t goal_distance;  //meters; default : 0
    uint32_t goal_time;      //min;   default: 0
    uint32_t work_long;     //sec;
    uint32_t gui_start;
    uint32_t gui_end;
}Sport_cfg_t;

typedef struct {    
    float total_steps;
    float total_distance;
    float total_cal;  //Kcal
    Activity_type_t active_type;
    float step_freq;
    float step_lenth;
    float pace;
    float elevation;   //meters
    float up_floors;
    float down_floors;   //floors
    float elevation_up;  // meters
    float elevation_down;    //meters
    float aver_step_freq;
    float aver_step_lenth;
    float aver_pace;
    float max_step_freq;
    float max_pace;
    float VO2max;            //ml/kg/min
    float HRR_intensity;     
    float HRR_zone;
    float uncal_dist;        //meters
    float latitude_1;       //degrees
    float latitude_2;       //degrees;  precision = latitude_1+2
    float longitude_1;      //degrees;
    float longitude_2;      //degrees;  precision=longitude1+2
    float speed;            //(km/hour)
    float slope;            //percentage
    float aver_speed;       //km/hour
    float max_speed;        //km/kour
    int tm_start;
    int tm_end;
    uint32_t workout_long;
}Sport_Data_t;

struct cywee_motion
{
    uint32_t change;
    Sport_cfg_t sport_cfg;
    struct body_info  body;
    struct sedentary_info sedentary;
    struct pedo_config adjust;
    struct biking_config biking;
    struct handup_down_cfg handup_down;
    struct sleep_config sleep;
};

int save_sleep_data();
int save_sleep_record(void);
void enable_sedentary(uint8_t enable);
void enable_handup_down(uint8_t enable);
int body_info_change(struct body_info *user);
void get_sleep_data(struct sleep_info *data);
int set_sport_config(Sport_cfg_t *cfg);
void get_sport_data(Sport_Data_t *data);
void get_daily_data(Daily_Activity_t *data);
int save_sport_record(void);
// void num_to_sfile(int num, char* fname);
// void num_to_file(int num, char* fname);
int motion_get_time(void);
void send_motion_msg_to_queue(pSensorEVT_t msg);
int getReqSensor(int index);
void* CWM_OS_malloc(int size);
void CWM_OS_free(void *ptr);
void handup_down_msg_send(int msg);