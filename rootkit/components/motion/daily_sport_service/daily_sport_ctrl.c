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
// #include "daily_sport_ctrl.h"
#include "gui_sport_cmd.h"

File_req_t sport_file_rec = {0}, sleep_file_rec = {0};
static struct k_spinlock sport_spin_lock;
#define DAILY_STACK   4096

GuiCfg_t rx_cfg, tx_cfg;
SensorEVT_t rx_data, tx_data;
#define QUEUE_DEPTH 20
char daily_sport_buffer[QUEUE_DEPTH*sizeof(SensorEVT_t)];
struct k_msgq sport_msgq;
char gui_cfg_buffer[QUEUE_DEPTH *sizeof(GuiCfg_t)];
struct k_msgq gui_cfg_msgq;

#define MAX_SLEEP_DATA_NUM 60
//static struct Sleep_segment_Node sleep_seq_data[MAX_SLEEP_DATA_NUM];
Sport_cfg_t Sport_config;
Daily_Goal_t DailyGoalCfg;
BodyInfo_t BodyInfoCfg;
static Sport_Data_t sport_data = {0}, previous_data = {0}; //power_off 
static Sport_status_t previous_status;
static Daily_Activity_t Daily_data = {0}, pre_daily_data = {0};
static struct sleep_info sleep_data;


void put_sport_running_data(pRunDataStruct_t data)
{   
    data->distance = sport_data.total_distance;
    data->avr_pace = sport_data.aver_pace;
}
 
void send_motion_msg_to_queue(pSensorEVT_t msg)
{
    int ret;
    printf("send_motion_msg_to_queue \n");
    if(msg != NULL) {
        memcpy(&tx_data, msg, sizeof(SensorEVT_t));  
        ret = k_msgq_put(&sport_msgq, &tx_data, K_NO_WAIT);
        if (ret) {
            printf("motion_msg put error : %d\n", ret);
            k_msgq_purge(&sport_msgq);
        }
    }
}

void send_gui_msg_to_queue(GuiCfg_t *msg)
{
    printf("send_gui_msg_to_queue \n");
    int ret;
    if(msg != NULL) {
        memcpy(&tx_cfg, msg, sizeof(GuiCfg_t)); 
        ret = k_msgq_put(&gui_cfg_msgq, &tx_cfg, K_NO_WAIT);
        if (ret) {
            printf("GUI_sport_msg put error\n");
            k_msgq_purge(&gui_cfg_msgq);
        }
    }
}

