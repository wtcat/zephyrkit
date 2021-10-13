#include "app_setting_window.h"
#include "custom_window_title.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include "string.h"
#include "stdio.h"
#include "app_list_define.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "custom_rounded_button.h"
#include "windows_manager.h"
extern VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
static UINT widget_event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	GX_WINDOW *window = (GX_WINDOW *)widget;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_KEY_DOWN:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			return 0;
		}
		break;
	case GX_EVENT_KEY_UP:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&app_list_window);
			windows_mgr_page_jump(window, NULL, WINDOWS_OP_BACK);
			return 0;
		}
		break;
	default:
		break;
	}
	return gx_window_event_process(window, event_ptr);
}

extern void app_setting_main_page_init(GX_WINDOW *window);
extern void app_setting_disp_page_init(GX_WINDOW *window);
extern void app_setting_list_style_page_init(GX_WINDOW *window);
extern void app_setting_dnd_mode_page_init(GX_WINDOW *window);
extern void app_setting_activity_detect_page_init(GX_WINDOW *window);
extern void app_setting_Vibration_strength_page_init(GX_WINDOW *window);
extern void app_setting_music_control_page_init(GX_WINDOW *window);
extern void app_setting_system_page_init(GX_WINDOW *window);
void app_setting_window_init(GX_WINDOW *parent)
{
	app_setting_main_page_init(parent);
	app_setting_disp_page_init(NULL); // disp setting main page
	app_setting_list_style_page_init(NULL);
	app_setting_dnd_mode_page_init(NULL);
	app_setting_Vibration_strength_page_init(NULL);
	app_setting_activity_detect_page_init(NULL);
	app_setting_music_control_page_init(NULL);
	app_setting_system_page_init(NULL);

	gx_widget_event_process_set((GX_WIDGET *)parent, widget_event_processing_function);
}

GUI_APP_DEFINE_WITH_INIT(setting, APP_ID_12_SETTING, (GX_WIDGET *)&app_setting_window,
						 GX_PIXELMAP_ID_APP_LIST_ICON_SETTING_90_90, GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY,
						 app_setting_window_init);
