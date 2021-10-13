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
#include "gui_sport_cmd.h"


static GuiCfg_t gui_cfg;

void gui_set_sport_param(Sport_cfg_t *cfg)
{
    gui_cfg.CfgType = GUI_SPORT_CFG;
    gui_cfg.iData[0] = cfg->mode;
    gui_cfg.iData[1] = cfg->status;
    gui_cfg.iData[2] = cfg->goal_calorise;
    gui_cfg.iData[3] = cfg->goal_distance;
    gui_cfg.iData[4] = cfg->goal_time;
    gui_cfg.iData[5] = cfg->work_long;
    gui_cfg.iData[6] = cfg->gui_start;
    gui_cfg.iData[7] = cfg->gui_end;
    send_gui_msg_to_queue(&gui_cfg);
}

void gui_set_body_info(BodyInfo_t *info)
{
    gui_cfg.CfgType = GUI_BODY_INFO_CFG;
    gui_cfg.iData[0] = info->age;
    gui_cfg.iData[1] = info->gender;
    gui_cfg.iData[2] = info->weight;
    gui_cfg.iData[3] = info->height;
    gui_cfg.iData[4] = info->wearing_hand;
    send_gui_msg_to_queue(&gui_cfg);
}

void gui_get_sport_running_data(RunDataStruct_t *data)
{
    put_sport_running_data(data);
}