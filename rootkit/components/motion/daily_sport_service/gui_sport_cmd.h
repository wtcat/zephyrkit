
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
#include "daily_sport_ctrl.h"

void gui_set_sport_param(Sport_cfg_t *cfg);
void gui_set_body_info(BodyInfo_t *info);
void gui_get_sport_running_data(RunDataStruct_t *data);
