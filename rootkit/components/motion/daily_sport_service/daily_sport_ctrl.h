
#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <init.h>
#include <sys/byteorder.h>
#include <stdio.h>
#include <stdlib.h>
#include "../cwm_lib.h"
#include "../simulate_signal.h"
#include "gpio/gpio_apollo3p.h"
#include <string.h>
#include <logging/log.h>
#include "fs/fs_interface.h"
#include <sys/_timeval.h>
#include <fs/fs.h>
#include <sys/time.h>
#include "drivers/counter.h"

#define GUI_SPORT_CFG       1
#define GUI_DAILY_GOAL_CFG  2
#define GUI_BODY_INFO_CFG   3 

typedef struct {
    float distance;
    float avr_pace;
}RunDataStruct_t, *pRunDataStruct_t;

typedef enum {
    GOAL_STEPS_FINISHED = 1,
    GOAL_ACTIVETY_HOURS_FINISHED = (0x1 << 1),
    GOAL_WORKOUT_MIN_FINISHED = (0x1 << 2),   
    GOAL_SET_MAX = (0x1 << 3)
}GOAL_FINISH_T;

typedef struct{
    uint32_t steps;
    uint32_t activety_hours;
    uint32_t workout_min;
}Daily_Goal_t;

typedef struct{
    uint32_t steps;
    uint32_t activety_hours;
    uint32_t workout_min;
    GOAL_FINISH_T finished;
}Daily_status_t;

typedef struct {
    Sport_cfg_t SportCfg;
    Daily_Goal_t Goalset;
    Daily_status_t GoalStatus;
}DailySport_t;

typedef struct {
    uint32_t CfgType;
    uint32_t index;
    union {
        struct  {
            float fData[16];
            };
        struct  {
            double dData[8];
            };
        struct  {
            int iData[16];
            };
        struct  {
            int    memSize; 
            void * memData;
            };
    };
} GuiCfg_t, *pGuiCfg_t;

void send_gui_msg_to_queue(GuiCfg_t *msg);
int set_sport_config(Sport_cfg_t *cfg);
void put_sport_running_data(pRunDataStruct_t data);