static struct Sleep_segment_Node *head;
static int m_req_sensor_state = 0;
static const char* REQ_SENS_NAME[] = {"acc", "gyro", "mag", "baro", "temp", "hr", "gnss", "ob", "oc", "accany"};
void cywee_motion_proc_func(pSensorEVT_t sensorEVT)
{
    int iData[16] = {0};
    printf("&&&&&&&&&  ");
    switch (sensorEVT->sensorType) {        
        case IDX_REQUEST_SENSOR:
            if (sensorEVT->fData[2] != 0)
                m_req_sensor_state |= (1<<(int)sensorEVT->fData[1]);
            else
                m_req_sensor_state &= ~(1<<(int)sensorEVT->fData[1]);

            printf("req_sensor input_sensor: %s(%.1f), switch: %.1f\n",\
                REQ_SENS_NAME[(int)sensorEVT->fData[1]], \
                sensorEVT->fData[1], sensorEVT->fData[2]);
            printf("input_sensor now: acc=%d gyro=%d mag=%d baro=%d \
                    temp=%d hr=%d gnss=%d ob=%d oc=%d accany=%d\n",\
                getReqSensor(0), getReqSensor(1), getReqSensor(2), \
                getReqSensor(3), getReqSensor(4), getReqSensor(5), \
                getReqSensor(6), getReqSensor(7), getReqSensor(8), getReqSensor(9));
        break;
        case IDX_ALGO_SEDENTARY:
            printf("%s: %.4f, %.4f, %.4f\n", "sedentary",
                    sensorEVT->fData[0],
                    sensorEVT->fData[1],
                    sensorEVT->fData[2]);
        break;
        case IDX_ACCEL:
            printf("%s: %.4f, %.4f, %.4f\n", "accel",
                    sensorEVT->fData[0],
                    sensorEVT->fData[1],
                    sensorEVT->fData[2]);
        break;
        case IDX_GYRO:
            printf("%s: %.4f, %.4f, %.4f\n", "gyro",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1],
                        sensorEVT->fData[2]);
        break;
        case IDX_MAG:
            printf("%s: %.4f, %.4f, %.4f,%.4f\n", "mag",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1],
                        sensorEVT->fData[2],
                        sensorEVT->fData[3]);
        break;
        case IDX_BARO:
            printf("%s: %.4f\n", "baro",
                        sensorEVT->fData[0]);
        break;
        case IDX_TEMP:
              printf("%s: %.4f\n", "temp",
                        sensorEVT->fData[0]);
        break;
        case IDX_HEARTRATE:
            printf("%s: %.4f, %.4f\n", "heart rate",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1]);
        break;
        case IDX_GNSS:
            printf("%s: %.4f, %.4f, %.4f, %.4f, %.4f, \
                    %.4f, %.4f, %.4f, %.4f\n", "gnss",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1],
                        sensorEVT->fData[2],
                        sensorEVT->fData[3],
                        sensorEVT->fData[4],
                        sensorEVT->fData[5],
                        sensorEVT->fData[6],
                        sensorEVT->fData[7],
                        sensorEVT->fData[8]);
        break;
        case IDX_OFFBODY_DETECT:
            printf("%s: %.4f, %.4f, %.4f\n", "offbody",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1],
                        sensorEVT->fData[2]);
        break;
        case IDX_ALGO_WATCH_HANDUP:
            printf("%s: %.4f, %.4f, %.4f\n", "watch handup",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1],
                        sensorEVT->fData[2]);
            iData[1] = sensorEVT->fData[1];
            handup_down_msg_send(iData[1]);
        break;
        case IDX_ALGO_SLEEP:
            printf("%s: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\n", "sleep",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1],
                        sensorEVT->fData[2],
                        sensorEVT->fData[3],
                        sensorEVT->fData[4],
                        sensorEVT->fData[5],
                        sensorEVT->fData[6]);
             
           
            for(int i = 0; i < 7; i++) {
                iData[i] = sensorEVT->fData[i];
                printf("i = %d, iData[%d] = %d, sensorEVT->fData[%d] = %f\n",
                    i, i, iData[i], i, sensorEVT->fData[i]);
            }
            printf("%s: %d, %d, %d, %d, %d, %d, %d\n", "sleep iData",
                    iData[0],
                    iData[1],
                    iData[2],
                    iData[3],
                    iData[4],
                    iData[5],
                    iData[6]);
            if (iData[0] == 1) {
                switch (iData[2]) {
                case FALL_ASLEEP:
                    memset(&sleep_data, 0, sizeof(sleep_data));
                    sleep_data.status = FALL_ASLEEP;
                break;
                case IN_SLEEP:
                    head = sleep_data.seg_data;
                    sleep_data.status = IN_SLEEP;
                    sleep_data.start_time.mon = iData[3];
                    sleep_data.start_time.day = iData[4];
                    sleep_data.start_time.hour = iData[5];
                    sleep_data.start_time.min = iData[6];
                break;
                case END_SLEEP:
                    if(sleep_data.status != END_SLEEP) {
                        sleep_data.end_time.mon = iData[3];
                        sleep_data.end_time.day = iData[4];
                        sleep_data.end_time.hour = iData[5];
                        sleep_data.end_time.min = iData[6];
                        head = NULL;
                        head = sleep_data.seg_data;
                        struct Sleep_segment_Node *temp;
                        temp = head->next;
                        while (temp != NULL) {
                            head->data.end.mon = temp->data.start.mon;
                            head->data.end.day = temp->data.start.day;
                            head->data.end.hour = temp->data.start.hour;
                            head->data.end.min = temp->data.start.min;
                            head = temp;
                            temp = temp->next;
                        }
                        head->data.end.mon = iData[3];
                        head->data.end.day = iData[4];
                        head->data.end.hour = iData[5];
                        head->data.end.min = iData[6];
                        save_sleep_data();
                    }
                break;
                case LAST_MSG:
                     if(sleep_data.status != END_SLEEP){
                        sleep_data.end_time.mon = iData[3];
                        sleep_data.end_time.day = iData[4];
                        sleep_data.end_time.hour = iData[5];
                        sleep_data.end_time.min = iData[6];
                    }
                break;
                case LINGHT_SLEEP:                   
                case DEEP_SLEEP:                   
                case REM:               
                case WAKE:                                    
                    head->data.type = iData[2];
                    head->data.start.mon = iData[3];
                    head->data.start.day = iData[4];
                    head->data.start.hour = iData[5];
                    head->data.start.min = iData[6];
                    head->next = (struct Sleep_segment_Node *)CWM_OS_malloc(sizeof(struct sleep_info));    
                    head = head->next;
                    sleep_data.segment_num++;
                break;

                default:
                break;
                }
            }
            if (iData[0] == 2) {
                sleep_data.total_time = iData[1];  //min
                sleep_data.light = iData[2];
                sleep_data.deep_sleep = iData[3];
                sleep_data.awake = iData[4];
                sleep_data.rem = iData[5];
                save_sleep_data();
                printf("sleep total data\n");
            }
            
        break;
        case IDX_ALGO_ACTTYPEDETECT:
            printf("%s: %.4f, %.4f\n", "act type",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1]);
        break;
        case IDX_ALGO_SHAKE:
            printf("%s: %.4f, %.4f\n", "shake",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1]);
        break;
        case IDX_ALGO_ACTIVITY_OUTPUT:
            printf("%s: %.4f, %.4f, %.4f, %.4f, "\
                    "%.4f, %.4f, %.4f, %.4f, "\
                    "%.4f, %.4f, %.4f, %.4f, "\
                    "%.4f, %.4f, %.4f, %.4f\n", "activity output",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1],
                        sensorEVT->fData[2],
                        sensorEVT->fData[3],
                        sensorEVT->fData[4],
                        sensorEVT->fData[5],
                        sensorEVT->fData[6],
                        sensorEVT->fData[7],
                        sensorEVT->fData[8],
                        sensorEVT->fData[9],
                        sensorEVT->fData[10],
                        sensorEVT->fData[11],
                        sensorEVT->fData[12],
                        sensorEVT->fData[13],
                        sensorEVT->fData[14],
                        sensorEVT->fData[15]);
            iData[0] = sensorEVT->fData[0];           
            switch (iData[0]) {
            case ACTIVITY_MODE_NORMAL:
                iData[4] = sensorEVT->fData[4];
                Daily_data.total_steps = sensorEVT->fData[1] + pre_daily_data.total_steps;
                Daily_data.total_distance = sensorEVT->fData[2] + pre_daily_data.total_distance;
                Daily_data.total_cal = sensorEVT->fData[3] + pre_daily_data.total_cal;
                if ((iData[4] == MOTION_WALK) || (iData[6] == MOTION_RUN)) {
                    Daily_data.step_freq = sensorEVT->fData[5];
                    Daily_data.step_lenth = sensorEVT->fData[6];
                    Daily_data.pace = sensorEVT->fData[7];
                }
                break;
            case ACTIVITY_MODE_TREADMILL:
            case ACTIVITY_MODE_OUTDOOR_RUNNING:
                iData[4] = sensorEVT->fData[4];
                sport_data.total_steps = sensorEVT->fData[1] \
                    + previous_data.total_steps;
                sport_data.total_distance = sensorEVT->fData[2]\
                    + previous_data.total_distance;
                sport_data.total_cal = sensorEVT->fData[3]\
                    + previous_data.total_cal;
                if ((iData[4] == MOTION_WALK) || (iData[6] == MOTION_RUN)) {                   
                    sport_data.active_type = iData[4];
                    sport_data.step_freq = sensorEVT->fData[5];
                    sport_data.step_lenth = sensorEVT->fData[6];
                    sport_data.pace = sensorEVT->fData[7];
                    sport_data.elevation = sensorEVT->fData[9];
                    sport_data.up_floors = sensorEVT->fData[10]\
                        + previous_data.up_floors;
                    sport_data.down_floors = sensorEVT->fData[11]\
                        + previous_data.down_floors;
                    sport_data.elevation_up = sensorEVT->fData[12]\
                        + previous_data.elevation_up;
                    sport_data.elevation_down = sensorEVT->fData[13]\
                        + previous_data.elevation_down;
                }   
                break;
            case ACTIVITY_MODE_OUTDOOR_BIKING:
                sport_data.latitude_1 = sensorEVT->fData[1];
                sport_data.latitude_2 = sensorEVT->fData[2];
                sport_data.longitude_1 = sensorEVT->fData[3];
                sport_data.longitude_2 = sensorEVT->fData[4];
                sport_data.total_distance = sensorEVT->fData[5]\
                        + previous_data.total_distance;
                sport_data.speed = sensorEVT->fData[6];
                sport_data.elevation = sensorEVT->fData[7];
                sport_data.slope = sensorEVT->fData[8];
                sport_data.total_cal = sensorEVT->fData[9]\
                        + previous_data.total_cal;
                sport_data.VO2max = MAX(sensorEVT->fData[10], previous_data.VO2max);
                sport_data.HRR_intensity = sensorEVT->fData[11];
                sport_data.HRR_zone = sensorEVT->fData[12];
                break;
            case NORMAL_EXTRA:
                if (Sport_config.mode == ACTIVITY_MODE_NORMAL) {
                    Daily_data.aver_step_freq = sensorEVT->fData[1];
                    Daily_data.aver_step_lenth  = sensorEVT->fData[2];
                    Daily_data.aver_pace  = sensorEVT->fData[3];
                    Daily_data.max_step_freq  = MAX(sensorEVT->fData[4], 
                        pre_daily_data.max_step_freq);
                    Daily_data.max_pace  = MAX(sensorEVT->fData[5], pre_daily_data.max_pace);
                    Daily_data.VO2max  = MAX(sensorEVT->fData[6], pre_daily_data.VO2max);
                }else {
                    sport_data.aver_step_freq = sensorEVT->fData[1];
                    sport_data.aver_step_lenth = sensorEVT->fData[2];
                    sport_data.aver_pace = sensorEVT->fData[3];
                    sport_data.max_step_freq = MAX(sensorEVT->fData[4], 
                        previous_data.max_step_freq);
                    sport_data.max_pace = MAX(sensorEVT->fData[5], previous_data.max_speed);
                    sport_data.VO2max = MAX(sensorEVT->fData[6], previous_data.VO2max);
                    sport_data.HRR_intensity = sensorEVT->fData[7];
                    sport_data.HRR_zone = sensorEVT->fData[8];
                    sport_data.uncal_dist = sensorEVT->fData[9];
                }
                break;
            case OUTDOOR_BIKING_EXTRA:
                sport_data.aver_speed = sensorEVT->fData[1];
                sport_data.max_speed = MAX(sensorEVT->fData[2], 
                    previous_data.max_speed);                
                sport_data.elevation_up = sensorEVT->fData[3]\
                    + previous_data.elevation_up;
                sport_data.elevation_down = sensorEVT->fData[4]\
                    + previous_data.elevation_down;
            default :
                break;
            }
        break;
        case IDX_ALGO_ANY_MOTION:
            printf("%s: %.4f, %.4f\n", "any motion",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1]);
        break;
        case IDX_ALGO_NO_MOTION:
            printf("%s: %.4f, %.4f\n", "no motion",
                        sensorEVT->fData[0],
                        sensorEVT->fData[1]);
        break;
    }
}

