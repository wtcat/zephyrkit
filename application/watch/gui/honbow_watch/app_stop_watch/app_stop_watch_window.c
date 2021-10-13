#include "gx_api.h"
#include "app_list_define.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "logging/log.h"
#include "windows_manager.h"
LOG_MODULE_DECLARE(guix_log);

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
			windows_mgr_page_jump((GX_WINDOW *)&app_timer_window, NULL, WINDOWS_OP_BACK);
			return 0;
		}
		break;
	default:
		break;
	}
	return gx_window_event_process(window, event_ptr);
}

void app_stopwatch_main_page_init(GX_WINDOW *window);
static void app_stop_watch_window_init(GX_WINDOW *window)
{
	LOG_INF("stop watch window init");
	gx_widget_event_process_set((GX_WIDGET *)window, widget_event_processing_function);
	app_stopwatch_main_page_init(window);
}

GUI_APP_DEFINE_WITH_INIT(stopwatch, APP_ID_09_STOP_WATCH, (GX_WIDGET *)&app_stop_watch_window,
						 GX_PIXELMAP_ID_APP_LIST_ICON_STOPWATCH_90_90, GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY,
						 app_stop_watch_window_init);
