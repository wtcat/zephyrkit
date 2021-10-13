#include "app_alarm_window.h"
#include "app_list_define.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "logging/log.h"
#include "app_alarm_page_mgr.h"
#include "windows_manager.h"

LOG_MODULE_DECLARE(guix_log);

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

extern void app_alarm_main_page_init(GX_WINDOW *window);
extern void alarm_time_sel_page_init(void);
extern void app_alarm_rpt_sel_page_init(void);
extern void alarm_vib_duration_sel_page_init(void);
extern void app_alarm_edit_page_init(void);

void app_alarm_window_init(GX_WINDOW *window)
{
	LOG_INF("alarm window init");
	gx_widget_event_process_set((GX_WIDGET *)window, widget_event_processing_function);

	app_alarm_page_mgr_init();

	app_alarm_main_page_init(window); // reg ALARM_PAGE_MAIN
	alarm_time_sel_page_init();		  // reg ALARM_PAGE_TIME_SEL
	app_alarm_rpt_sel_page_init();
	alarm_vib_duration_sel_page_init();
	app_alarm_edit_page_init();
}

GUI_APP_DEFINE_WITH_INIT(alarm, APP_ID_07_ALARM, (GX_WIDGET *)&app_alarm_window,
						 GX_PIXELMAP_ID_APP_LIST_ICON_ALARM_90_90, GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY,
						 app_alarm_window_init);