void save_daily_data(void)
{

}

void reset_daily_data()
{
    SettingControl_t scl;      
    CWM_SettingControl(SCL_PEDO_RESET, &scl);

    if((sleep_data.status == END_SLEEP) || \
    (sleep_data.status == LAST_MSG)) {
        memset(&sleep_data, 0, sizeof(sleep_data));
    }

    if(Sport_config.mode == ACTIVITY_MODE_NORMAL) {
        memset(&sport_data, 0, sizeof(sport_data));
    }
    save_daily_data();
}

/*save last sport data*/
void save_last_sport_data(void){
     /*write later*/
    k_spinlock_key_t key;
    key = k_spin_lock(&sport_spin_lock);
    memcpy(&previous_data, &sport_data, sizeof(sport_data));
    k_spin_unlock(&sport_spin_lock, key); 
}

int save_sleep_data()
{   
    int ret;
    char fname[NAME_LEN], date[15];
    //num_to_sfile(sleep_file_rec.file_index, fname);
    struct fs_file_t fp;
    struct timeval tv;
    time_t now;
    struct tm *tm_now; 

    if (sleep_file_rec.file_count >= MAX_FILE) {
        /**
         * delete file_name[0]; write_file[6];
         * move file_name[1] << 1
         * move data_count[1] << 1
        */
        snprintf(fname, NAME_LEN, "%s%d", SLEEP_FILE, sleep_file_rec.file_name[0]);
        fs_unlink(fname);
        sleep_file_rec.file_count = MAX_FILE -1;        
        memcpy(&(sleep_file_rec.file_name[0]),&(sleep_file_rec.file_name[1]), (MAX_FILE-1) * sizeof(uint32_t));
        memcpy(&sleep_file_rec.data_count[0], &sleep_file_rec.data_count[1], (MAX_FILE-1) * sizeof(uint32_t));
        sport_file_rec.data_count[sport_file_rec.file_count] = 0;
    }
    gettimeofday(&tv, NULL);
    now = tv.tv_sec;
    tm_now = gmtime(&now);

    if(sleep_file_rec.file_count == 0) {
        if (sleep_file_rec.data_count[sleep_file_rec.file_count] == 0) {
            memset(fname, 0, NAME_LEN);
           
            
            snprintf(date, 15, "%04d%02d%02d",
                tm_now->tm_year + 1900,
                tm_now->tm_mon + 1,
                tm_now->tm_wday);
            snprintf(fname, NAME_LEN, "%s%s", SLEEP_FILE, date);  
            sleep_file_rec.file_name[sleep_file_rec.file_count] = strtoul(date, NULL, 10); 
            sleep_file_rec.data_count[sleep_file_rec.file_count]++;
            sleep_file_rec.file_count++;
        }
    } else {
       
        if ((sleep_file_rec.file_name[sleep_file_rec.file_count] % 100) == tm_now->tm_wday){
            sleep_file_rec.data_count[sleep_file_rec.file_count -1]++;
        }
        else {
            snprintf(date, 15, "%04d%02d%02d",
                tm_now->tm_year + 1900,
                tm_now->tm_mon + 1,
                tm_now->tm_wday);           
            sleep_file_rec.file_name[sleep_file_rec.file_count] = strtoul(date, NULL, 10); 
            sleep_file_rec.data_count[sleep_file_rec.file_count]++;
            sleep_file_rec.file_count++;
        }
        snprintf(fname, 20, "%s%d", SLEEP_FILE, sleep_file_rec.file_name[sleep_file_rec.file_count - 1]);

    }

    fs_file_t_init(&fp);
    ret = fs_open(&fp, fname, FS_O_RDWR|FS_O_CREATE|FS_O_APPEND);

    if (ret) {
        printf("open file %s error : %d\n", fname, ret);
        return ret;
    }
    fs_seek(&fp, 0, FS_SEEK_END);
    fs_write(&fp, &sleep_data, sizeof(sleep_data));
    struct Sleep_segment_Node *Head, *temp;
    Head = sleep_data.seg_data;
    temp = Head;
    for(int i = 0; i < sleep_data.segment_num; i++)
    {
        Head = Head->next;
        fs_write(&fp, &temp->data, sizeof(SLEEP_SEGMEMT_TYPE_T));
        CWM_OS_free(temp);
        temp = Head;  
    }
    fs_close(&fp);   
    save_sleep_record();
    return 0;
}

