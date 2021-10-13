#include "gx_api.h"
#include "custom_window_title.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include <logging/log.h>
#include "app_setting.h"
#include "app_setting_disp_page.h"
#include "guix_language_resources_custom.h"
#include "custom_rounded_button.h"
#include "custom_animation.h"
#include "custom_vertical_scroll_limit.h"

LOG_MODULE_DECLARE(guix_log);
static GX_WIDGET curr_page;
static Custom_title_icon_t icon_title = {
	.bg_color = GX_COLOR_ID_HONBOW_GREEN,
	.icon = GX_PIXELMAP_ID_APP_SETTING_BTN_BACK,
};
static GX_PROMPT setting_prompt;
static GX_PROMPT time_prompt;
static char time_prompt_buff[8];

// page info
static GX_VERTICAL_LIST system_list;
static GX_SCROLLBAR system_list_scrollbar;
typedef struct SYSTEM_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	Custom_RoundedButton_TypeDef child;
	uint8_t id;
} SYSTEM_LIST_ROW;
#define SYSTEM_LIST_VISIBLE_ROWS 4
#define WIDGET_ID_ROUND_BUTTON 1
static SYSTEM_LIST_ROW system_list_row_memory[SYSTEM_LIST_VISIBLE_ROWS + 1];

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
static GX_WIDGET *system_setting_child[5] = {0};

extern GX_WINDOW_ROOT *root;
static const GX_CHAR *get_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_CHINESE:
		return "系统";
	case LANGUAGE_ENGLISH:
		return "System";
	default:
		return "System";
	}
}
static const GX_CHAR *lauguage_ch_menu_element[5] = {"重启", "关机", "恢复出厂", "监管信息", "关于设备"};
static const GX_CHAR *lauguage_en_menu_element[5] = {"Reboot", "Shutdown", "Factory Reset", "Supervision Info",
													 "About Device"};

static const GX_CHAR **menu_string[] = {
	[LANGUAGE_CHINESE] = lauguage_ch_menu_element, [LANGUAGE_ENGLISH] = lauguage_en_menu_element};
static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return menu_string[id];
}
static void system_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **menu_string = get_lauguage_string();
	prompt->gx_string_ptr = menu_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

static GX_WIDGET *disp_setting_menu_screen_list[] = {
	&curr_page,
	GX_NULL,
	GX_NULL,
};

void setting_system_page_children_reg(GX_WIDGET *dst, uint8_t id)
{
	system_setting_child[id] = dst;
}

void setting_system_page_children_return(GX_WIDGET *curr)
{
	disp_setting_menu_screen_list[1] = curr;
	custom_animation_start(disp_setting_menu_screen_list, (GX_WIDGET *)&app_setting_window, SCREEN_SLIDE_RIGHT);
}

static UINT setting_main_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(WIDGET_ID_ROUND_BUTTON, GX_EVENT_CLICKED): {
		SYSTEM_LIST_ROW *row = CONTAINER_OF(widget, SYSTEM_LIST_ROW, widget);
		LOG_INF("[%s]row %d clicked!", __FUNCTION__, row->id);
		if (system_setting_child[row->id]) {
			disp_setting_menu_screen_list[1] = system_setting_child[row->id];
			custom_animation_start(disp_setting_menu_screen_list, (GX_WIDGET *)&app_setting_window, SCREEN_SLIDE_LEFT);
		}
		return 0;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

static VOID system_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	SYSTEM_LIST_ROW *row = (SYSTEM_LIST_ROW *)widget;
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
		system_list_prompt_get(index, &string);
		custom_rounded_button_create(&row->child, WIDGET_ID_ROUND_BUTTON, &row->widget, &childsize,
									 GX_COLOR_ID_HONBOW_DARK_GRAY, GX_PIXELMAP_NULL, &string,
									 CUSTOM_ROUNDED_BUTTON_TEXT_LEFT);
		row->id = index;
	}
}

static VOID app_setting_list_children_create(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < SYSTEM_LIST_VISIBLE_ROWS + 1; count++) {
		system_list_row_create(list, (GX_WIDGET *)&system_list_row_memory[count], count);
	}
}

static VOID app_setting_list_callback(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	SYSTEM_LIST_ROW *row = CONTAINER_OF(widget, SYSTEM_LIST_ROW, widget);
	GX_STRING string;
	system_list_prompt_get(index, &string);
	gx_prompt_text_set_ext(&row->child.desc, &string);
	row->id = index;
}

static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
static UINT event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
	} break;
	case GX_EVENT_HIDE: {
	} break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}
	case GX_EVENT_PEN_UP: {
		delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x >= 50) {
			extern void jump_setting_main_page(GX_WIDGET * now);
			jump_setting_main_page(&curr_page);
		}
		break;
	}
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}
void app_setting_system_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_page, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	extern void setting_main_page_jump_reg(GX_WIDGET * dst, SETTING_CHILD_PAGE_T id);
	setting_main_page_jump_reg(&curr_page, SETTING_CHILD_PAGE_SYSTEM);

	GX_WIDGET *parent = &curr_page;
	custom_window_title_create(parent, &icon_title, &setting_prompt, NULL);

	GX_STRING string;
	string.gx_string_ptr = get_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&setting_prompt, &string);

	gx_widget_event_process_set(parent, event_processing_function);

	GX_BOOL result;
	gx_widget_created_test(&system_list, &result);
	if (!result) {
		GX_RECTANGLE child_size;
		gx_utility_rectangle_define(&child_size, 8, 60, 296 + 20 - 1, 359);
		gx_vertical_list_create(&system_list, NULL, parent, 5, app_setting_list_callback,
								GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child_size);
		gx_widget_fill_color_set(&system_list, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
								 GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_event_process_set(&system_list, custom_vertical_event_processing_function);

		app_setting_list_children_create(&system_list);

		gx_vertical_scrollbar_create(
			&system_list_scrollbar, NULL, &system_list, &app_disp_setting_list_scrollbar_properties,
			GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB | GX_SCROLLBAR_VERTICAL);
		gx_widget_fill_color_set(&system_list_scrollbar, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
								 GX_COLOR_ID_HONBOW_DARK_GRAY);
		extern VOID custom_scrollbar_vertical_draw(GX_SCROLLBAR * scrollbar);
		gx_widget_draw_set(&system_list_scrollbar, custom_scrollbar_vertical_draw);
	}

	void app_setting_system_reboot_page_init(GX_WINDOW * window);
	app_setting_system_reboot_page_init(NULL);
	void app_setting_system_shutdown_page_init(GX_WINDOW * window);
	app_setting_system_shutdown_page_init(NULL);

	void app_setting_system_factory_reset_page_init(GX_WINDOW * window);
	app_setting_system_factory_reset_page_init(NULL);
}
