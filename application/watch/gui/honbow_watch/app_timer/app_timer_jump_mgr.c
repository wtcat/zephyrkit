#include "gx_api.h"
#include "custom_animation.h"
#include "guix_simple_specifications.h"
#include "app_timer.h"

static GX_WIDGET *screen_list[] = {
	GX_NULL,
	GX_NULL,
	GX_NULL,
};
static GX_WIDGET *record_list[] = {
	GX_NULL,
	GX_NULL,
	GX_NULL,
};

void app_timer_page_reg(APP_TIMER_PAGE_INDEX id, GX_WIDGET *widget)
{
	record_list[id] = widget;
}

void app_timer_page_jump_internal(GX_WIDGET *curr, APP_TIMER_PAGE_JUMP_OP op)
{
	enum slide_direction dir = SCREEN_SLIDE_LEFT;
	screen_list[0] = curr;

	if (op == APP_TIMER_PAGE_JUMP_BACK) {
		dir = SCREEN_SLIDE_RIGHT;
		screen_list[1] = record_list[APP_TIMER_PAGE_SELECT];
	} else if (op == APP_TIMER_PAGE_JUMP_MAIN) {
		screen_list[1] = record_list[APP_TIMER_PAGE_MAIN];
	} else {
		screen_list[1] = record_list[APP_TIMER_PAGE_CUSTOM];
	}

	custom_animation_start(screen_list, (GX_WIDGET *)&app_timer_window, dir);
}

void app_timer_page_reset(void)
{
	GX_WIDGET *parent = (GX_WIDGET *)&app_timer_window;
	GX_WIDGET *child = parent->gx_widget_first_child;

	while (child) {
		gx_widget_detach(child);
		child = parent->gx_widget_first_child;
	}
	gx_widget_resize(record_list[0], &parent->gx_widget_size);
	gx_widget_attach(parent, record_list[0]);
}
