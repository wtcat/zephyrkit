#include "gx_api.h"
#include "custom_window_title.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "custom_rounded_button.h"
#include "sys/util.h"
#include "custom_window_title.h"
#include <posix/time.h>
#include <sys/timeutil.h>
#include "string.h"
#include "stdio.h"
#include "app_list_define.h"
#include "app_setting.h"
#include "custom_animation.h"
#include "app_setting_disp_page.h"
#include <logging/log.h>
#include "guix_language_resources_custom.h"
#include "custom_vertical_scroll_limit.h"

LOG_MODULE_DECLARE(guix_log);
GX_WIDGET disp_setting_main_page;
static Custom_title_icon_t icon_title = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
};
static GX_PROMPT setting_prompt;
static GX_PROMPT time_prompt;
static char time_prompt_buff[8];

static GX_WIDGET *disp_setting_child[5] = {0};

#define WIDGET_ID_ROUND_BUTTON 1

// for main page
static GX_VERTICAL_LIST app_setting_list;
typedef struct APP_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	Custom_RoundedButton_TypeDef child;
	SETTING_CHILD_PAGE_T id;
} APP_SETTING_LIST_ROW;

static GX_SCROLLBAR_APPEARANCE app_disp_setting_list_scrollbar_properties = {
	9,						  /* scroll width                   */
	9,						  /* thumb width                    */
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
static GX_SCROLLBAR app_setting_list_scrollbar;
#define APP_SETTING_LIST_VISIBLE_ROWS 4
static APP_SETTING_LIST_ROW app_setting_list_row_memory[APP_SETTING_LIST_VISIBLE_ROWS + 1];
extern GX_WINDOW_ROOT *root;

static const GX_CHAR *lauguage_ch_disp_setting_element[5] = {"屏幕亮度", "亮屏时长", "抬腕亮屏", "夜间模式",
															 "省电模式"};
static const GX_CHAR *lauguage_en_disp_setting_element[5] = {"Display Brightness", "Bright Screen Time",
															 "Wake on Wrist Raise", "Night Mode", "Eco Mode"};

static const GX_CHAR **setting_string[] = {
	[LANGUAGE_CHINESE] = lauguage_ch_disp_setting_element, [LANGUAGE_ENGLISH] = lauguage_en_disp_setting_element};
static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return setting_string[id];
}

static const GX_CHAR *get_language_setting_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "显示";
	case LANGUAGE_ENGLISH:
		return "Display";
	default:
		return "Display";
	}
}

static void app_setting_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

static GX_WIDGET *disp_setting_menu_screen_list[] = {
	&disp_setting_main_page,
	GX_NULL,
	GX_NULL,
};

void setting_disp_page_children_reg(GX_WIDGET *dst, DISP_SETTING_CHILD_PAGE_T id)
{
	disp_setting_child[id] = dst;
}

void setting_disp_page_children_return(GX_WIDGET *curr)
{
	disp_setting_menu_screen_list[1] = curr;
	custom_animation_start(disp_setting_menu_screen_list, (GX_WIDGET *)&app_setting_window, SCREEN_SLIDE_RIGHT);
}

static UINT setting_main_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(WIDGET_ID_ROUND_BUTTON, GX_EVENT_CLICKED): {
		APP_SETTING_LIST_ROW *row = CONTAINER_OF(widget, APP_SETTING_LIST_ROW, widget);
		LOG_INF("[%s]row %d clicked!", __FUNCTION__, row->id);
		if (disp_setting_child[row->id]) {
			disp_setting_menu_screen_list[1] = disp_setting_child[row->id];
			custom_animation_start(disp_setting_menu_screen_list, (GX_WIDGET *)&app_setting_window, SCREEN_SLIDE_LEFT);
		}
		return 0;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

static VOID app_setting_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	APP_SETTING_LIST_ROW *row = (APP_SETTING_LIST_ROW *)widget;
	GX_BOOL result;

	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 295, 75 - 1);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&row->widget, setting_main_processing_function);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								 GX_COLOR_ID_HONBOW_BLACK);

		childsize.gx_rectangle_bottom -= 5;
		GX_STRING string;
		app_setting_list_prompt_get(index, &string);
		custom_rounded_button_create(&row->child, WIDGET_ID_ROUND_BUTTON, &row->widget, &childsize,
									 GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string,
									 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
		row->id = index;
	}
}

