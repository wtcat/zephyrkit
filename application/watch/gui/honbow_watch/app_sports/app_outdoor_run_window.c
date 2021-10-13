#include "app_outdoor_run_window.h"
#include "app_sports_window.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "app_list_define.h"
#include "drivers_ext/sensor_priv.h"
#include "custom_window_title.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include "string.h"
#include "stdio.h"

#include "custom_rounded_button.h"
#include "zephyr.h"
#include "guix_language_resources_custom.h"
#include "app_sports_window.h"
#if 1

GX_WIDGET outdoor_run_window;

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);
static Custom_title_icon_t icon_title = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,	
	.icon = GX_PIXELMAP_ID_APP_SPORT_BTN_BACK,
};

static uint8_t type;

static GX_PROMPT outdoor_run_prompt;
static GX_PROMPT time_prompt;
extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];
static GX_VERTICAL_LIST app_outdoor_run_list;
typedef struct APP_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	Custom_RoundedIconButton_TypeDef child;
} APP_OUTDOOR_RUN_LIST_ROW;


GX_SCROLLBAR_APPEARANCE app_outdoor_run_list_scrollbar_properties = {
	2,						  /* scroll width                   */
	2,						  /* thumb width                    */
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


#define APP_OUTDOOR_RUN_LIST_VISIBLE_ROWS 4
static APP_OUTDOOR_RUN_LIST_ROW app_outdoor_run_list_row_memory[APP_OUTDOOR_RUN_LIST_VISIBLE_ROWS];
extern GX_WINDOW_ROOT *root;

static const GX_CHAR *lauguage_en_setting_element[4] = {"Free Training", "Calories", "Distance", "Time"};

static const GX_CHAR **setting_string[] = {
	[LANGUAGE_ENGLISH] = lauguage_en_setting_element};
static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return setting_string[id];
}

VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;

UINT app_outdoor_run_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		return 0;
	}
	case GX_EVENT_PEN_UP: {
		delta_x = (event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x >= 50) {
			jump_sport_page(&sport_main_page, &outdoor_run_window, SCREEN_SLIDE_RIGHT);
			gx_widget_delete(&outdoor_run_window);			
		}
		return 0;
	}
	}
	return gx_widget_event_process(window, event_ptr);
}

UINT app_outdoor_run_list_widget_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	APP_OUTDOOR_RUN_LIST_ROW *row = (APP_OUTDOOR_RUN_LIST_ROW *)widget;
	switch (event_ptr->gx_event_type) {	
		case GX_SIGNAL(1, GX_EVENT_CLICKED):
			if (row->child.id) {
				app_sport_goal_set_window_init(NULL, type, row->child.id);
				jump_sport_page(&sport_goal_set_window, &outdoor_run_window, SCREEN_SLIDE_LEFT);				
			}else {
				jump_sport_page(&gps_connecting_window, &outdoor_run_window, SCREEN_SLIDE_LEFT);
				gx_widget_delete(&outdoor_run_window);
			}			
			return 0;
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

UINT app_bicycle_list_widget_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	APP_OUTDOOR_RUN_LIST_ROW *row = (APP_OUTDOOR_RUN_LIST_ROW *)widget;
	switch (event_ptr->gx_event_type) {	
		case GX_SIGNAL(1, GX_EVENT_CLICKED):
			if (row->child.id) {
				if(2 == row->child.id) {
					 row->child.id++;
				}
				app_sport_goal_set_window_init(NULL, type, row->child.id);
				jump_sport_page(&sport_goal_set_window, &outdoor_run_window, SCREEN_SLIDE_LEFT);				
			}else {
				jump_sport_page(&gps_connecting_window, &outdoor_run_window, SCREEN_SLIDE_LEFT);
				gx_widget_delete(&outdoor_run_window);
			}
			return 0;
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

void app_outdoor_run_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

void app_bicycle_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	if (2 == index)	{
		index++;
	}
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

static GX_RESOURCE_ID outdoor_run_icon = GX_PIXELMAP_ID_APP_SPORT_BTN_BACK;

static VOID app_outdoor_run_list_callback(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_OUTDOOR_RUN_LIST_ROW *row = CONTAINER_OF(widget, APP_OUTDOOR_RUN_LIST_ROW, widget);
	GX_STRING string;
	app_outdoor_run_list_prompt_get(index, &string);
	gx_prompt_text_set_ext(&row->child.desc, &string);
	row->child.icon0 = outdoor_run_icon;
	row->child.id = index;
}

static VOID app_bicycle_run_list_callback(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_OUTDOOR_RUN_LIST_ROW *row = CONTAINER_OF(widget, APP_OUTDOOR_RUN_LIST_ROW, widget);
	GX_STRING string;
	app_bicycle_list_prompt_get(index, &string);
	gx_prompt_text_set_ext(&row->child.desc, &string);
	row->child.icon0 = outdoor_run_icon;
	row->child.id = index;
}

UINT outdoor_run_button_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	UINT status = 0;
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN:
		if (widget->gx_widget_style & GX_STYLE_ENABLED) {
			_gx_system_input_capture(widget);
			widget->gx_widget_style |= GX_STYLE_BUTTON_PUSHED;
			gx_system_dirty_mark(widget);
		}
		status = gx_widget_event_to_parent(widget, event_ptr);
		break;
	case GX_EVENT_PEN_UP:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release(widget);
			widget->gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
			gx_system_dirty_mark(widget);
			if (gx_utility_rectangle_point_detect(&widget->gx_widget_size,
				event_ptr->gx_event_payload.gx_event_pointdata)) {
				gx_widget_event_generate(widget, GX_EVENT_CLICKED, 0);
			}
		}
		// need check clicked event happended or not
		status = gx_widget_event_to_parent(widget, event_ptr);
		break;
	case GX_EVENT_INPUT_RELEASE:
		if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
			_gx_system_input_release(widget);
			widget->gx_widget_style &= ~GX_STYLE_BUTTON_PUSHED;
			gx_system_dirty_mark(widget);
		}
		break;
	default:
		gx_widget_event_process(widget, event_ptr);
	}
	return status;
}

