/* This is a small demo of the high-performance GUIX graphics framework. */

#include "device.h"
#include "gx_api.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/_timespec.h>
#include <time.h>

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "gx_widget.h"

#include "init.h"
#include "pixelmap_mirror.h"
#include "sys/time_units.h"
#include "watch_face_manager.h"

#include "drivers/counter.h"
#include "drivers_ext/rtc.h"
#include "soc.h"

#include "alarm_list_ctl.h"
#include "posix/time.h"

#define MIRROR_UPDATE_EVERY_REFRESH 1

GX_WINDOW *p_main_screen;
GX_WINDOW_ROOT *root;

typedef struct _wf_mirror_mgr {
	mirror_obj_t *wf_mirror_obj;
	uint8_t wf_use_mirror_or_not; // if 1, use mirror pixelmap to speed up.
} wf_mirror_mgr_t;

wf_mirror_mgr_t wf_mirror_mgr;

/* Define menu screen list. */
GX_WIDGET *menu_screen_list[] = {
	(GX_WIDGET *)&root_window.root_window_watch_face_window,
	(GX_WIDGET *)&window, (GX_WIDGET *)&alarm_window, GX_NULL};

#define ANIMATION_ID_MENU_SLIDE 1

GX_ANIMATION slide_animation;

VOID start_slide_animation(GX_WINDOW *window)
{
	GX_ANIMATION_INFO slide_animation_info;

	memset(&slide_animation_info, 0, sizeof(GX_ANIMATION_INFO));
	slide_animation_info.gx_animation_parent = (GX_WIDGET *)window;
	slide_animation_info.gx_animation_style =
		GX_ANIMATION_SCREEN_DRAG | GX_ANIMATION_HORIZONTAL | GX_ANIMATION_WRAP;
	slide_animation_info.gx_animation_id = ANIMATION_ID_MENU_SLIDE;
	slide_animation_info.gx_animation_frame_interval = 1;
	slide_animation_info.gx_animation_slide_screen_list = menu_screen_list;
	gx_animation_drag_enable(&slide_animation, (GX_WIDGET *)window,
							 &slide_animation_info);
}

/* Define menu screen list. */
GX_WIDGET *status_menu_screen_list[] = {
	(GX_WIDGET *)&root_window.root_window_main_window,
	(GX_WIDGET *)&status_window, GX_NULL};

#define ANIMATION_ID_MENU_STATUS_MENU 2

GX_ANIMATION status_menu_animation;

VOID start_status_menu_animation(GX_WINDOW *window)
{
	GX_ANIMATION_INFO slide_animation_info;

	memset(&slide_animation_info, 0, sizeof(GX_ANIMATION_INFO));
	slide_animation_info.gx_animation_parent = (GX_WIDGET *)window;
	slide_animation_info.gx_animation_style =
		GX_ANIMATION_SCREEN_DRAG | GX_ANIMATION_VERTICAL | GX_ANIMATION_WRAP;
	slide_animation_info.gx_animation_id = ANIMATION_ID_MENU_STATUS_MENU;
	slide_animation_info.gx_animation_frame_interval = 1;
	slide_animation_info.gx_animation_slide_screen_list =
		status_menu_screen_list;

	// new property for window shift limit.
	slide_animation_info.gx_animation_range_limit_type =
		GX_ANIMATION_LIMIT_STYLE_VERTICAL_DOWN;

	gx_animation_drag_enable(&status_menu_animation, (GX_WIDGET *)window,
							 &slide_animation_info);
}

VOID tmp_disp_draw(GX_WINDOW *widget)
{
#if 0
    gx_canvas_pixelmap_draw(widget->gx_widget_size.gx_rectangle_left,
                            widget->gx_widget_size.gx_rectangle_top,
                                &wf_mirror_mgr.wf_mirror_obj->map);
#else
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);
	GX_COLOR brush_color;
	gx_context_color_get(GX_COLOR_ID_BLUE, &brush_color);
	gx_brush_define(&current_context->gx_draw_context_brush, brush_color, 0,
					GX_BRUSH_ROUND);
	gx_context_brush_width_set(8);

	for (uint8_t i = 0; i < 3; i++) {
		int32_t x_center = widget->gx_widget_size.gx_rectangle_left + 180;
		int32_t y_center = widget->gx_widget_size.gx_rectangle_top + 180;

		uint32_t r = 160 - i * 15;

		int32_t end_angle = 90;
		int32_t start_angle = 180 + i * 30;

		gx_canvas_arc_draw(x_center, y_center, r, start_angle, end_angle);
	}
#endif
}

#include <sys/timeutil.h>

VOID alarm_list_children_create(GX_VERTICAL_LIST *list);

