#include "custom_animation.h"
#include "custom_screen_stack.h"
#include "logging/log.h"
#include "windows_manager.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
LOG_MODULE_DECLARE(guix_log);
static GX_SCREEN_STACK_CONTROL screen_stack_control;
static GX_WIDGET *screen_stack_buff[10];

void windows_mgr_init(void)
{
	custom_screen_stack_create(&screen_stack_control, screen_stack_buff, sizeof(screen_stack_buff));
	custom_screen_stack_reset(&screen_stack_control);
}

const static GX_WIDGET *widget_without_anim[] = {
	(GX_WIDGET *)&app_compass_window,
	(GX_WIDGET *)&wf_list,
};

// return true if no need animation
static bool screen_switch_type_judge(GX_WIDGET *next, GX_WIDGET *now)
{
	for (uint8_t i = 0; i < sizeof(widget_without_anim) / sizeof(GX_WIDGET *); i++) {
		if ((next == widget_without_anim[i]) || (now == widget_without_anim[i])) {
			return true;
		}
	}
	return false;
}

void windows_mgr_page_jump(GX_WINDOW *curr, GX_WINDOW *dst, WINDOWS_OP_T op)
{
	if (curr == NULL) {
		curr = (GX_WINDOW *)root_window.gx_widget_first_child;
	}

	if (op == WINDOWS_OP_BACK) {
		INT top;
		GX_WIDGET *screen_dst;
		top = screen_stack_control.gx_screen_stack_control_top;
		if (top >= 0) {
			top <<= 1;
			screen_dst = screen_stack_control.gx_screen_stack_control_memory[top];
			if (screen_switch_type_judge(screen_dst, (GX_WIDGET *)curr)) {
				gx_screen_stack_pop(&screen_stack_control);
			} else {
				custom_screen_stack_pop(&screen_stack_control);
			}
		}
	} else if (op == WINDOWS_OP_SWITCH_NEW) {
		if (screen_switch_type_judge((GX_WIDGET *)dst, (GX_WIDGET *)curr)) {
			gx_screen_stack_push(&screen_stack_control, (GX_WIDGET *)curr, (GX_WIDGET *)dst);
		} else {
			custom_screen_stack_push(&screen_stack_control, (GX_WIDGET *)curr, (GX_WIDGET *)dst);
		}
	} else {
		return;
	}
}

void wm_point_down_record(WM_QUIT_JUDGE_T *info, GX_POINT *pen_down_ponit)
{
	info->pen_down_point = *pen_down_ponit;
	info->pen_down_flag = GX_TRUE;
}
GX_BOOL wm_quit_judge(WM_QUIT_JUDGE_T *info, GX_POINT *pen_up_ponit)
{
	GX_VALUE delta_x, delta_y;
	if (info->pen_down_flag == GX_TRUE) {
		delta_x = (pen_up_ponit->gx_point_x - info->pen_down_point.gx_point_x);
		delta_y = (pen_up_ponit->gx_point_y - info->pen_down_point.gx_point_y);
		info->pen_down_flag = GX_FALSE;
		if ((GX_ABS(delta_y) < WM_DELTA_Y_MAX_FOR_QUIT) && (delta_x >= WM_DELTA_X_MIN_FOR_QUIT)) {
			return GX_TRUE;
		}
	}
	return GX_FALSE;
}