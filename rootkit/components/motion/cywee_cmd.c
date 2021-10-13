#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <init.h>
// #include <sys/byteorder.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include "cwm_lib.h"
// #include "simulate_signal.h"
// #include "gpio/gpio_apollo3p.h"
// #include "cywee_motion.h"
// #include <string.h>
// #include <logging/log.h>
// #include "fs/fs_interface.h"
// #include <sys/_timeval.h>
// #include <fs/fs.h>
// #include <sys/time.h>
#include <sys/_timeval.h>
#include <fs/fs.h>
#include <sys/time.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <drivers/i2c.h>
#include <string.h>
#include <sys/util.h>
#include <stdlib.h>
#include <logging/log.h>
#include "cywee_motion.h"
#include <sys/timeutil.h>
#include "drivers/counter.h"
#include "drivers_ext/rtc.h"
#include "fs/fs_interface.h"

static Sport_cfg_t param_cfg = {0};

void gui_set_sport_param(Sport_cfg_t *cfg);
void gui_set_body_info(BodyInfo_t *info);

static int set_sport_runing(void)
{
    if(param_cfg.status == SPORT_START)
    {
        printf("please end sport first\n");
        return 1;
    }
    param_cfg.mode = ACTIVITY_MODE_OUTDOOR_RUNNING;
    param_cfg.status = SPORT_START;
    //set_sport_config(&param_cfg);
    gui_set_sport_param(&param_cfg);
    return 0;
}


static int set_sport_biking(void)
{
    if(param_cfg.status == SPORT_START)
    {
        printf("please end sport first\n");
        return 1;
    }
    param_cfg.mode = ACTIVITY_MODE_OUTDOOR_BIKING;
    param_cfg.status = SPORT_START;
    //set_sport_config(&param_cfg);
    gui_set_sport_param(&param_cfg);
    return 0;
}

static int set_sport_treadmill(void)
{
    if(param_cfg.status == SPORT_START)
    {
        printf("please end sport first\n");
        return 1;
    }
    param_cfg.mode = ACTIVITY_MODE_TREADMILL;
    param_cfg.status = SPORT_START;
   //set_sport_config(&param_cfg);
    gui_set_sport_param(&param_cfg);
    return 0;
}

static int pause_sport(void)
{   
    param_cfg.status = SPORT_PAUSE;
    //set_sport_config(&param_cfg);
    gui_set_sport_param(&param_cfg);
    return 0;
}

static int end_sport(void)
{
    param_cfg.status = SPORT_END;
    //set_sport_config(&param_cfg);   
    gui_set_sport_param(&param_cfg);
    return 0;
}

static int read_sport_data(void)
{
    Sport_Data_t sData = {0};
    get_sport_data(&sData);
    printf("--------------------------SPORT DATA------------------------\n");
    printf("tatal steps : %.4f\n", sData.total_steps);
    printf("total distance : %.4f\n", sData.total_distance);
    printf("total cal : %.4f\n", sData.total_cal);
    printf("step freq : %.4f\n", sData.step_freq);
    printf("step length: %.4f\n", sData.step_lenth);
    printf("pace : %.4f\n", sData.pace);
    printf("elevation : %.4f\n", sData.elevation);
    printf("floor up : %.4f\n", sData.up_floors);
    printf("floor down : %.4f\n", sData.down_floors);
    printf("elevation up : %.4f\n", sData.elevation_up);
    printf("elevation down : %.4f\n", sData.elevation_down);
    printf("aver step_freq : %.4f\n", sData.aver_step_freq);
    printf("aver_step_length : %.4f\n", sData.aver_step_lenth);
    printf("aver_pace : %.4f\n", sData.aver_pace);
    printf("max step freq : %.4f\n", sData.max_step_freq);
    printf("max pace : %.4f\n", sData.max_pace);
    printf("HRR_Zone : %.4f\n", sData.HRR_zone);
    printf("HRR_intensity : %.4f\n", sData.HRR_intensity);
    printf("latitude : %.8f\n", sData.latitude_1+sData.latitude_2);
    printf("longitude : %.8f\n", sData.longitude_1 + sData.longitude_2);
    printf("speed : %.4f\n", sData.speed);
    printf("slope : %.4f\n", sData.slope);
    printf("VO2MAX : %.4f\n", sData.VO2max);
    printf("aver_speed : %.4f\n", sData.aver_speed);
    printf("max_speed : %.4f\n", sData.max_speed);
    printf("----------------------------SPORT END--------------------------\n");
    return 0;
}

static int read_daily_data(void)
{
    Daily_Activity_t dData = {0};
    get_daily_data(&dData);
    printf("--------------------------DAILY DATA------------------------\n");
    printf("tatal steps : %.4f\n", dData.total_steps);
    printf("total distance : %.4f\n", dData.total_distance);
    printf("total cal : %.4f\n", dData.total_cal);
    printf("step freq : %.4f\n", dData.step_freq);
    printf("step length: %.4f\n", dData.step_lenth);
    printf("pace : %.4f\n", dData.pace);
    printf("aver step_freq : %.4f\n", dData.aver_step_freq);
    printf("aver_step_length : %.4f\n", dData.aver_step_lenth);
    printf("aver_pace : %.4f\n", dData.aver_pace);
    printf("max step freq : %.4f\n", dData.max_step_freq);
    printf("max pace : %.4f\n", dData.max_pace);
    printf("VO2MAX : %.4f\n", dData.VO2max);
    printf("--------------------------DAILY DATA END------------------------\n");
    return 0;
}

