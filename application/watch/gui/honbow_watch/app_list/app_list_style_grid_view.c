#include "app_list_style_grid_view.h"
#include "app_list_define.h"
#include "custom_horizontal_total_rows.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "gx_window.h"
#include "stdio.h"
#include "string.h"
#include "sys/util.h"
#include "sys/printk.h"
#include "custom_vertical_scroll_limit.h"
#include "windows_manager.h"

static GX_VERTICAL_LIST app_list;

static GX_SCROLLBAR_APPEARANCE app_list_scrollbar_properties = {
	8,						  /* scroll width                   */
	8,						  /* thumb width                    */
	0,						  /* thumb travel min               */
	0,						  /* thumb travel max               */
	4,						  /* thumb border style             */
	0,						  /* scroll fill pixelmap           */
	0,						  /* scroll thumb pixelmap          */
	0,						  /* scroll up pixelmap             */
	0,						  /* scroll down pixelmap           */
	GX_COLOR_ID_HONBOW_WHITE, /* scroll thumb color             */
	GX_COLOR_ID_HONBOW_WHITE, /* scroll thumb border color      */
	GX_COLOR_ID_HONBOW_WHITE, /* scroll button color            */
};
static GX_SCROLLBAR app_list_scrollbar;
#define APP_LIST_VISIBLE_ROWS 3

#define ID_CHILD1 1
#define ID_CHILD2 2
#define ID_CHILD3 3

typedef struct APP_CHILD_STRUCT {
	GX_PIXELMAP_BUTTON pixelmap_button;
	GX_WIDGET *dst_widget;
	GX_RESOURCE_ID icon_id;
} APP_CHILD;
typedef struct APP_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	APP_CHILD child1;
	APP_CHILD child2;
	APP_CHILD child3;
	UCHAR app_group;
} APP_LIST_ROW;
APP_LIST_ROW app_list_row_memory[APP_LIST_VISIBLE_ROWS + 1];

static VOID app_list_row_widget_draw(GX_WIDGET *widget)
{
	gx_widget_children_draw(widget);
}
extern VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);

