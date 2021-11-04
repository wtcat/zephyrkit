#ifndef __APP_TODAY_WINDOW_H__
#define __APP_TODAY_WINDOW_H__
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

#include "windows_manager.h"

extern GX_WIDGET sleep_data_widget;
extern GX_WIDGET today_main_page;
extern GX_WIDGET no_sport_record_window;

void today_child_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr);
// void app_sleep_data_window_init(GX_WINDOW *window, SLEEP_DATA_STRUCT_DEF *sleep_data);
// void init_sleep_data_test(int index, SLEEP_DATA_STRUCT_DEF *data);
void app_no_sport_record_window_init(GX_WINDOW *window);

typedef struct {
    UINT start_time;
    UINT end_time;
}SLEEP_DATA_T;

typedef struct{
    UINT sleep_lenth;
    SLEEP_DATA_T sleep_data;
}SLEEP_ROW_DATA_T;

typedef struct {
    UINT start_time;
    UINT enn_time;
    UINT sport_type;
}SPORT_DATA_T;


typedef struct SLEEP_NO_RECORD_WIDGET_STRUCT{
	GX_WIDGET widget;
	GX_WIDGET child;
	GX_RESOURCE_ID icon;
	GX_PROMPT decs;	
}SLEEP_NO_RECORD_WIDGET_T;

typedef enum {
	OUTDOOR_RUN = 0,
	OUTDOOR_WALK,
	OUTDOOR_BICYCLE,
	INDOOR_RUN,
	INDOOR_WALK,
	ON_FOOT,
	INDOOR_BICYCLE,
    ELLIPTICAL_MACHINE,
    ROWING_MACHINE,
    YOGA,
    CLIMBING,
    POOL_SWIMMING,
    OPEN_SWIMMING,
    OTHER,
} SPORT_TYPE_T;

typedef struct {
    UINT sport_lenth;
    SPORT_TYPE_T type;
    SPORT_DATA_T sport_data;
}sport_ROW_T;

typedef struct {
    UINT steps;
    UINT goal_steps;
    UINT active_hours;
    UINT goal_active_hours;
    UINT exc_min;
    UINT goal_exc_min;
    UINT distance;
    UINT calorise;
    SLEEP_ROW_DATA_T sleep[3];
    sport_ROW_T sport[10];
}TODAY_OVERVIEWS_Typdef;

void app_today_main_page_init(GX_WINDOW *window);
void jump_today_page(GX_WIDGET *next, GX_WIDGET *prev, uint8_t dir);
#endif