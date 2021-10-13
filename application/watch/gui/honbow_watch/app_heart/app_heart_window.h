#ifndef __APP_HEART_WINDOW_H__
#define __APP_HEART_WINDOW_H__
#include "gx_api.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "app_list_define.h"
#include "drivers_ext/sensor_priv.h"
#include "custom_window_title.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include "string.h"
#include "stdio.h"
#include "custom_rounded_button.h"
#include "app_outdoor_run_window.h"
#include "guix_language_resources_custom.h"
#include "guix_api.h"
// #include "hr_srv.h"
#include "sensor/gh3011/HeartRate_srv/hr_srv.h"

extern GX_WIDGET heart_main_page;
extern GX_WIDGET please_wear_window;

typedef struct{
	GX_WIDGET widget;
	GX_RESOURCE_ID icon;
	GX_PROMPT  decs1;	
}HEART_DETECTION_WIGET_T;

typedef struct {
	GX_WIDGET hr_widget;
	GX_RESOURCE_ID icon;
}MaxMinHr_Widget_T;

void get_24hour_hr_data(AllDaysHr_T *data);
void app_heart_summary_list_prompt_get(INT index, GX_STRING *prompt);
void init_test_data_window(GX_WIDGET *parent);
void init_hr_data_window(GX_WIDGET *parent);
void heart_icon_widget_draw(GX_WIDGET *widget);
void jump_heart_page(GX_WIDGET *next, GX_WIDGET *prev, uint8_t dir);
#endif