extern GX_WINDOW_ROOT *root;
void outdoor_run_button_draw(GX_WIDGET *widget)
{
	GX_RECTANGLE fillrect;
	INT xpos;
	INT ypos;

	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
	gx_context_brush_width_set(0);
	gx_canvas_rectangle_draw(&fillrect);
	Custom_RoundedIconButton_TypeDef *element = CONTAINER_OF(widget, 
		Custom_RoundedIconButton_TypeDef, widget);
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);
	GX_COLOR color = 0;
	gx_context_color_get(element->bg_color, &color);

	if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		uint8_t red = ((color >> 11) >> 1);
		uint8_t green = (((color >> 5) & 0x3f) >> 1);
		uint8_t blue = ((color & 0x1f) >> 1);
		color = (red << 11) + (green << 5) + blue;
	}
	uint16_t width = abs(widget->gx_widget_size.gx_rectangle_bottom - 
		widget->gx_widget_size.gx_rectangle_top) + 1;
	gx_brush_define(&current_context->gx_draw_context_brush, color, 0, GX_BRUSH_ALIAS);
	gx_context_brush_width_set(width - 20);
	uint16_t half_line_width_2 = ((width - 20) >> 1) + 1;
	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + 1,
		widget->gx_widget_size.gx_rectangle_top + 10 + half_line_width_2,
		widget->gx_widget_size.gx_rectangle_right - 1,
		widget->gx_widget_size.gx_rectangle_top + 10 + half_line_width_2);
	gx_brush_define(&current_context->gx_draw_context_brush, color, 
		0, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
	gx_context_brush_width_set(20);
	uint16_t half_line_width = 11;
	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + half_line_width,
						widget->gx_widget_size.gx_rectangle_top + half_line_width,
						widget->gx_widget_size.gx_rectangle_right - half_line_width,
						widget->gx_widget_size.gx_rectangle_top + half_line_width);
	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + half_line_width,
						widget->gx_widget_size.gx_rectangle_bottom - half_line_width + 2,
						widget->gx_widget_size.gx_rectangle_right - half_line_width,
						widget->gx_widget_size.gx_rectangle_bottom - half_line_width + 2);
	// draw icon
	GX_PIXELMAP *map = NULL;
	gx_context_pixelmap_get(element->icon0, &map);
	if (map) {
		xpos = widget->gx_widget_size.gx_rectangle_right - 
			widget->gx_widget_size.gx_rectangle_left - map->gx_pixelmap_width + 1;
		xpos /= 2;
		xpos += widget->gx_widget_size.gx_rectangle_left;

		ypos = widget->gx_widget_size.gx_rectangle_bottom - 
			widget->gx_widget_size.gx_rectangle_top - map->gx_pixelmap_height + 1;
		ypos /= 2;
		ypos += widget->gx_widget_size.gx_rectangle_top;
		gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
	}
	gx_widget_children_draw(widget);
}