UINT app_list_row_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(ID_CHILD1, GX_EVENT_CLICKED): {
		APP_LIST_ROW *row = CONTAINER_OF(widget, APP_LIST_ROW, widget);
		if (row->child1.dst_widget != NULL) {
			// screen_switch((GX_WIDGET *)&root_window, row->child1.dst_widget);
			windows_mgr_page_jump((GX_WINDOW *)&app_list_window, (GX_WINDOW *)row->child1.dst_widget,
								  WINDOWS_OP_SWITCH_NEW);
		}
		return 0;
	}
	case GX_SIGNAL(ID_CHILD2, GX_EVENT_CLICKED): {
		APP_LIST_ROW *row = CONTAINER_OF(widget, APP_LIST_ROW, widget);
		if (row->child2.dst_widget != NULL) {
			// screen_switch((GX_WIDGET *)&root_window, row->child2.dst_widget);
			windows_mgr_page_jump((GX_WINDOW *)&app_list_window, (GX_WINDOW *)row->child2.dst_widget,
								  WINDOWS_OP_SWITCH_NEW);
		}
		return 0;
	}
	case GX_SIGNAL(ID_CHILD3, GX_EVENT_CLICKED): {
		APP_LIST_ROW *row = CONTAINER_OF(widget, APP_LIST_ROW, widget);
		if (row->child3.dst_widget != NULL) {
			// screen_switch((GX_WIDGET *)&root_window, row->child3.dst_widget);
			windows_mgr_page_jump((GX_WINDOW *)&app_list_window, (GX_WINDOW *)row->child3.dst_widget,
								  WINDOWS_OP_SWITCH_NEW);
		}
		return 0;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

static void sync_info_widget(const APP_LIST_META_DATA *meta, APP_CHILD *child)
{
	if ((meta != NULL) && (child != NULL)) {
		gx_pixelmap_button_pixelmap_set(&child->pixelmap_button, meta->res_id_normal, meta->res_id_select,
										meta->res_id_normal);
		child->dst_widget = meta->app_widget;
		child->icon_id = meta->res_id_normal;
	}
}

extern VOID _gx_widget_context_fill_set(GX_WIDGET *widget);
VOID app_list_pixelmap_button_draw(GX_PIXELMAP_BUTTON *button)
{

	GX_RESOURCE_ID pixelmap;
	GX_RESOURCE_ID pixelmap_overlay = 0;

	GX_PIXELMAP *map;
	GX_WIDGET *widget = (GX_WIDGET *)button;
	INT xpos;
	INT ypos;
	GX_PIXELMAP *map_overlay = NULL;

#if 0
#if 1
	GX_RECTANGLE fillrect;
	gx_widget_client_get(widget, 0, &fillrect);

	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);
#else
	_gx_widget_border_draw(widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, true);
#endif
#endif
	/* default to using the normal pixlemap */
	pixelmap = button->gx_pixelmap_button_normal_id;

	gx_context_pixelmap_get(pixelmap, &map);

	if (button->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		pixelmap_overlay = button->gx_pixelmap_button_selected_id;
		gx_context_pixelmap_get(pixelmap_overlay, &map_overlay);
	}

	if (map) {
		xpos = button->gx_widget_size.gx_rectangle_right - button->gx_widget_size.gx_rectangle_left -
			   map->gx_pixelmap_width + 1;

		xpos /= 2;
		xpos += button->gx_widget_size.gx_rectangle_left;

		ypos = button->gx_widget_size.gx_rectangle_bottom - button->gx_widget_size.gx_rectangle_top -
			   map->gx_pixelmap_height + 1;

		ypos /= 2;
		ypos += button->gx_widget_size.gx_rectangle_top;

		_gx_widget_context_fill_set(widget);
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);

		if (map_overlay) {
			gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map_overlay);
		}
	}
}
static VOID app_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	APP_LIST_ROW *row = (APP_LIST_ROW *)widget;
	GX_BOOL result;

	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 293, 109);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_draw_set(&row->widget, app_list_row_widget_draw);
		gx_widget_event_process_set(&row->widget, app_list_row_event);

		gx_utility_rectangle_define(&childsize, 0, 0, 97, 99);
		gx_pixelmap_button_create(&row->child1.pixelmap_button, NULL, &row->widget, GX_PIXELMAP_NULL, GX_PIXELMAP_NULL,
								  GX_PIXELMAP_NULL, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, ID_CHILD1, &childsize);
		gx_widget_draw_set(&row->child1.pixelmap_button, app_list_pixelmap_button_draw);

		gx_utility_rectangle_define(&childsize, 98, 0, 195, 99);
		gx_pixelmap_button_create(&row->child2.pixelmap_button, NULL, &row->widget, GX_PIXELMAP_NULL, GX_PIXELMAP_NULL,
								  GX_PIXELMAP_NULL, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, ID_CHILD2, &childsize);
		gx_widget_draw_set(&row->child2.pixelmap_button, app_list_pixelmap_button_draw);

		gx_utility_rectangle_define(&childsize, 196, 0, 293, 99);
		gx_pixelmap_button_create(&row->child3.pixelmap_button, NULL, &row->widget, GX_PIXELMAP_NULL, GX_PIXELMAP_NULL,
								  GX_PIXELMAP_NULL, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, ID_CHILD3, &childsize);
		gx_widget_draw_set(&row->child3.pixelmap_button, app_list_pixelmap_button_draw);
	}

	row->app_group = index;

	const APP_LIST_META_DATA *meta = app_list_element_meta_get(app_list_element_sequence_get(index * 3));
	sync_info_widget(meta, &row->child1);

	meta = app_list_element_meta_get(app_list_element_sequence_get(index * 3 + 1));
	sync_info_widget(meta, &row->child2);

	meta = app_list_element_meta_get(app_list_element_sequence_get(index * 3 + 2));
	sync_info_widget(meta, &row->child3);
}

static VOID app_list_children_create(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < APP_LIST_VISIBLE_ROWS + 1; count++) {
		app_list_row_create(list, (GX_WIDGET *)&app_list_row_memory[count], count);
	}
}

