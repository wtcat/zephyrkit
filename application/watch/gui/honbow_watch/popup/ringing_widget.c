#include "ux/ux_api.h"

#include "app_list/app_list_define.h"
#include "popup/widget.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"

#define RINGING_TIMER_ID 0x8001

#define MUTE_BUTTON_ID 0x01
#define REJECT_BUTTON_ID 0x02

struct ringing_widget {
	struct pixelmap_button mute;
	struct pixelmap_button reject;
	GX_PROMPT name;
	GX_PROMPT phone;
	GX_WIDGET widget;
	GX_RECTANGLE size;
	GX_RESOURCE_ID call_icon;
	GX_VALUE angle;
	void (*draw)(struct ringing_widget *);
};

static struct ringing_widget popup_widget;

static void ringing_pixelmap_shake(struct ringing_widget *rwidget)
{
	GX_PIXELMAP *pixelmap;
	GX_RECTANGLE size;
	GX_VALUE xpos, ypos;

	gx_utility_rectangle_define(&size, 120, 34, 120 + 79, 34 + 79);
	gx_context_pixelmap_get(rwidget->call_icon, &pixelmap);
	if (pixelmap) {
		xpos = size.gx_rectangle_right - size.gx_rectangle_left - 
			pixelmap->gx_pixelmap_width + 1;
		xpos >>= 1;
		xpos += size.gx_rectangle_left;
		ypos = size.gx_rectangle_bottom - size.gx_rectangle_top - 
			pixelmap->gx_pixelmap_height + 1;
		ypos >>= 1;
		ypos += rwidget->size.gx_rectangle_top;
		gx_canvas_pixelmap_rotate(xpos, ypos, pixelmap, rwidget->angle, -1, -1);
	}
}

static void ringing_pixelmap_show(struct ringing_widget *rwidget)
{
	GX_RECTANGLE *client = &rwidget->widget.gx_widget_size;
	GX_RECTANGLE size;

	size.gx_rectangle_left = client->gx_rectangle_left + 120;
	size.gx_rectangle_top = client->gx_rectangle_top + 34;
	size.gx_rectangle_right = size.gx_rectangle_left + 79;
	size.gx_rectangle_bottom = size.gx_rectangle_top + 79;
	ux_rectangle_pixelmap_draw(&size, rwidget->call_icon, 0);
}

static void ringing_window_draw(GX_WIDGET *widget)
{
	struct ringing_widget *rwidget = CONTAINER_OF(widget, 
		struct ringing_widget, widget);
	_gx_widget_background_draw(widget);
	rwidget->draw(rwidget);
	_gx_widget_children_draw(widget);
}

static UINT ringing_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	struct ringing_widget *rwidget = CONTAINER_OF(widget, struct ringing_widget, widget);
	GX_STRING name_str;
	GX_STRING phone_str;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		rwidget->draw = ringing_pixelmap_show;
		name_str.gx_string_ptr = "Lucy";
		name_str.gx_string_length = 4;
		phone_str.gx_string_ptr = "130 5566 8878";
		phone_str.gx_string_length = 13;
		gx_prompt_text_set_ext(&rwidget->name, &name_str);
		gx_prompt_text_set_ext(&rwidget->phone, &phone_str);
		break;
	case GX_EVENT_HIDE:
		gx_system_timer_stop(widget, RINGING_TIMER_ID);
		break;
	case GX_EVENT_TIMER:
		if (event_ptr->gx_event_payload.gx_event_timer_id == RINGING_TIMER_ID) {
			rwidget->angle *= -1;
			_gx_system_dirty_partial_add((GX_WIDGET *)widget, &rwidget->size);
			return GX_SUCCESS;
		}
		break;
	case GX_EVENT_ANIMATION_COMPLETE:
		if (widget->gx_widget_status & GX_STATUS_VISIBLE) {
			rwidget->draw = ringing_pixelmap_shake;
			gx_system_timer_start(widget, RINGING_TIMER_ID, 5, 5);
		}
		break;
	case GX_SIGNAL(MUTE_BUTTON_ID, GX_EVENT_TOGGLE_ON):
		break;
	case GX_SIGNAL(MUTE_BUTTON_ID, GX_EVENT_TOGGLE_OFF):
		break;
	case GX_SIGNAL(REJECT_BUTTON_ID, GX_EVENT_CLICKED):
		rwidget->draw = ringing_pixelmap_show;
		gx_system_timer_stop(widget, RINGING_TIMER_ID);
		return ux_screen_pop_and_switch(widget, popup_hide_animation);
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

GX_WIDGET *ringing_widget_get(void)
{
	return &popup_widget.widget;
}

UINT ringing_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
	struct ringing_widget *rwidget = &popup_widget;
	GX_RECTANGLE client;

	gx_utility_rectangle_define(&client, 0, 0, 320, 360);
	gx_widget_create(&rwidget->widget, "shutdown", parent, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, 0, &client);
	gx_widget_fill_color_set(&rwidget->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
							 GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&rwidget->widget, ringing_event_process);
	gx_widget_draw_set(&rwidget->widget, ringing_window_draw);

	/* Name */
	gx_utility_rectangle_define(&client, 106, 128, 106 + 108, 128 + 36);
	gx_prompt_create(&rwidget->name, "Name", &rwidget->widget, 0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0,
					 &client);
	gx_prompt_text_color_set(&rwidget->name, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&rwidget->name, GX_FONT_ID_SIZE_26);

	/* Phone number */
	gx_utility_rectangle_define(&client, 55, 174, 55 + 211, 174 + 30);
	gx_prompt_create(&rwidget->phone, "Name", &rwidget->widget, 0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0,
					 &client);
	gx_prompt_text_color_set(&rwidget->phone, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&rwidget->phone, GX_FONT_ID_SIZE_26);

	/* Mute button */
	gx_utility_rectangle_define(&client, 36, 241, 36 + 90, 241 + 90);
	ux_pixelmap_button_create(
		&rwidget->mute, "mute", &rwidget->widget, GX_PIXELMAP_ID_MUTE, GX_COLOR_ID_HONBOW_DARK_GRAY,
		GX_COLOR_ID_HONBOW_GREEN, MUTE_BUTTON_ID,
		GX_STYLE_ENABLED | GX_STYLE_BUTTON_TOGGLE | GX_STYLE_HALIGN_CENTER | GX_STYLE_VALIGN_CENTER, &client, NULL);

	/* Hang-up button*/
	gx_utility_rectangle_define(&client, 194, 241, 194 + 90, 241 + 90);
	ux_pixelmap_button_create(&rwidget->reject, "mute", &rwidget->widget, GX_PIXELMAP_ID_HANG_UP,
							  GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_DARK_GRAY, REJECT_BUTTON_ID,
							  GX_STYLE_ENABLED | GX_STYLE_HALIGN_CENTER | GX_STYLE_VALIGN_CENTER, &client, NULL);

	gx_utility_rectangle_define(&rwidget->size, 120, 34, 120 + 79, 34 + 79);
	rwidget->call_icon = GX_PIXELMAP_ID_CALL;
	rwidget->angle = 10;
	if (ptr)
		*ptr = &rwidget->widget;
	return GX_SUCCESS;
}