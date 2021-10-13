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

extern GX_WIDGET breath_main_page;
extern GX_WIDGET breath_attention_window;
extern GX_WIDGET breath_inhale_window;
extern GX_WIDGET breath_result_window;
extern GX_WIDGET set_breath_time_window;
extern GX_WIDGET set_breath_freq_window;

void breath_confirm_rectangle_button_draw(GX_WIDGET *widget);
void breath_simple_icon_widget_draw(GX_WIDGET *widget);

void jump_breath_page(GX_WIDGET *next, GX_WIDGET *prev, uint8_t dir);
UINT breath_child_widget_common_event(GX_WIDGET *widget, GX_EVENT *event_ptr);
void app_breath_attention_window_init(GX_WINDOW *window);
void app_breath_inhale_window_init(GX_WINDOW *window);
void app_breath_result_window_init(GX_WINDOW *window, uint8_t hr_data);
void app_breath_window_init(GX_WINDOW *window);
void app_set_breath_freq_window_init(GX_WINDOW *window);
void app_set_breath_time_window_init(GX_WINDOW *window);

typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID icon;
	GX_RESOURCE_ID bg_color;
}breath_icon_Widget_T;

typedef struct {
	uint16_t breath_time;
	uint16_t breath_freq;
}BREATH_SET_T;

extern BREATH_SET_T breath_data;
#endif