int save_sport_data(void)
{
    /*write later*/
    int ret; 
    char fname[NAME_LEN], date[15];    
    struct fs_file_t fp;
     struct timeval tv;
    time_t now;
    struct tm *tm_now; 

    /*count full*/
    if(sport_file_rec.file_count >= MAX_FILE) {
        sport_file_rec.file_count = MAX_FILE - 1;
        snprintf(fname, NAME_LEN, "%s%d", SPORT_FILE, sport_file_rec.file_name[0]);
        fs_unlink(fname);
        memcpy(&(sport_file_rec.file_name[0]), &(sport_file_rec.file_name[1]), (MAX_FILE -1) * sizeof(uint32_t));
        memcpy(&(sport_file_rec.data_count[0]), &(sport_file_rec.data_count[1]), (MAX_FILE -1) * sizeof(uint32_t));
        sport_file_rec.data_count[sport_file_rec.file_count] = 0;
    }    

    memset(fname, 0, NAME_LEN); 
    gettimeofday(&tv, NULL);
    now = tv.tv_sec;
    tm_now = gmtime(&now);

    if(0 == sport_file_rec.file_count) { 
        snprintf(date, 15, "%04d%02d%02d",tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday);
        snprintf(fname, NAME_LEN, "%s%s", SPORT_FILE, date);
        sport_file_rec.file_name[sport_file_rec.file_count] = strtoul(date, NULL, 10);
        sport_file_rec.data_count[sport_file_rec.file_count]++;
        sport_file_rec.file_count++;
    } else {
        if (tm_now->tm_mday == (sport_file_rec.file_name[sport_file_rec.file_count] % 100)) {
            sport_file_rec.data_count[sport_file_rec.file_count]++;            
        }else {
            snprintf(date, 15, "%04d%02d%02d",tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday);
            sleep_file_rec.file_name[sleep_file_rec.file_count] = strtoul(date, NULL, 10); 
            sleep_file_rec.data_count[sleep_file_rec.file_count]++;
            sleep_file_rec.file_count++;
        }
        snprintf(fname, NAME_LEN, "%s%d", SPORT_FILE, sleep_file_rec.file_name[sleep_file_rec.file_count-1]);
    } 

    fs_file_t_init(&fp);    
    ret = fs_open(&fp, fname, FS_O_RDWR|FS_O_CREATE|FS_O_APPEND);

    if (ret) {
        printf("open file %s error : %d\n", fname, ret);
        return ret;
    }
    fs_seek(&fp, 0, FS_SEEK_END);
    fs_write(&fp, &sport_data, sizeof(sport_data));
    fs_close(&fp);
    memset(&previous_data, 0, sizeof(previous_data));
    save_sport_record();
    return 0;
}

