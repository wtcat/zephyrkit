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
#include "sensor/gh3011/HeartRate_srv/hr_srv.h"

extern GX_WIDGET spo2_measuring_window;
extern GX_WIDGET spo2_main_page;
extern GX_WIDGET spo2_result_window;
extern GX_WIDGET measuring_fail_window;

void spo2_child_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr);
void jump_spo2_page(GX_WIDGET *next, GX_WIDGET *prev, uint8_t dir);
void app_spo2_measuring_window_init(GX_WINDOW *window);
void app_spo2_result_window_init(GX_WINDOW *window, uint8_t spo2_data);
void app_spo2_measuring_failed_window_init(GX_WINDOW *window);

typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID icon;
	GX_RESOURCE_ID bg_color;
}icon_Widget_T;

// typedef struct APP_HEART_ROW_STRUCT {
// 	GX_WIDGET widget;	
// 	UCHAR id;
// } APP_HEART_LIST_ROW_T;

// typedef struct {
// 	uint8_t max_hr;
// 	uint8_t min_hr;
// }HOUR_HR_T;

// typedef struct {
// 	HOUR_HR_T hour_hr[24];
// }AllDaysHr_T;

// typedef struct {
// 	GX_WIDGET hr_widget;
// 	GX_RESOURCE_ID icon;
// }MaxMinHr_Widget_T;

// void get_24hour_hr_data(AllDaysHr_T *data);
#endif