static VOID app_list_callback(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	// need refresh child1 child2 child3
	APP_LIST_ROW *row = (APP_LIST_ROW *)widget;
	row->app_group = index;
	const APP_LIST_META_DATA *meta = app_list_element_meta_get(app_list_element_sequence_get(index * 3));
	sync_info_widget(meta, &row->child1);

	meta = app_list_element_meta_get(app_list_element_sequence_get(index * 3 + 1));
	sync_info_widget(meta, &row->child2);

	meta = app_list_element_meta_get(app_list_element_sequence_get(index * 3 + 2));
	sync_info_widget(meta, &row->child3);
}

#define SCROLL_BAR_TIMER 10
extern UINT _gx_system_input_release(GX_WIDGET *owner);
extern GX_WIDGET *_gx_system_top_widget_find(GX_WIDGET *root, GX_POINT test_point, ULONG status);
extern INT _gx_widget_client_index_get(GX_WIDGET *parent, GX_WIDGET *child);
extern UINT _gx_widget_event_to_parent(GX_WIDGET *widget, GX_EVENT *event_ptr);
extern UINT _gx_system_timer_stop(GX_WIDGET *owner, UINT timer_id);
extern UINT _gx_scrollbar_reset(GX_SCROLLBAR *scrollbar, GX_SCROLL_INFO *info);
static UINT app_list_event(GX_VERTICAL_LIST *list, GX_EVENT *event_ptr)
{
	GX_WIDGET *widget = (GX_WIDGET *)list;
	UINT timer_id;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		gx_widget_hide(&app_list_scrollbar);
		break;
	case GX_EVENT_HIDE:
		gx_system_timer_stop(widget, SCROLL_BAR_TIMER);
		break;
	case GX_EVENT_PEN_DRAG:
		gx_system_timer_stop(widget, SCROLL_BAR_TIMER);
		gx_widget_show(&app_list_scrollbar);
		break;
	case GX_EVENT_PEN_UP:
		if (list->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			gx_system_timer_start(widget, SCROLL_BAR_TIMER, 100, 0);
			break;
		} else {
			_gx_widget_event_to_parent((GX_WIDGET *)list, event_ptr);
		}
		return 0;
	case GX_EVENT_TIMER:
		timer_id = event_ptr->gx_event_payload.gx_event_timer_id;
		if (timer_id == SCROLL_BAR_TIMER) {
			gx_widget_hide(&app_list_scrollbar);
			return 0;
		}
		break;
	default:
		break;
	}
	return custom_vertical_event_processing_function(widget, event_ptr);
}

void app_list_style_grid_view_init(GX_WINDOW *parenet)
{
	GX_BOOL result;
	gx_widget_created_test(&app_list, &result);
	if (!result) {
		GX_RECTANGLE child_size;
		gx_utility_rectangle_define(&child_size, 10, 22, 316, 352);
		gx_vertical_list_create(&app_list, NULL, parenet, 4, app_list_callback, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED,
								GX_ID_NONE, &child_size);
#if 1
		gx_widget_event_process_set(&app_list, app_list_event);
#else
		gx_widget_event_process_set(&app_list, custom_vertical_event_processing_function);
#endif
		gx_widget_fill_color_set(&app_list, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								 GX_COLOR_ID_HONBOW_BLACK);

		// app position init
		app_list_element_pos_init();
		app_list_children_create(&app_list);

		// add scrollbar
		gx_vertical_scrollbar_create(&app_list_scrollbar, NULL, &app_list, &app_list_scrollbar_properties,
									 GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB |
										 GX_SCROLLBAR_VERTICAL);
		gx_widget_fill_color_set(&app_list_scrollbar, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
								 GX_COLOR_ID_HONBOW_DARK_GRAY);

		extern VOID custom_scrollbar_vertical_draw(GX_SCROLLBAR * scrollbar);
		gx_widget_draw_set(&app_list_scrollbar, custom_scrollbar_vertical_draw);

	} else {
		gx_widget_attach(parenet, &app_list);
	}
}

void app_list_style_grid_view_deinit(void)
{
	gx_widget_detach(&app_list);
}
