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

#include "custom_pixelmap_mirror.h"
#include "drivers/counter.h"
#include "drivers_ext/rtc.h"
#include "init.h"
#include "soc.h"
#include "sys/time_units.h"

#include "alarm_list_ctl.h"
#include "home_window.h"
#include "posix/time.h"
#include "watch_face_manager.h"
#include "wf_list_window.h"
#include "wf_window.h"

#include "app_list_style_grid_view.h"
#include "app_list_window.h"
#include "app_compass_window.h"
#include "app_setting_window.h"

#include "control_center_window.h"
#include <sys/timeutil.h>

#include "app_list_define.h"
#include "custom_pixelmap_mirror.h"
#include "logging/log.h"

#include "custom_animation.h"
#include "guix_language_resources_custom.h"
#include "view_service_custom.h"
#include "setting_service_custom.h"

#include "ux/ux_api.h"
#include "popup/widget.h"
#include "windows_manager.h"

LOG_MODULE_REGISTER(guix_log);

#define SYSTEM_SCREEN_STACK_SIZE 10
#define MIRROR_UPDATE_EVERY_REFRESH 1

GX_WINDOW *p_main_screen;
GX_WINDOW_ROOT *root;
GX_WIDGET *current_screen;
static GX_WIDGET *system_screen_stack[SYSTEM_SCREEN_STACK_SIZE << 1];

#ifdef CONFIG_GUI_SPLIT_BINRES
extern GX_CONST GX_THEME *honbow_disp_theme_table[HONBOW_DISP_THEME_TABLE_SIZE];
// GX_CONST GX_STRING *honbow_disp_language_table[HONBOW_DISP_LANGUAGE_TABLE_SIZE];
extern GX_CONST GX_STRING *honbow_disp_language_table[2];
GX_PIXELMAP **honbow_disp_theme_1_pixelmap_table;
#endif
extern GX_STUDIO_DISPLAY_INFO guix_simple_display_table[];

static UINT shutdown_fire(GX_WIDGET *widget)
{
	(void)widget;
	ux_screen_push_and_switch(current_screen, shutdown_widget_get(), NULL);
	return GX_SUCCESS;
}

static UINT root_window_event(GX_WINDOW *window, GX_EVENT *event_ptr)
{
	return ux_key_timeout_event_wrapper(window, event_ptr, GX_KEY_HOME, UX_MSEC(3000), shutdown_fire,
										gx_window_event_process);
}

