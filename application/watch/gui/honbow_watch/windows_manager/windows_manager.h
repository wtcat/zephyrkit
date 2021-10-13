#ifndef __WINDOWS_MANAGER_H__
#define __WINDOWS_MANAGER_H__
#include "gx_api.h"

typedef enum {
	WINDOWS_OP_SWITCH_NEW = 0,
	WINDOWS_OP_BACK,
} WINDOWS_OP_T;

#define WM_DELTA_X_MIN_FOR_QUIT 150
#define WM_DELTA_Y_MAX_FOR_QUIT 50

typedef struct {
	GX_POINT pen_down_point;
	GX_BOOL pen_down_flag;
} WM_QUIT_JUDGE_T;

void windows_mgr_init(void);
void windows_mgr_page_jump(GX_WINDOW *curr, GX_WINDOW *dst, WINDOWS_OP_T op);

void wm_point_down_record(WM_QUIT_JUDGE_T *info, GX_POINT *pen_down_ponit);

GX_BOOL wm_quit_judge(WM_QUIT_JUDGE_T *info, GX_POINT *pen_up_ponit);
#endif