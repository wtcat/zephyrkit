#include "ux/ux_api.h"
#include "gx_widget.h"
#include "gx_window.h"
#include "gx_system.h"
#include "gx_scrollbar.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"

#define NULL_RESOURCE_ID ((GX_RESOURCE_ID)-1)
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

struct language_item {
	GX_WIDGET widget;
	GX_PROMPT prompt;
	GX_RESOURCE_ID icon_id;
	GX_RESOURCE_ID text_id;
};

struct language_list {
	GX_VERTICAL_LIST list;
	GX_SCROLLBAR vscrollbar;
};


static struct language_list language_list;

static const GX_CHAR *language_strings[] = {
	[LANGUAGE_ENGLISH] = "English",
};

static struct language_item language_items[] = {
	{
		.icon_id = GX_PIXELMAP_ID_IC_MOON,
		.text_id = 0,
	}
};

static void language_item_draw(GX_WIDGET *widget)
{
	struct language_item *item = CONTAINER_OF(widget, struct language_item, widget);
	GX_DRAW_CONTEXT *context;

	if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		GX_CANVAS *canvas;
		GX_COLOR color;
		uint16_t width;

		canvas = ux_root_window_get(NULL, NULL)->gx_window_root_canvas;
		gx_system_draw_context_get(&context);
		gx_context_color_get(GX_COLOR_ID_HONBOW_DARK_GRAY, &color);
		gx_brush_define(&context->gx_draw_context_brush, color, 0, GX_BRUSH_ALIAS);
		width = widget->gx_widget_size.gx_rectangle_bottom - 
				widget->gx_widget_size.gx_rectangle_top + 1;
		gx_context_brush_width_set(width);
		gx_canvas_rectangle_draw(&widget->gx_widget_size);
	} else {
		_gx_widget_background_draw(widget);
	}

	ux_rectangle_pixelmap_draw(&widget->gx_widget_size, item->icon_id, 
		GX_STYLE_VALIGN_CENTER);
	_gx_widget_children_draw(widget);
}

static UINT language_item_event_process(GX_WIDGET *widget, 
		GX_EVENT *event_ptr)
{
	struct language_item *item = CONTAINER_OF(widget, 
			struct language_item, widget);
	UINT ret;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN:
		if (widget->gx_widget_style & GX_STYLE_ENABLED) {
			_gx_system_input_capture(widget);
			widget->gx_widget_style |= GX_STYLE_BUTTON_PUSHED;
			item->prompt.gx_widget_style |= GX_STYLE_DRAW_SELECTED;
			gx_system_dirty_mark(widget);
		}
		ret = gx_widget_event_to_parent(widget, event_ptr);
		break;
	case GX_EVENT_PEN_UP:
		if (widget->gx_widget_style & GX_STYLE_ENABLED) {
			_gx_system_input_release(widget);
			widget->gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
			item->prompt.gx_widget_style &= ~GX_STYLE_DRAW_SELECTED;
			gx_system_dirty_mark(widget);
			if (gx_utility_rectangle_point_detect(&widget->gx_widget_size,
				event_ptr->gx_event_payload.gx_event_pointdata)) {
				gx_widget_event_generate(widget, GX_EVENT_CLICKED, 0);
			}
		}
		ret = gx_widget_event_to_parent(widget, event_ptr);
		break;
	case GX_EVENT_INPUT_RELEASE:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release(widget);
			widget->gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
			item->prompt.gx_widget_style &= ~GX_STYLE_DRAW_SELECTED;
			gx_system_dirty_mark(widget);
		}
	default:
		ret = gx_widget_event_process(widget, event_ptr);
		break;
	}
	return ret;
}

