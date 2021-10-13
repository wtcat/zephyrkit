#ifndef __WF_WINDOW_H__
#define __WF_WINDOW_H__
#include "custom_pixelmap_mirror.h"
#include "gx_api.h"
#include "stdbool.h"

typedef struct _wf_mirror_mgr {
	mirror_obj_t *wf_mirror_obj;
	uint8_t wf_use_mirror_or_not; // if 1, use mirror pixelmap to speed up.
} wf_mirror_mgr_t;

extern wf_mirror_mgr_t wf_mirror_mgr;

void wf_window_draw(GX_WINDOW *window);
UINT wf_window_event(GX_WINDOW *window, GX_EVENT *event_ptr);
void wf_window_mirror_set(bool use_mirror_flag);

#endif