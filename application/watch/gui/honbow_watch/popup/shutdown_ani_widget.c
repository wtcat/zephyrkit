#include "ux/ux_api.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"


struct shutdown_ani_widget {
	GX_WIDGET widget;
	GX_RECTANGLE client;
	GX_RESOURCE_ID icon;
	GX_VALUE angle;
};

static struct shutdown_ani_widget popup_widget;

static void shutdown_ani_widget_draw(struct shutdown_ani_widget *w)
{
	GX_PIXELMAP *pixelmap;

	_gx_widget_background_draw(&w->widget);
	gx_context_pixelmap_get(w->icon, &pixelmap);
	if (pixelmap) {
		gx_canvas_pixelmap_rotate(w->client.gx_rectangle_left, w->client.gx_rectangle_top, 
			pixelmap, w->angle, -1, -1);
	}
}

static UINT shutdown_ani_widget_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	struct shutdown_ani_widget *w = CONTAINER_OF(widget, 
		struct shutdown_ani_widget, widget);

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_TIMER:
		if (event_ptr->gx_event_payload.gx_event_timer_id == UX_TIMER_ID(widget)) {
			w->angle += 20;
			gx_system_dirty_partial_add(widget, &w->client);
		}
		break;
	case GX_EVENT_ANIMATION_COMPLETE:
		gx_system_timer_start(widget, UX_TIMER_ID(widget), 1, 5);
		break;
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

GX_WIDGET *shutdown_ani_widget_get(void)
{
	return &popup_widget.widget;
}

UINT shutdown_ani_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
	struct shutdown_ani_widget *w = &popup_widget;
	GX_RECTANGLE client;

	gx_utility_rectangle_define(&client, 0, 0, 320, 360);
	gx_widget_create(&w->widget, "shutdown-ani", parent, 
		GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, 0, &client);
	gx_widget_fill_color_set(&w->widget, GX_COLOR_ID_HONBOW_BLACK,
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&w->widget, shutdown_ani_widget_event_process);
	gx_widget_draw_set(&w->widget, shutdown_ani_widget_draw);

	gx_utility_rectangle_define(&w->client, 113, 133, 320-113, 360-133);
	w->icon = GX_PIXELMAP_ID_SHUTDOWN_ANI_ICON;
	w->angle = 0;
	return GX_SUCCESS;
}