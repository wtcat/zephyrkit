#include "gx_api.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"
extern GX_WINDOW_ROOT *root;

const GX_CHAR *app_timer_get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "计时器";
	case LANGUAGE_ENGLISH:
		return "Timer";
	default:
		return "Timer";
	}
}

const GX_CHAR *app_timer_custom_get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "自定义";
	case LANGUAGE_ENGLISH:
		return "Custom";
	default:
		return "Custom";
	}
}