UINT guix_main(UINT disp_id, struct guix_driver *drv)
{
	// get ticks from rtc counter
	const struct device *rtc = device_get_binding("apollo3p_rtc");
	int ticks;

#if 1 // demo for time setting to rtc
	if (rtc) {
		struct tm tm_now;
		tm_now.tm_year = 121; // year since 1900
		tm_now.tm_mon = 0;
		tm_now.tm_mday = 26;
		tm_now.tm_hour = 11;
		tm_now.tm_min = 0;
		tm_now.tm_sec = 0;
		time_t ts = timeutil_timegm(&tm_now);

		ticks = counter_us_to_ticks(rtc, ts * USEC_PER_SEC);
		counter_set_value(rtc, ticks);
	}
#endif

	// sync rtc ticks to system time
	if (rtc) {
		counter_get_value(rtc, &ticks);
		uint64_t us_cnt = counter_ticks_to_us(rtc, ticks);

		// convert tick to timespec,then set to posix real time
		struct timespec time;
		time.tv_sec = us_cnt / 1000000UL;
		time.tv_nsec = 0;
		clock_settime(CLOCK_REALTIME, &time);
	}

	gx_system_initialize(); // Initialize GUIX.

	gx_studio_display_configure(MAIN_DISPLAY, drv->setup, 0,
								MAIN_DISPLAY_THEME_1, &root);

	alarm_list_ctl_init();

	gx_studio_named_widget_create(
		"root_window", (GX_WIDGET *)root,
		(GX_WIDGET **)&p_main_screen); // create empty watch face window.

	wf_mgr_init(drv);

	uint8_t id_default = wf_mgr_get_default_id();
	if (0 != wf_mgr_theme_select(
				 id_default,
				 (GX_WIDGET *)&root_window.root_window_watch_face_window)) {
		wf_mgr_theme_select(WF_THEME_DEFAULT_ID,
							(GX_WIDGET *)&root_window
								.root_window_watch_face_window); // set default
	}

	gx_studio_named_widget_create("window", GX_NULL, NULL);

	gx_studio_named_widget_create("status_window", GX_NULL, NULL);

	gx_studio_named_widget_create("alarm_window", GX_NULL, NULL);
	alarm_list_children_create(&alarm_window.alarm_window_alarm_list);
	extern void ui_alarm_callback(void *data);
	alarm_list_ctl_reg_callback(ui_alarm_callback);

	gx_widget_draw_set((GX_WIDGET *)&window, tmp_disp_draw);

	gx_animation_create(&slide_animation);

	gx_animation_landing_speed_set(&slide_animation, 60);

	gx_animation_create(&status_menu_animation);

	gx_animation_landing_speed_set(&status_menu_animation, 60);

	gx_widget_show(
		root); // Show the root window to make it and patients screen visible.

	gx_system_start(); // start GUIX thread

	return 0;
}

#define CLOCK_TIMER 0x01
volatile static uint8_t root_window_enter_pen_down = 0;
#include "drivers/counter.h"
#include "posix/sys/time.h"
#include "sys/timeutil.h"
#include "time.h"
#include <posix/time.h>
/*************************************************************************************/
UINT main_disp_handler(GX_WINDOW *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		gx_system_timer_start(&root_window.root_window_watch_face_window,
							  CLOCK_TIMER, 50, 50);

		break;
	case GX_EVENT_HIDE:
		gx_system_timer_stop(&root_window.root_window_watch_face_window,
							 CLOCK_TIMER);
		break;
	case GX_EVENT_TIMER:
		if (event_ptr->gx_event_payload.gx_event_timer_id == CLOCK_TIMER) {
			if (slide_animation.gx_animation_status == GX_ANIMATION_IDLE) {
#if 0
				static time_t last_sec = 0xff;
				struct timeval tv;
				gettimeofday(&tv, NULL);
				if (last_sec == tv.tv_sec) {
					break;
				}
				last_sec = tv.tv_sec;
#else
				static uint8_t last_sec = 0xff;
				posix_time_update();
				uint8_t sec_now = watch_face_get_sec();
				if (last_sec == sec_now) {
					break;
				}
				last_sec = sec_now;
#endif
#if 0
				gx_system_dirty_mark(window);
#else
				uint8_t cnts = wf_theme_element_cnts_get();
				element_style_t *style = wf_theme_element_styles_get();
				for (uint8_t i = 0; i < cnts; i++) {
					if (wf_theme_element_refresh_adjust(&style[i])) {
						gx_system_dirty_mark(&style->e_widget);
					}
				}
#endif
			}
		}
		break;
	}
	return gx_window_event_process(window, event_ptr);
}

uint8_t tmp_test = 0;

// called after gx_system_dirty_mark(&watch_face_window)
VOID main_disp_draw(GX_WINDOW *widget)
{
	if ((!wf_mirror_mgr.wf_use_mirror_or_not) ||
		(wf_mirror_mgr.wf_mirror_obj == NULL)) {
		wf_theme_show(widget);

#if MIRROR_UPDATE_EVERY_REFRESH // testing: copy data at once when screen
								// refresh.
		GX_CANVAS *canvas_tmp;
		gx_widget_canvas_get((GX_WIDGET *)widget, &canvas_tmp);
		mirror_obj_copy(&wf_mirror_mgr.wf_mirror_obj->canvas, canvas_tmp, 0);
#endif
	} else {
		GX_PIXELMAP map_tmp = wf_mirror_mgr.wf_mirror_obj->map;

		gx_canvas_pixelmap_draw(widget->gx_widget_size.gx_rectangle_left,
								widget->gx_widget_size.gx_rectangle_top,
								&map_tmp);
#if 0
        if ((widget->gx_widget_size.gx_rectangle_left == 0) &&
                (widget->gx_widget_size.gx_rectangle_top == 0)) {
            if (root_window_enter_pen_down == 0){
                wf_mirror_mgr.wf_use_mirror_or_not = 0;
            }
        }
#else
		if ((slide_animation.gx_animation_status == GX_ANIMATION_IDLE) &&
			(status_menu_animation.gx_animation_status == GX_ANIMATION_IDLE)) {
			wf_mirror_mgr.wf_use_mirror_or_not = 0;
		}
#endif
	}
}