static void language_list_callback(GX_VERTICAL_LIST *list, 
		GX_WIDGET *widget, INT index)
{
	struct language_list *ll = (struct language_list *)list;
	struct language_item *item = CONTAINER_OF(widget, 
			struct language_item, widget);
	GX_RECTANGLE size;
	GX_BOOL result;

	gx_widget_created_test(&item->widget, &result);
	if (!result) {
		GX_STRING str;

		gx_utility_rectangle_define(&size, 20, 22, 320-20, 22+80);
		gx_widget_create(&item->widget, "", &ll->list,
			GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, 0, &size);
		gx_widget_draw_set(&item->widget, language_item_draw);
		gx_widget_event_process_set(&item->widget, language_item_event_process);
		gx_widget_fill_color_set(&item->widget, GX_COLOR_ID_HONBOW_BLACK,
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

		gx_utility_rectangle_define(&size, 92, 45, 
				item->widget.gx_widget_size.gx_rectangle_right, 45+35);
		gx_prompt_create(&item->prompt, "", &item->widget, 0, 
			GX_STYLE_BORDER_NONE | GX_STYLE_TRANSPARENT | 
			GX_STYLE_TEXT_LEFT | GX_STYLE_ENABLED, 0, &size);
		gx_prompt_text_color_set(&item->prompt, GX_COLOR_ID_HONBOW_WHITE,
			GX_COLOR_ID_HONBOW_BLUE, GX_COLOR_ID_HONBOW_WHITE);
		gx_prompt_font_set(&item->prompt, GX_FONT_ID_SIZE_26);

		str.gx_string_ptr = language_strings[item->text_id];
		str.gx_string_length = strlen(str.gx_string_ptr);
		gx_prompt_text_set_ext(&item->prompt, &str);
	}
}

static UINT list_children_position(GX_VERTICAL_LIST *vertical_list)
{
    GX_RECTANGLE childsize = vertical_list->gx_window_client;
    GX_WIDGET *child = vertical_list->gx_widget_first_child;
    INT index = vertical_list->gx_vertical_list_top_index;
    GX_VALUE height;
    GX_VALUE client_height;

    vertical_list->gx_vertical_list_child_height = 0;
    vertical_list->gx_vertical_list_child_count = 0;
    while (child) {
        if (!(child->gx_widget_status & GX_STATUS_NONCLIENT)) {
            vertical_list->gx_vertical_list_child_count++;
            if (!(child->gx_widget_id)) {
                child->gx_widget_id = (USHORT)(LIST_CHILD_ID_START +
                    vertical_list->gx_vertical_list_child_count);
            }
            if (index == vertical_list->gx_vertical_list_selected)
                child->gx_widget_style |= GX_STYLE_DRAW_SELECTED;
            else
                child->gx_widget_style &= ~GX_STYLE_DRAW_SELECTED;
            index++;
            child->gx_widget_status &= ~GX_STATUS_ACCEPTS_FOCUS;
            _gx_widget_height_get(child, &height);
            if (height > vertical_list->gx_vertical_list_child_height)
                vertical_list->gx_vertical_list_child_height = height;
            /* move this child into position */
            childsize.gx_rectangle_bottom = (GX_VALUE)(childsize.gx_rectangle_top + height - 1);
            _gx_widget_resize(child, &childsize);
            childsize.gx_rectangle_top = (GX_VALUE)(childsize.gx_rectangle_bottom + 10);
        }
        child = child->gx_widget_next;
    }

    /* calculate number of visible rows, needed for scrolling info */
    if (vertical_list->gx_vertical_list_child_height > 0) {
				GX_VALUE average_height = vertical_list->gx_vertical_list_child_height;
        _gx_window_client_height_get((GX_WINDOW *)vertical_list, &client_height);
        vertical_list->gx_vertical_list_visible_rows = 
            (GX_VALUE)((client_height + average_height - 1) / average_height);
    } else {
        vertical_list->gx_vertical_list_visible_rows = 1;
    }
    return GX_SUCCESS;
}

static UINT language_list_event_process(GX_VERTICAL_LIST *list, 
	GX_EVENT *event_ptr)
{
		GX_WIDGET *widget = (GX_WIDGET *)list;
		INT list_height;
		INT widget_height;
		INT new_pen_index;
		GX_WIDGET *child;
    GX_SCROLLBAR *pScroll;

		switch (event_ptr->gx_event_type) {
    case GX_EVENT_SHOW:
        _gx_window_event_process((GX_WINDOW *)list, event_ptr);
        if (!list->gx_vertical_list_child_count)
            list_children_position(list);
        _gx_window_scrollbar_find((GX_WINDOW *)list, GX_TYPE_VERTICAL_SCROLL, &pScroll);
        if (pScroll)
            _gx_scrollbar_reset(pScroll, GX_NULL);
        break;
		case GX_EVENT_PEN_UP:
			if (list->gx_widget_status & GX_STATUS_OWNS_INPUT) {
				_gx_system_input_release(widget);
				list_height = list->gx_vertical_list_child_count * 
					list->gx_vertical_list_child_height;
				widget_height = list->gx_window_client.gx_rectangle_bottom - 
					list->gx_window_client.gx_rectangle_top + 1;
				if (list_height > widget_height)
					_gx_vertical_list_slide_back_check(list);
				if (list->gx_vertical_list_pen_index >= 0 && 
						list->gx_vertical_list_snap_back_distance == 0) {
					/* test to see if pen-up is over same child widget as pen-down */
					child = _gx_system_top_widget_find((GX_WIDGET *)list, 
						event_ptr->gx_event_payload.gx_event_pointdata, GX_STATUS_SELECTABLE);
					while (child && child->gx_widget_parent != (GX_WIDGET *)list)
						child = child->gx_widget_parent;
					if (child) {
						new_pen_index = list->gx_vertical_list_top_index + 
								_gx_widget_client_index_get(widget, child);
						if (new_pen_index >= list->gx_vertical_list_total_rows)
							new_pen_index -= list->gx_vertical_list_total_rows;
						if (new_pen_index == list->gx_vertical_list_pen_index)
							_gx_vertical_list_selected_set(list, list->gx_vertical_list_pen_index);
					}
				}
			} else {
				_gx_widget_event_to_parent((GX_WIDGET *)list, event_ptr);
			}
			break;
		default:
			_gx_vertical_list_event_process(list, event_ptr);
			break;
		}
		return GX_SUCCESS;
}

static UINT language_window_event_process(GX_WINDOW *window, GX_EVENT *event_ptr)
{
		UINT ret = GX_SUCCESS;

		switch (event_ptr->gx_event_type) {
		case GX_SIGNAL(LIST_CHILD_ID_START+1, GX_EVENT_CLICKED):
				gx_system_active_language_set(LANGUAGE_ENGLISH);
				ux_screen_pop_and_switch((GX_WIDGET *)window, NULL);
				break;
		case GX_SIGNAL(LIST_CHILD_ID_START+2, GX_EVENT_CLICKED):
				gx_system_active_language_set(LANGUAGE_CHINESE);
				ux_screen_pop_and_switch((GX_WIDGET *)window, NULL);
				break;
		default:
				ret = gx_widget_event_process(window, event_ptr);
				break;
		}
		return ret;
}

static void language_children_create(struct language_list *ll, int rows)
{
	for (int i = 0; i < rows; i++) 
		language_list_callback(&ll->list, &language_items[i].widget, i);
}

UINT language_window_create(const char *name, GX_WIDGET *parent, GX_WIDGET **ptr)
{
		struct language_list *ll = &language_list;
		// GX_SCROLLBAR_APPEARANCE scrollbar;
		GX_RECTANGLE client;
		GX_WIDGET *win;
		UINT ret;

		ret = gx_studio_named_widget_create((char *)name, parent, &win);
		if (ret != GX_SUCCESS)
			return ret;
		client = win->gx_widget_size;
		gx_vertical_list_create(&ll->list, "language-list", win, ARRAY_SIZE(language_items), 
			language_list_callback, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT,
			GX_ID_NONE, &client);

		// memset(&scrollbar, 0, sizeof(scrollbar));
		// scrollbar.gx_scroll_width = 5;
		// scrollbar.gx_scroll_thumb_width = 5;
		// scrollbar.gx_scroll_thumb_border_style = 4;
		// scrollbar.gx_scroll_thumb_color = GX_COLOR_ID_HONBOW_WHITE;
		// scrollbar.gx_scroll_thumb_border_color = GX_COLOR_ID_HONBOW_WHITE;
		// scrollbar.gx_scroll_button_color = GX_COLOR_ID_HONBOW_WHITE;
		// gx_vertical_scrollbar_create(&ll->vscrollbar, "language-bar", &ll->list, &scrollbar,
		// 	GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB | GX_SCROLLBAR_VERTICAL);

		gx_widget_event_process_set(&ll->list, language_list_event_process);
		language_children_create(ll, ARRAY_SIZE(language_items));
		gx_widget_event_process_set(win, language_window_event_process);
		if (ptr)
			*ptr = win;
		return ret;
}

