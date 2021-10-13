#include "gx_api.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"

extern GX_WINDOW_ROOT *root;

const GX_CHAR *app_stop_watch_get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "秒表";
	case LANGUAGE_ENGLISH:
		return "stop-watch";
	default:
		return "stop-watch";
	}
}