static GX_VALUE last_x = 0;
static GX_VALUE last_y = 0;
static volatile uint8_t enter_h = 0;
static volatile uint8_t enter_v = 0;

GX_VALUE delta_y;
GX_VALUE delta_x;

UINT root_event(GX_WINDOW *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		GX_CANVAS *canvas_tmp;
		gx_widget_canvas_get((GX_WIDGET *)window, &canvas_tmp);
		mirror_obj_create_copy(&wf_mirror_mgr.wf_mirror_obj, canvas_tmp);
	} break;
	case GX_EVENT_HIDE:
		mirror_obj_delete(
			&wf_mirror_mgr.wf_mirror_obj); //  reset obj's canvas data
		break;
#if 1
	case GX_EVENT_PEN_DOWN:
	case GX_EVENT_PEN_DRAG:
#if MIRROR_UPDATE_EVERY_REFRESH
		if ((!wf_mirror_mgr.wf_use_mirror_or_not) &&
			(wf_mirror_mgr.wf_mirror_obj != NULL)) {
			wf_mirror_mgr.wf_use_mirror_or_not = 1;
		}
#else
		if (root_window.root_window_watch_face_window.gx_widget_status &
			GX_STATUS_VISIBLE) {
			if ((!wf_mirror_mgr.wf_use_mirror_or_not) &&
				(wf_mirror_mgr.wf_mirror_obj != NULL) &&
				(status_menu_animation.gx_animation_status ==
				 GX_ANIMATION_IDLE) &&
				(slide_animation.gx_animation_status == GX_ANIMATION_IDLE)) {
				wf_mirror_mgr.wf_use_mirror_or_not = 1;
				GX_CANVAS *canvas_tmp;
				gx_widget_canvas_get((GX_WIDGET *)window, &canvas_tmp);
				mirror_obj_copy(&wf_mirror_mgr.wf_mirror_obj->canvas,
								canvas_tmp, 0);
			}
		}
#endif

		if ((root_window_enter_pen_down) && (!enter_h) && (!enter_v)) {
			delta_y = GX_ABS(
				event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y -
				last_y);
			delta_x = GX_ABS(
				event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x -
				last_x);
			if ((delta_x > 5) || (delta_y > 5)) {
				GX_WINDOW *parent_window = NULL;
				if (delta_y > delta_x) {
					parent_window = window;
					start_status_menu_animation(parent_window);
					enter_h = 1;
				} else {
					parent_window = &root_window.root_window_main_window;
					start_slide_animation(parent_window);
					enter_v = 1;
				}
				// send pen down event to animation's parent window.
				GX_EVENT tmp = *event_ptr;
				tmp.gx_event_payload.gx_event_pointdata.gx_point_x = last_x;
				tmp.gx_event_payload.gx_event_pointdata.gx_point_y = last_y;
				tmp.gx_event_type = GX_EVENT_PEN_DOWN;
				tmp.gx_event_target = (struct GX_WIDGET_STRUCT *)parent_window;
				gx_system_event_send(&tmp);
			}
		}

		if (!root_window_enter_pen_down) {
			root_window_enter_pen_down = 1;
		}

		last_y = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y;
		last_x = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;

		break;

	case GX_EVENT_PEN_UP:
		root_window_enter_pen_down = 0;
	case GX_EVENT_ANIMATION_COMPLETE:
#if 0
        if ((root_window.root_window_watch_face_window.gx_widget_status & GX_STATUS_VISIBLE) &&
            (status_menu_animation.gx_animation_status == GX_ANIMATION_IDLE) &&
            (slide_animation.gx_animation_status == GX_ANIMATION_IDLE)) {
#else
		if ((status_menu_animation.gx_animation_status == GX_ANIMATION_IDLE) &&
			(slide_animation.gx_animation_status == GX_ANIMATION_IDLE)) {
#endif
		if (enter_h) {
			gx_animation_drag_disable(&status_menu_animation,
									  (GX_WIDGET *)window);
			enter_h = 0;
		}
		if (enter_v) {
			gx_animation_drag_disable(
				&slide_animation,
				(GX_WIDGET *)&root_window.root_window_main_window);
			enter_v = 0;
		}
	}
	break;
#endif
}
return gx_window_event_process(window, event_ptr);
}

VOID status_window_draw(GX_WINDOW *widget)
{
	gx_widget_draw(widget);
}

VOID callback_name(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT int32)
{
	return;
}