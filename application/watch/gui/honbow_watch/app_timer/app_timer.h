#ifndef __APP_TIMER_H__
#define __APP_TIMER_H__
#include "gx_api.h"
#include "custom_animation.h"

typedef struct {
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t tenth_of_second;
} TIMER_PARA_TRANS_T;

typedef enum {
	APP_TIMER_PAGE_SELECT = 0,
	APP_TIMER_PAGE_MAIN,
	APP_TIMER_PAGE_CUSTOM,
} APP_TIMER_PAGE_INDEX;

typedef enum {
	APP_TIMER_PAGE_JUMP_BACK = 0,
	APP_TIMER_PAGE_JUMP_MAIN,
	APP_TIMER_PAGE_JUMP_CUSTOM,
} APP_TIMER_PAGE_JUMP_OP;

void app_timer_main_page_show(GX_BOOL ext_flag, GX_WINDOW *curr, GX_WIDGET *child_page, TIMER_PARA_TRANS_T para);
// void app_timer_page_jump_internal(GX_WIDGET *curr, GX_WIDGET *dst, enum slide_direction dir);
void app_timer_page_jump_internal(GX_WIDGET *curr, APP_TIMER_PAGE_JUMP_OP op);
void app_timer_page_reg(APP_TIMER_PAGE_INDEX id, GX_WIDGET *widget);
void app_timer_page_reset(void);
void app_timer_custom_add(uint8_t hour, uint8_t min, uint8_t sec);
#endif