static int read_sleep_data(void)
{
    struct sleep_info sData = {0};
    get_sleep_data(&sData);
    printf("--------------------------SLEEP DATA------------------------\n");
    printf("start time : %d/%d %d:%d \n", sData.start_time.mon, 
                            sData.start_time.day, 
                            sData.start_time.hour,
                            sData.start_time.min);
    printf("end time :  %d/%d %d:%d \n", sData.end_time.mon,
                            sData.end_time.day,
                            sData.end_time.hour,
                            sData.end_time.min);
    printf("total time : %d (min)\n", sData.total_time);
    printf("awakt time : %d (min)\n", sData.awake);
    printf("light time : %d (min)\n", sData.light);
    printf("rem time : %d (min)\n", sData.rem);
    printf("deep sleep : %d (min)\n", sData.deep_sleep);
    printf("sleep status : %d\n", sData.status);    
    printf("--------------------------SLEEP DATA END------------------------\n");
    return 0;
}

static int read_sleep_file(void)
{
    struct fs_file_t fp, rec_fp;
    File_req_t sleep_record = {0};    
    char fname[20] = FILE_REQ;
    int i = 0, j = 0;
    struct sleep_info sData = {0};
    int ret = 0;   
    fs_file_t_init(&rec_fp);
    ret = fs_open(&rec_fp, fname, FS_O_READ);

    if (ret) {
        printf("fs_open file %s error : %d\n", FILE_REQ, ret);
        return ret;
    }
    /*first is sport_rec data in the /home/rec_data*/
    fs_seek(&rec_fp, sizeof(sleep_record), FS_SEEK_SET);
    fs_read(&rec_fp, &sleep_record, sizeof(sleep_record));
    fs_close(&rec_fp);

    fs_file_t_init(&fp);

    for(i = 0; i < sleep_record.file_count; i++) {
        snprintf(fname, 20, "%s%d", SLEEP_FILE, sleep_record.file_name[i]);
        ret = fs_open(&fp, fname, FS_O_READ);
        
        if (ret) {
            printf("fs_open file %s error : %d\n", FILE_REQ, ret);
            return ret;
        }
        fs_seek(&fp, 0, FS_SEEK_SET);    
        fs_read(&fp, &sData, sizeof(sData));
        printf("------------------%s SLEEP DATA num : %d----------------------\n", fname, j);
        printf("start time : %d/%d %d:%d \n", sData.start_time.mon, 
                                sData.start_time.day, 
                                sData.start_time.hour,
                                sData.start_time.min);
        printf("end time :  %d/%d %d:%d \n", sData.end_time.mon,
                                sData.end_time.day,
                                sData.end_time.hour,
                                sData.end_time.min);
        printf("total time : %d (min)\n", sData.total_time);
        printf("awakt time : %d (min)\n", sData.awake);
        printf("light time : %d (min)\n", sData.light);
        printf("rem time : %d (min)\n", sData.rem);
        printf("deep sleep : %d (min)\n", sData.deep_sleep);
        printf("sleep status : %d\n", sData.status);    
        printf("--------------------------------------------------\n");      
        ret = fs_close(&fp);
    }
    return 0;
}

/*set_param <time(min)> <dist(meter)> <cal(Kcal)>*/
static int set_sport_param(const struct shell *shell, size_t argc, char **argv)
{
    memset(&param_cfg, 0, sizeof(param_cfg));
    param_cfg.mode = ACTIVITY_MODE_NORMAL;
    param_cfg.status = SPORT_END;
    param_cfg.goal_time = strtoul(argv[1], NULL, 10);
    param_cfg.goal_distance = strtoul(argv[2], NULL, 10);
    param_cfg.goal_calorise = strtoul(argv[3], NULL, 10);
    set_sport_config(&param_cfg);
    return 0;
}

/*set_user <age(year)> <gender(0 female, 1 male)> <height(cm)> <weight(kg)>*/
static int set_user_info(const struct shell *shell, size_t argc, char **argv)
{
    struct body_info user;
    user.age = strtoul(argv[1], NULL, 10);
    user.gender = strtoul(argv[2], NULL, 10);
    user.height = strtoul(argv[3], NULL, 10);
    user.weight = strtoul(argv[4], NULL, 10);
    user.wearing_hand = 0;
    body_info_change(&user);
    return 0;
}



SHELL_STATIC_SUBCMD_SET_CREATE(sub_sport_cmds,
                SHELL_CMD(start_running, NULL,
                    "running start", set_sport_runing),
                SHELL_CMD(start_biking, NULL,
                    "biking start", set_sport_biking),
                SHELL_CMD(start_treadmill, NULL,
                    "treadmill start", set_sport_treadmill),    
                SHELL_CMD(pause, NULL,
                    "sport pause", pause_sport),
                SHELL_CMD(end, NULL,
                    "sport end", end_sport),
                SHELL_CMD(sport_data, NULL,
                    "get sport data", read_sport_data),
                SHELL_CMD(daily_data, NULL,
                    "get daily data", read_daily_data),
                SHELL_CMD(sleep_data, NULL,
                    "get sleep data", read_sleep_data),  
                SHELL_CMD(read_sleep_file, NULL,
                    "read sleep storage file", read_sleep_file),               
                SHELL_CMD_ARG(set_param, NULL,
                        "set sport goal config; time, distdance, calories",
                        set_sport_param, 4, 5),                     
                SHELL_CMD_ARG(set_user, NULL,
                    "set user info; age, gender, height, weight",
                    set_user_info, 5, 5),          
                SHELL_SUBCMD_SET_END     /* Array terminated. */
                );

SHELL_CMD_REGISTER(sport, &sub_sport_cmds, "SPORT commands", NULL);