int save_sport_record(void)
{
    struct fs_file_t rec_fp;
    fs_file_t_init(&rec_fp);
    char fname[] = FILE_REQ;
    int ret = 0;
    ret = fs_open(&rec_fp, fname, FS_O_RDWR|FS_O_CREATE);    

    if (ret) {
        printf("open file %s error : %d\n", FILE_REQ, ret);
        return ret;
    }
    fs_seek(&rec_fp, 0, FS_SEEK_SET);
    fs_write(&rec_fp, &sport_file_rec, sizeof(sport_file_rec));
    fs_close(&rec_fp);
    return ret;
}

int save_sleep_record(void)
{
    struct fs_file_t rec_fp;
    fs_file_t_init(&rec_fp);
    char fname[] = FILE_REQ;
    int ret = 0;
    ret = fs_open(&rec_fp, fname, FS_O_RDWR|FS_O_CREATE);    

    if (ret) {
        printf("open file %s error : %d\n", FILE_REQ, ret);
        return ret;
    }
    fs_seek(&rec_fp, sizeof(sport_file_rec), FS_SEEK_SET);
    fs_write(&rec_fp, &sleep_file_rec, sizeof(sleep_file_rec));
    fs_close(&rec_fp);
    return ret;
}

int read_param_from_file(void)
{
    off_t file_len;
    struct fs_file_t rec_fp;    
    int ret = 0;
    char fname[] = FILE_REQ;
  	
    fs_file_t_init(&rec_fp);
    ret = fs_open(&rec_fp, fname, FS_O_RDWR|FS_O_CREATE);    

    if (ret) {
        printf("open file %s error : %d\n", FILE_REQ, ret);
        return ret;
    }

    fs_seek(&rec_fp, 0, FS_SEEK_END);
    file_len = fs_tell(&rec_fp);
    fs_seek(&rec_fp, 0, FS_SEEK_SET);

    if (file_len < sizeof(sport_file_rec)) {        
        memset(&sport_file_rec, 0, sizeof(sport_file_rec)); 
        memset(&sleep_file_rec, 0, sizeof(sleep_file_rec));        
        fs_write(&rec_fp, &sport_file_rec, sizeof(sport_file_rec));
        fs_write(&rec_fp, &sleep_file_rec, sizeof(sleep_file_rec));
    }
    else {
        fs_read(&rec_fp, &sport_file_rec, sizeof(sport_file_rec));
        fs_read(&rec_fp, &sleep_file_rec, sizeof(sleep_file_rec));
    }

    fs_close(&rec_fp);
    return 0;
}