UINT guix_main(UINT disp_id, struct guix_driver *drv)
{
	mirror_obj_init();
	view_service_init();
	setting_service_init();
	windows_mgr_init();

#ifdef CONFIG_GUI_SPLIT_BINRES
	GX_THEME *theme;
	// GX_STRING **language;
#endif
	gx_system_initialize(); // Initialize GUIX.
	gx_system_screen_stack_create(system_screen_stack, sizeof(system_screen_stack));
#ifdef CONFIG_GUI_SPLIT_BINRES
	UINT ret = 0;
	ret = guix_binres_load(drv, 0, &theme, NULL);
	if (ret == GX_SUCCESS) {
		honbow_disp_theme_1_pixelmap_table = theme->theme_pixelmap_table;
		// honbow_disp_theme_table[0] = theme;
		((GX_THEME *)honbow_disp_theme_table[0])->theme_color_table = theme->theme_color_table;
		((GX_THEME *)honbow_disp_theme_table[0])->theme_color_table_size = theme->theme_color_table_size;

		((GX_THEME *)honbow_disp_theme_table[0])->theme_pixelmap_table = theme->theme_pixelmap_table;
		((GX_THEME *)honbow_disp_theme_table[0])->theme_pixelmap_table_size = theme->theme_pixelmap_table_size;

		guix_simple_display_table[disp_id].theme_table = honbow_disp_theme_table;
		guix_simple_display_table[disp_id].language_table = (GX_CONST GX_STRING **)honbow_disp_language_table;
		guix_simple_display_table[disp_id].language_table_size = HONBOW_DISP_LANGUAGE_TABLE_SIZE_CUSTOM;
		guix_simple_display_table[disp_id].string_table_size = HONBOW_DISP_STRING_TABLE_SIZE_CUSTOM;
	}
#endif

	void *addr = drv->dev->mmap(~0u);
	gx_studio_display_configure(HONBOW_DISP, drv->setup, 0, HONBOW_DISP_THEME_1, &root);
	drv->dev->unmap(addr);

	gx_studio_named_widget_create("root_window", (GX_WIDGET *)root, (GX_WIDGET **)&p_main_screen);
	current_screen = (GX_WIDGET *)&root_window.root_window_home_window;
	gx_widget_event_process_set((GX_WIDGET *)&root_window.root_window_home_window, home_window_event);
	home_window_init((GX_WIDGET *)&root_window.root_window_home_window);

	gx_widget_draw_set(&root_window.root_window_wf_window, wf_window_draw);
	gx_widget_event_process_set(&root_window.root_window_wf_window, wf_window_event);

	gx_studio_named_widget_create("control_center_window", GX_NULL, GX_NULL);
	control_center_window_init((GX_WIDGET *)&control_center_window);
	gx_widget_event_process_set((GX_WIDGET *)&control_center_window, control_center_window_event);

	gx_studio_named_widget_create("app_compass_window", GX_NULL, GX_NULL);
	gx_widget_draw_set(&app_compass_window, app_compass_window_draw);
	gx_widget_event_process_set(&app_compass_window, app_compass_window_event);

	gx_studio_named_widget_create("app_list_window", GX_NULL, GX_NULL);
	gx_widget_event_process_set(&app_list_window, app_list_window_event);
	gx_widget_draw_set(&app_list_window, app_list_window_draw);
	app_list_style_grid_view_init((GX_WINDOW *)&app_list_window);

	gx_studio_named_widget_create("app_setting_window", GX_NULL, GX_NULL);
	gx_studio_named_widget_create("app_sport_window", GX_NULL, GX_NULL);

	gx_studio_named_widget_create("app_alarm_window", GX_NULL, GX_NULL);
	gx_studio_named_widget_create("app_today_window", GX_NULL, GX_NULL);
	gx_studio_named_widget_create("app_reminders_window", GX_NULL, GX_NULL);
	gx_studio_named_widget_create("app_music_window", GX_NULL, GX_NULL);
	gx_studio_named_widget_create("app_timer_window", GX_NULL, GX_NULL);
	gx_studio_named_widget_create("app_stop_watch_window", GX_NULL, GX_NULL);
	gx_studio_named_widget_create("app_heart_window", GX_NULL, GX_NULL);
	gx_studio_named_widget_create("app_spo2_window", GX_NULL, GX_NULL);
	gx_studio_named_widget_create("app_breath_window", GX_NULL, GX_NULL);

	message_window_create("message_window", NULL, NULL);
	language_window_create("language_window", NULL, NULL);
	pairing_window_create("pairing_window", NULL, NULL);

	shutdown_widget_create(NULL, NULL);
	ringing_widget_create(NULL, NULL);
	ota_widget_create(NULL, NULL);
	lowbat_widget_create(NULL, NULL);
	hrov_widget_create(NULL, NULL);
	charging_widget_create(NULL, NULL);
	sedentary_widget_create(NULL, NULL);
	alarm_remind_widget_create(NULL, NULL);
	shutdown_ani_widget_create(NULL, NULL);
	goal_arrived_widget_create(NULL, NULL);
	sport_widget_create(NULL, NULL);

	wf_mgr_init(drv);
	uint8_t id_default = wf_mgr_get_default_id();
	GX_WIDGET *wf_widget = (GX_WIDGET *)&root_window.root_window_wf_window;
	if (0 != wf_mgr_theme_select(id_default, wf_widget)) {
		wf_mgr_theme_select(WF_THEME_DEFAULT_ID, wf_widget);
	}
	gx_studio_named_widget_create("wf_list", GX_NULL, GX_NULL);
	wf_list_children_create((GX_HORIZONTAL_LIST *)&wf_list);

	app_init_func_process();

	gx_animation_create(&slide_hor_animation);
	gx_animation_landing_speed_set(&slide_hor_animation, 120);
	gx_animation_create(&slide_ver_animation);
	gx_animation_landing_speed_set(&slide_ver_animation, 120);
	custom_animation_init();

	// FOR test
	// ux_screen_push_and_switch(current_screen, (GX_WIDGET *)&pairing_window, NULL);
	// ux_screen_push_and_switch((GX_WIDGET *)&pairing_window, (GX_WIDGET *)&language_window);
	gx_widget_show(root);
	gx_system_start(); // start GUIX thread
	return 0;
}

static GX_WIDGET *screen_list[] = {
	GX_NULL,
	GX_NULL,
	GX_NULL,
};

const static GX_WIDGET *widget_without_anim[] = {
	(GX_WIDGET *)&app_compass_window,
	(GX_WIDGET *)&wf_list,
};

// return true if no need animation
bool screen_switch_type_judge(GX_WIDGET *next, GX_WIDGET *now)
{
	for (uint8_t i = 0; i < sizeof(widget_without_anim) / sizeof(GX_WIDGET *); i++) {
		if ((next == widget_without_anim[i]) || (now == widget_without_anim[i])) {
			return true;
		}
	}
	return false;
}

#if 0
VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen)
{
	if (screen_switch_type_judge(new_screen, current_screen)) {
		gx_widget_detach(current_screen);
		gx_widget_attach(parent, new_screen);
		gx_widget_resize(new_screen, &parent->gx_widget_size);
		current_screen = new_screen;
	} else {
		screen_list[0] = current_screen;
		screen_list[1] = new_screen;
		if (new_screen == (GX_WIDGET *)&root_window.root_window_home_window) {
			if (0 == custom_animation_start(screen_list, parent, SCREEN_SLIDE_RIGHT)) {
				current_screen = new_screen;
			}
		} else {
			if ((new_screen == (GX_WIDGET *)&app_list_window) &&
				(current_screen != (GX_WIDGET *)&root_window.root_window_home_window)) {
				if (0 == custom_animation_start(screen_list, parent, SCREEN_SLIDE_RIGHT)) {
					current_screen = new_screen;
				}
			} else {
				if (0 == custom_animation_start(screen_list, parent, SCREEN_SLIDE_LEFT)) {
					current_screen = new_screen;
				}
			}
		}
	}
}
#endif
