#include "app_today_window.h"
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
#include "guix_language_resources_custom.h"
#include "common_draw_until.h"
#include "custom_animation.h"
#include "app_breath_window.h"


GX_WIDGET set_breath_freq_window;
// static uint8_t hr = 0;
static GX_PROMPT title_prompt;
static GX_PROMPT freq_prompt;
static breath_icon_Widget_T confirm_widget;

static GX_WIDGET scroll_widget;

#define TIME_FRESH_TIMER_ID 2
static GX_VALUE gx_point_x_first;
extern GX_WINDOW_ROOT *root;

static GX_NUMERIC_SCROLL_WHEEL freq_wheel;

static Custom_title_icon_t title_icon = {
	.icon = GX_PIXELMAP_ID_APP_BREATH_BTN_BACK,
	.bg_color = GX_COLOR_ID_HONBOW_BLUE,
};

UINT app_set_breath_freq_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	INT row;
	switch (event_ptr->gx_event_type) {	
	case GX_SIGNAL(2, GX_EVENT_CLICKED):
		gx_scroll_wheel_selected_get(&freq_wheel, &row);
		breath_data.breath_freq = row + 4;
		// Gui_close_spo2();
		jump_breath_page(&breath_main_page, &set_breath_freq_window, SCREEN_SLIDE_RIGHT);
	break;
	// case GX_EVENT_KEY_UP:
	// 	if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
	// 		windows_mgr_page_jump((GX_WINDOW *)&app_spo2_window, NULL, WINDOWS_OP_BACK);		
	// 		// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);			
	// 		return 0;
	// 	}
	break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}		
	
	default:
		break;
	}
	return gx_widget_event_process(window, event_ptr);
}

static const GX_CHAR *get_language_result_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_ENGLISH:
		return "Frequency";
	default:
		return "Frequency";
	}
}

static VOID set_breath_freq_child_widget_create(GX_WIDGET *widget)
{
	GX_RECTANGLE childsize;
	UINT status;	
	GX_STRING string;

	gx_utility_rectangle_define(&childsize, 12, 58, 308, 283);
	gx_widget_create(&scroll_widget, "scroll_widget", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
	gx_widget_fill_color_set(&scroll_widget, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
	// _gxe_widget_event_process_set(&greate_icon.widget, breath_child_widget_common_event);
	// gx_widget_draw_set(&greate_icon.widget, breath_simple_icon_widget_draw);

	status = gx_numeric_scroll_wheel_create(&freq_wheel, "freq_wheel", &scroll_widget, 4, 10, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_TEXT_CENTER, GX_ID_NONE, &childsize);
	gx_scroll_wheel_total_rows_set(&freq_wheel, 7);

	gx_widget_fill_color_set(&freq_wheel, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);
	gx_text_scroll_wheel_font_set(&freq_wheel, GX_FONT_ID_SIZE_60, GX_FONT_ID_SIZE_60);
	gx_text_scroll_wheel_text_color_set(&freq_wheel, GX_COLOR_ID_DISABLED_FILL, GX_COLOR_ID_HONBOW_WHITE,
										GX_COLOR_ID_HONBOW_WHITE);
	gx_scroll_wheel_row_height_set(&freq_wheel, 65);

	gx_utility_rectangle_define(&childsize, 192, 163, 243, 193);
	gx_prompt_create(&freq_prompt, NULL, widget, 0,
					GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
					0, &childsize);
	gx_prompt_text_color_set(&freq_prompt, GX_COLOR_ID_HONBOW_WHITE, 
		GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
	gx_prompt_font_set(&freq_prompt, GX_FONT_ID_SIZE_30);
	string.gx_string_ptr = "s";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&freq_prompt, &string);

	gx_scroll_wheel_gradient_alpha_set(&freq_wheel, 200, 0);
	gx_utility_rectangle_define(&childsize, 12, 283, 308, 348);
	gx_widget_create(&confirm_widget.widget, "confirm_widget", widget, 
		GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 2, &childsize);
	gx_widget_fill_color_set(&confirm_widget.widget, GX_COLOR_ID_HONBOW_BLACK,
		GX_COLOR_ID_HONBOW_BLACK,GX_COLOR_ID_HONBOW_BLACK);
	gx_widget_event_process_set(&confirm_widget.widget, breath_child_widget_common_event);
	gx_widget_draw_set(&confirm_widget.widget, breath_confirm_rectangle_button_draw);
	confirm_widget.icon = GX_PIXELMAP_ID_APP_BREATH_BTN_L_OK;	
	confirm_widget.bg_color = GX_COLOR_ID_HONBOW_BLUE;
}

void app_set_breath_freq_window_init(GX_WINDOW *window)
{
	GX_RECTANGLE childsize;
	GX_BOOL result;
	gx_widget_created_test(&set_breath_freq_window, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 319, 359);
		gx_widget_create(&set_breath_freq_window, "set_breath_freq_window", window, 
						GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
						&childsize);
		gx_widget_event_process_set(&set_breath_freq_window, app_set_breath_freq_window_event);
		GX_WIDGET *parent = &set_breath_freq_window;
		custom_window_title_create(parent, &title_icon, &title_prompt, NULL);
		GX_STRING string;
		string.gx_string_ptr = get_language_result_title();
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&title_prompt, &string);		

		set_breath_freq_child_widget_create(parent);
	}
	
}