void gui_cmd_proc(GuiCfg_t *cfg)
{
    printf("gui_cmd_proc type : %d\n", cfg->CfgType);
    switch (cfg->CfgType) {
    case GUI_SPORT_CFG:
        Sport_config.mode = cfg->iData[0];
        Sport_config.status = cfg->iData[1];
        Sport_config.goal_calorise = cfg->iData[2];
        Sport_config.goal_distance = cfg->iData[3];
        Sport_config.goal_time = cfg->iData[4];
        set_sport_config(&Sport_config);
        if (Sport_config.status == SPORT_START) {
            previous_status = Sport_config.status;
        } else if (Sport_config.status == SPORT_END) {
            if (previous_status != Sport_config.status) { 
                sport_data.workout_long = cfg->iData[5]; 
                sport_data.tm_start = cfg->iData[6]; 
                sport_data.tm_end = cfg->iData[7];  
                previous_status = Sport_config.status;
                save_sport_data();                      
            } 
        } else {
                if (previous_status == SPORT_START) {
                    previous_status = Sport_config.status;
                    memcpy(&previous_data, &sport_data, sizeof(Sport_Data_t));
                }
        }  
    break;
    case GUI_DAILY_GOAL_CFG:
        DailyGoalCfg.steps = cfg->iData[0];
        DailyGoalCfg.activety_hours = cfg->iData[1];
        DailyGoalCfg.workout_min = cfg->iData[2];
    break;
    case GUI_BODY_INFO_CFG:
        BodyInfoCfg.age = cfg->iData[0];
        BodyInfoCfg.gender = cfg->iData[1];
        BodyInfoCfg.weight = cfg->iData[2];
        BodyInfoCfg.height = cfg->iData[3];
        BodyInfoCfg.wearing_hand = cfg->iData[4];
        body_info_change(&BodyInfoCfg);
    break;
    default:
    break;
    }
}