void outdoor_run_button_create(Custom_RoundedIconButton_TypeDef *element, 
	USHORT widget_id, GX_WIDGET *parent, GX_RECTANGLE *size, 
	GX_RESOURCE_ID color_id, GX_RESOURCE_ID icon_id, GX_STRING *desc,
	UCHAR disp_type)
{
	gx_widget_create(&element->widget, GX_NULL, parent, 
		GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, widget_id, size);

	if (disp_type != CUSTOM_ROUNDED_BUTTON_ICON_ONLY) {
		if (desc != NULL) {
			GX_RECTANGLE child_size = *size;
			ULONG style = 0;
			if (disp_type == CUSTOM_ROUNDED_BUTTON_TEXT_LEFT) {
				style = GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE;
			} else {
				style = GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE;
			}
			gx_prompt_create(&element->desc, NULL, &element->widget, 0, style, 0, &child_size);
			gx_prompt_text_color_set(&element->desc, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_prompt_font_set(&element->desc, GX_FONT_ID_SIZE_30);
			gx_prompt_text_set_ext(&element->desc, desc);
		}
	}
	gx_widget_draw_set(&element->widget, outdoor_run_button_draw);
	gx_widget_event_process_set(&element->widget, outdoor_run_button_event);
	element->bg_color = color_id;
	element->icon0 = icon_id;
	element->disp_type = disp_type;
}

static VOID app_outdoor_run_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	APP_OUTDOOR_RUN_LIST_ROW *row = (APP_OUTDOOR_RUN_LIST_ROW *)widget;
	GX_BOOL result;

	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 68 + index * 88, 308, 156 + index * 88);
		gx_widget_create(&row->widget, NULL, list, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&row->widget, app_outdoor_run_list_widget_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);		
		childsize.gx_rectangle_bottom -= 10;
		GX_STRING string;
		app_outdoor_run_list_prompt_get(index, &string);
		outdoor_run_button_create(&row->child, 1, &row->widget, &childsize, 
			GX_COLOR_ID_HONBOW_DARK_GRAY, GX_ID_NONE, &string, 
			CUSTOM_ROUNDED_BUTTON_TEXT_MIDDLE);
		row->child.id = index;
	}
}

static VOID app_bicycle_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	APP_OUTDOOR_RUN_LIST_ROW *row = (APP_OUTDOOR_RUN_LIST_ROW *)widget;
	GX_BOOL result;

	gx_widget_created_test(&row->widget, &result);
	if (!result) {	
		gx_utility_rectangle_define(&childsize, 12, 68 + index * 88, 308, 156 + index * 88);
		gx_widget_create(&row->widget, NULL, list, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&row->widget, app_bicycle_list_widget_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);		
		childsize.gx_rectangle_bottom -= 10;
		GX_STRING string;
		app_bicycle_list_prompt_get(index, &string);
		outdoor_run_button_create(&row->child, 1, &row->widget, &childsize, 
			GX_COLOR_ID_HONBOW_DARK_GRAY, GX_ID_NONE, &string, 
			CUSTOM_ROUNDED_BUTTON_TEXT_MIDDLE);
		row->child.id = index;
	}
}

static VOID app_outdoor_run_list_children_create(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < APP_OUTDOOR_RUN_LIST_VISIBLE_ROWS; count++) {
		app_outdoor_run_list_row_create(list, 
			(GX_WIDGET *)&app_outdoor_run_list_row_memory[count], count);
	}
}

static VOID app_bicycle_list_children_create(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < APP_OUTDOOR_RUN_LIST_VISIBLE_ROWS-1; count++) {
		app_bicycle_list_row_create(list, 
			(GX_WIDGET *)&app_outdoor_run_list_row_memory[count], count);
	}
}


void app_outdoor_run_window_init(GX_WINDOW *window, SPORT_CHILD_PAGE_T sport_type)
{
	GX_RECTANGLE win_size;
	gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
	gx_widget_create(&outdoor_run_window, "outdoor_run_window", window, 
		GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 0, &win_size);

	GX_WIDGET *parent = &outdoor_run_window;
	custom_window_title_create(parent, &icon_title, &outdoor_run_prompt, &time_prompt);
	GX_STRING string;
	app_sport_list_prompt_get(sport_type, &string);
	gx_prompt_text_set_ext(&outdoor_run_prompt, &string);
	type = sport_type;
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	time_t now = ts.tv_sec;
	struct tm *tm_now = gmtime(&now);
	snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
	time_prompt_buff[7] = 0;
	string.gx_string_ptr = time_prompt_buff;
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&time_prompt, &string);

	GX_BOOL result;
	gx_widget_created_test(&app_outdoor_run_list, &result);
	if (!result) {
		GX_RECTANGLE child_size;
		gx_utility_rectangle_define(&child_size, 12, 68, 308, 359);
		if ((OUTDOOR_BICYCLE_CHILD_PAGE == sport_type) || 
			(INDOOR_BICYCLE_CHILD_PAGE == sport_type)){
			gx_vertical_list_create(&app_outdoor_run_list, NULL,
				parent, 3, app_bicycle_run_list_callback, 
				GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, 
				GX_ID_NONE, &child_size);
			gx_widget_fill_color_set(&app_outdoor_run_list, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			app_bicycle_list_children_create(&app_outdoor_run_list);
		}else {
			gx_vertical_list_create(&app_outdoor_run_list, NULL,
				parent, 4, app_outdoor_run_list_callback,
				GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child_size);
			gx_widget_fill_color_set(&app_outdoor_run_list, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			app_outdoor_run_list_children_create(&app_outdoor_run_list);	
		}
	}
	gx_widget_event_process_set(&outdoor_run_window, app_outdoor_run_window_event);
}
#endif