static VOID app_setting_list_children_create(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < APP_SETTING_LIST_VISIBLE_ROWS + 1; count++) {
		app_setting_list_row_create(list, (GX_WIDGET *)&app_setting_list_row_memory[count], count);
	}
}

static VOID app_setting_list_callback(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_SETTING_LIST_ROW *row = CONTAINER_OF(widget, APP_SETTING_LIST_ROW, widget);
	GX_STRING string;
	app_setting_list_prompt_get(index, &string);
	gx_prompt_text_set_ext(&row->child.desc, &string);
	row->id = index;
}
void setting_main_page_jump_reg(GX_WIDGET *dst, SETTING_CHILD_PAGE_T id);

static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 1
extern void jump_setting_main_page(GX_WIDGET *now);

static UINT disp_setting_event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}
	case GX_EVENT_PEN_UP: {
		delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x >= 50) {
			jump_setting_main_page(&disp_setting_main_page);
		}
		break;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}
void app_setting_brightness_page_init(GX_WINDOW *window);
void app_setting_brightTime_page_init(GX_WINDOW *window);
void app_setting_wrist_raise_page_init(GX_WINDOW *window);
void app_setting_night_mode_page_init(GX_WINDOW *window);
void app_setting_eco_mode_page_init(GX_WINDOW *window);
void app_setting_disp_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&disp_setting_main_page, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
					 &childSize);
	setting_main_page_jump_reg(&disp_setting_main_page, SETTING_CHILD_PAGE_DISP);

	// child page need init
	GX_WIDGET *parent = &disp_setting_main_page;
	custom_window_title_create(parent, &icon_title, &setting_prompt, NULL);

	GX_STRING string;
	string.gx_string_ptr = get_language_setting_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&setting_prompt, &string);

	GX_BOOL result;
	gx_widget_created_test(&app_setting_list, &result);
	if (!result) {
		GX_RECTANGLE child_size;
		gx_utility_rectangle_define(&child_size, 8, 60, 296 + 20 - 1, 359);
		gx_vertical_list_create(&app_setting_list, NULL, parent, 5, app_setting_list_callback,
								GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child_size);
		gx_widget_fill_color_set(&app_setting_list, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								 GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_event_process_set(&app_setting_list, custom_vertical_event_processing_function);

		app_setting_list_children_create(&app_setting_list);

		gx_vertical_scrollbar_create(
			&app_setting_list_scrollbar, NULL, &app_setting_list, &app_disp_setting_list_scrollbar_properties,
			GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB | GX_SCROLLBAR_VERTICAL);
		gx_widget_fill_color_set(&app_setting_list_scrollbar, GX_COLOR_ID_HONBOW_DARK_GRAY,
								 GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY);
		extern VOID custom_scrollbar_vertical_draw(GX_SCROLLBAR * scrollbar);
		gx_widget_draw_set(&app_setting_list_scrollbar, custom_scrollbar_vertical_draw);
	}
	gx_widget_event_process_set(parent, disp_setting_event_processing_function);

	app_setting_brightness_page_init(NULL);	 // disp setting child page 1
	app_setting_brightTime_page_init(NULL);	 // disp setting child page 2
	app_setting_wrist_raise_page_init(NULL); // disp setting child page 3
	app_setting_night_mode_page_init(NULL);	 // disp setting child page 4
	app_setting_eco_mode_page_init(NULL);	 // disp setting child page 5
}
