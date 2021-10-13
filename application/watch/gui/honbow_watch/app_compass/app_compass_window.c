#include "app_compass_window.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "app_list_define.h"
#include "drivers_ext/sensor_priv.h"
#include "windows_manager.h"
static uint32_t angree = 0;
void app_compass_window_draw(GX_WIDGET *widget)
{
	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);

	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);

	GX_PIXELMAP *map = NULL;
	gx_context_pixelmap_get(GX_PIXELMAP_ID_APP_COMPASS_MAIN, &map);
	if (map != NULL) {
		gx_canvas_pixelmap_rotate(widget->gx_widget_size.gx_rectangle_left + 50,
								  widget->gx_widget_size.gx_rectangle_top + 82, map, angree, -1, -1);
	}

	gx_context_pixelmap_get(GX_PIXELMAP_ID_APP_COMPASS_ARROW, &map);
	if (map != NULL) {
		gx_canvas_pixelmap_draw(widget->gx_widget_size.gx_rectangle_left + 151,
								widget->gx_widget_size.gx_rectangle_top + 70, map);
	}
}

#define COMPASS_TIME_ID 12
VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
UINT app_compass_window_event(GX_WINDOW *window, GX_EVENT *event_ptr)
{
	static GX_VALUE gx_point_x_first;
	static GX_VALUE delta_x;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		gx_system_timer_start(window, COMPASS_TIME_ID, 1, 1);
#if 0
		dev = device_get_binding("sensor_pah8112");
		if (dev != NULL) {
			struct sensor_value value;
			value.val1 = 1;
			sensor_attr_set(dev, (enum sensor_channel)SENSOR_CHAN_HEART_RATE,
							(enum sensor_attribute)SENSOR_ATTR_SENSOR_START_STOP, &value);
		}
#endif
		break;
	case GX_EVENT_HIDE:
		gx_system_timer_stop(window, COMPASS_TIME_ID);
#if 0
		dev = device_get_binding("sensor_pah8112");
		if (dev != NULL) {
			struct sensor_value value;
			value.val1 = 0;
			sensor_attr_set(dev, (enum sensor_channel)SENSOR_CHAN_HEART_RATE,
							(enum sensor_attribute)SENSOR_ATTR_SENSOR_START_STOP, &value);
		}
#endif
		break;
	case GX_EVENT_TIMER: {
		UINT timer_id = event_ptr->gx_event_payload.gx_event_timer_id;
		if (timer_id == COMPASS_TIME_ID) {
			angree += 10;
			if (angree >= 60) {
				angree = 0;
			}
			gx_system_dirty_mark(window);
#if 0
			dev = device_get_binding("sensor_pah8112");
			if (dev != NULL) {
				struct sensor_value value;
				sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_HEART_RATE, &value);
				if (value.val2)
					printk("sensor channel get val1: %d\n", value.val1);
			}
#endif
			return 0;
		}
		break;
	}
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		return 0;
	}
	case GX_EVENT_PEN_UP: {
		delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x >= 50) {
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&app_list_window);
			windows_mgr_page_jump(window, NULL, WINDOWS_OP_BACK);
		}
		return 0;
	}
	}
	return gx_widget_event_process(window, event_ptr);
}

GUI_APP_DEFINE(compass, APP_ID_11_COMPASS, (GX_WIDGET *)&app_compass_window, GX_PIXELMAP_ID_APP_LIST_ICON_COMPASS_90_90,
			   GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
