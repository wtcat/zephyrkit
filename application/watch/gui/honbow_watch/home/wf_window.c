#include "wf_window.h"
#include "custom_pixelmap_mirror.h"
#include "drivers/counter.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "home_window.h"
#include "posix/sys/time.h"
#include "sys/timeutil.h"
#include "time.h"
#include "watch_face_manager.h"
#include "watch_face_port.h"
#include "watch_face_theme.h"
#include <posix/time.h>
wf_mirror_mgr_t wf_mirror_mgr;

void wf_window_draw(GX_WINDOW *widget)
{
	if ((!wf_mirror_mgr.wf_use_mirror_or_not) || (wf_mirror_mgr.wf_mirror_obj == NULL)) {
		wf_theme_show(widget);
		if ((widget->gx_widget_size.gx_rectangle_left == 0) && (widget->gx_widget_size.gx_rectangle_top == 0) &&
			(wf_mirror_mgr.wf_mirror_obj != NULL)) {
			GX_CANVAS *canvas_tmp;
			gx_widget_canvas_get((GX_WIDGET *)widget, &canvas_tmp);
			mirror_obj_copy(&wf_mirror_mgr.wf_mirror_obj->canvas, canvas_tmp, 0);
		}
	} else {
		GX_PIXELMAP *map_tmp = &wf_mirror_mgr.wf_mirror_obj->map;
		gx_canvas_pixelmap_draw(widget->gx_widget_size.gx_rectangle_left, widget->gx_widget_size.gx_rectangle_top,
								map_tmp);
		if (!home_window_is_pen_down() && !home_window_drag_anim_busy())
			wf_mirror_mgr.wf_use_mirror_or_not = 0;
	}
}

#define CLOCK_TIMER 0x01
/*************************************************************************************/
UINT wf_window_event(GX_WINDOW *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		gx_system_timer_start(window, CLOCK_TIMER, 50, 50);
		break;
	case GX_EVENT_HIDE:
		gx_system_timer_stop(window, CLOCK_TIMER);
		break;
	case GX_EVENT_TIMER:
		if (event_ptr->gx_event_payload.gx_event_timer_id == CLOCK_TIMER) {
			if (home_window_is_pen_down()) {
				return 0;
			}
			static uint8_t last_sec = 0xff;
			wf_port_time_update();
			uint8_t sec_now = 0;
			wf_port_get_value func = wf_get_data_func(ELEMENT_TYPE_SEC);
			if (func != NULL) {
				func(&sec_now);
				if (last_sec != sec_now) {
					last_sec = sec_now;
					uint8_t cnts = wft_element_cnts_get();
					element_style_t *style = wft_element_styles_get();
					for (uint8_t i = 0; i < cnts; i++) {
						if (true == wft_element_refresh_judge(&style[i])) {
							gx_system_dirty_mark(style->e_widget);
						}
					}
				}
			}
			return 0;
		}
		break;
	}
	return gx_window_event_process(window, event_ptr);
}

void wf_window_mirror_set(bool use_mirror_flag)
{
	if (use_mirror_flag) {
		if ((!wf_mirror_mgr.wf_use_mirror_or_not) && (wf_mirror_mgr.wf_mirror_obj != NULL)) {
			wf_mirror_mgr.wf_use_mirror_or_not = 1;
		}
	} else {
		wf_mirror_mgr.wf_use_mirror_or_not = 0;
	}
}
