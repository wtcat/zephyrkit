#ifndef __APP_SPORT_WINDOW_H__
#define __APP_SPORT_WINDOW_H__
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
#include "sport_goal_set_window.h"
#include "app_today_window.h"
//#include "gui_sport_cmd.h"
//#include "cywee_motion.h"
//#include "sport_ctrl.h"
extern Sport_cfg_t gui_sport_cfg;
VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);

extern uint32_t sport_time_count;


typedef enum {
	OUTDOOR_RUN_CHILD_PAGE = 0,
	OUTDOOR_WALK_CHILD_PAGE,
	OUTDOOR_BICYCLE_CHILD_PAGE,
	INDOOR_RUN_CHILD_PAGE,
	INDOOR_WALK_CHILD_PAGE,
	ON_FOOT_CHILD_PAGE,
	INDOOR_BICYCLE_CHILD_PAGE,
    ELLIPTICAL_MACHINE_CHILD_PAGE,
    ROWING_MACHINE_CHILD_PAGE,
    YOGA_CHILD_PAGE,
    CLIMBING_CHILD_PAGE,
    POOL_SWIMMING_CHILD_PAGE,
    OPEN_SWIMMING_CHILD_PAGE,
    OTHER_CHILD_PAGE,
} SPORT_CHILD_PAGE_T;

void clear_sport_widow(void);

void clear_gps_connect_status(void);
void set_gps_connetting_titlle_prompt(INT index);

void app_outdoor_run_window_init(GX_WINDOW *window, SPORT_CHILD_PAGE_T sport_type);
void app_gps_connecting_window_init(GX_WINDOW *window, SPORT_CHILD_PAGE_T sport_type);
void app_sport_start_window_init(GX_WINDOW *window);
void app_count_down_window_init(GX_WINDOW *window);
void app_sport_data_window_init(GX_WINDOW *window);
void app_sport_paused_window_init(GX_WINDOW *window);
void app_sport_summary_window_init(GX_WINDOW *window, GX_BOOL button_flag);
void app_sport_time_short_windown_init(GX_WINDOW *window);
void app_sport_goal_set_window_init(GX_WINDOW *window, SPORT_CHILD_PAGE_T sport_type, GOAL_SETTING_TYPE_T goal_type);
void app_gps_confirm_window_init(GX_WINDOW *window);
void app_gps_not_connectd_windown_init(GX_WINDOW *window);

extern APP_SPORT_WINDOW_CONTROL_BLOCK app_sport_window;

extern GX_WIDGET sport_paused_window;
extern GX_WIDGET gps_confirm_window;
extern GX_WIDGET gps_not_connectd_window;
extern GX_WIDGET sport_start_window;
extern GX_WIDGET sport_paused_window;
extern GX_WIDGET sport_summary_window;
extern GX_WIDGET sport_main_page;
extern GX_WIDGET sport_time_short_window;
extern GX_WIDGET sport_data_window;
extern GX_WIDGET count_down_window;
extern GX_WIDGET sport_goal_set_window;
extern GX_WIDGET outdoor_run_window;
extern GX_WIDGET gps_connecting_window;




void sport_main_page_jump_reg(GX_WIDGET *dst, SPORT_CHILD_PAGE_T id);
void jump_sport_main_page(GX_WIDGET *now);
void jump_sport_page(GX_WIDGET *next, GX_WIDGET *prev, uint8_t dir);
void app_sport_list_prompt_get(INT index, GX_STRING *prompt);



#endif