void get_sleep_data(struct sleep_info *data)
{
    if(data != NULL) {
        memcpy(data, &sleep_data, sizeof(sleep_data));
    }    
}

void get_daily_data(Daily_Activity_t *data)
{
    if(data != NULL) {
        memcpy(data, &Daily_data, sizeof(Daily_data));
    }
}

void get_sport_data(Sport_Data_t *data)
{
    if(data != NULL) {
        memcpy(data, &sport_data, sizeof(sport_data));
    }
}

void init_daily_spot_param(void)
{
    DailyGoalCfg.steps = 0;
    DailyGoalCfg.activety_hours = 0;
    DailyGoalCfg.workout_min = 0;
}

void daily_sport_srv_thread(void)
{  
    struct timeval tv;
    time_t now;
    struct tm *tm_now; 
    static bool data_reset = false;

    read_param_from_file();  
    init_daily_spot_param();      
    k_msgq_init(&sport_msgq, daily_sport_buffer, sizeof(SensorEVT_t), QUEUE_DEPTH);
    k_msgq_init(&gui_cfg_msgq, gui_cfg_buffer, sizeof(GuiCfg_t), QUEUE_DEPTH);
    while(1) {   

        gettimeofday(&tv, NULL);
        now = tv.tv_sec;
        tm_now = gmtime(&now);

        if((tm_now->tm_hour == 0)&& (tm_now->tm_min == 0) && (tm_now->tm_sec < 5)) {
            if(data_reset == false) {
                reset_daily_data();
                data_reset = true;
            }
        } else{
            data_reset = false;
        }

        if (!k_msgq_get(&sport_msgq, &rx_data, K_NO_WAIT)) {
            cywee_motion_proc_func(&rx_data);
        }
        if (!k_msgq_get(&gui_cfg_msgq, &rx_cfg, K_NO_WAIT)) {
            gui_cmd_proc(&rx_cfg);
        }
        k_msleep(20);
    }  
}

K_THREAD_DEFINE(daily_sport_srv, 
			DAILY_STACK,
			daily_sport_srv_thread, 
			NULL, NULL, NULL,
			5,
			0, 
			0);