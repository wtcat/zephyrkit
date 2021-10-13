#include "app_list_window.h"
#include "app_list_style_grid_view.h"
#include "guix_simple_specifications.h"
#include "windows_manager.h"
extern VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
void app_list_window_draw(GX_WINDOW *widget)
{
	gx_window_draw(widget);
}
UINT app_list_window_event(GX_WINDOW *window, GX_EVENT *event_ptr)
{
	static WM_QUIT_JUDGE_T pen_info_judge;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		break;
	}
	case GX_EVENT_HIDE: {
		break;
	}
	case GX_EVENT_PEN_DOWN:
		wm_point_down_record(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata);
		break;
	case GX_EVENT_PEN_UP:
		if (wm_quit_judge(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata) == GX_TRUE) {
			windows_mgr_page_jump((GX_WINDOW *)&app_list_window, NULL, WINDOWS_OP_BACK);
		}
		break;
	case GX_EVENT_KEY_DOWN:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			return 0;
		}
		break;
	case GX_EVENT_KEY_UP:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			windows_mgr_page_jump((GX_WINDOW *)&app_list_window, NULL, WINDOWS_OP_BACK);
			return 0;
		}
		break;
	}
	return gx_window_event_process(window, event_ptr);
}