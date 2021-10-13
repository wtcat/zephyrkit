#include "gx_api.h"
#include "home_window.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "custom_rounded_button.h"
#include "timer_widget.h"
#include "app_timer.h"
#include "windows_manager.h"

static GX_WIDGET curr_widget;
static TIMER_WIDGET_TYPE child1;
static TIMER_WIDGET_TYPE child2;
static TIMER_WIDGET_TYPE child3;
static TIMER_WIDGET_TYPE child4;
static Custom_RoundedButton_TypeDef child5;
#define BUTTON_ID_COUNTER_1 1
#define BUTTON_ID_COUNTER_2 2
#define BUTTON_ID_COUNTER_3 3
#define BUTTON_ID_COUNTER_4 4
#define BUTTON_ID_MORE 5

extern VOID custom_circle_pixelmap_button_draw(GX_PIXELMAP_BUTTON *button);

void timer_widget_create_helper(TIMER_WIDGET_TYPE *child, GX_RECTANGLE *rect, GX_WIDGET *parent)
{
	gx_pixelmap_button_create(&child->counter_down_widget, NULL, parent, GX_PIXELMAP_NULL, GX_PIXELMAP_NULL,
							  GX_PIXELMAP_NULL,
							  GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_VALIGN_CENTER | GX_STYLE_HALIGN_CENTER,
							  child->button_id, rect);
	gx_widget_fill_color_set(&child->counter_down_widget, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
							 GX_COLOR_ID_HONBOW_DARK_GRAY);
	gx_widget_draw_set(&child->counter_down_widget, custom_circle_pixelmap_button_draw);

	parent = (GX_WIDGET *)&child->counter_down_widget;
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, rect->gx_rectangle_left + 34, rect->gx_rectangle_top + 20,
								rect->gx_rectangle_left + 34 + 53 - 1, rect->gx_rectangle_top + 20 + 46 - 1);
	gx_prompt_create(&child->prompt_counter_value, GX_NULL, parent, GX_ID_NONE,
					 GX_STYLE_TRANSPARENT | GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TEXT_CENTER, GX_ID_NONE,
					 &childSize);
	gx_prompt_text_color_set(&child->prompt_counter_value, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&child->prompt_counter_value, GX_FONT_ID_SIZE_46);
	GX_STRING value;
	value.gx_string_ptr = child->counter_value;
	value.gx_string_length = strlen(value.gx_string_ptr);
	gx_prompt_text_set_ext(&child->prompt_counter_value, &value);

	gx_utility_rectangle_define(&childSize, rect->gx_rectangle_left + 34, rect->gx_rectangle_top + 74,
								rect->gx_rectangle_left + 34 + 52 - 1, rect->gx_rectangle_top + 74 + 26 - 1);
	gx_prompt_create(&child->prompt_unit, GX_NULL, parent, GX_ID_NONE,
					 GX_STYLE_TRANSPARENT | GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TEXT_CENTER, GX_ID_NONE,
					 &childSize);
	gx_prompt_text_color_set(&child->prompt_unit, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(&child->prompt_unit, GX_FONT_ID_SIZE_26);
	value.gx_string_ptr = child->unit_value;
	value.gx_string_length = strlen(value.gx_string_ptr);
	gx_prompt_text_set_ext(&child->prompt_unit, &value);
}

static UINT event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	if ((event_ptr->gx_event_type & 0x00ff) == GX_EVENT_CLICKED) {
		uint8_t key_id = (event_ptr->gx_event_type & 0xff00) >> 8;
		TIMER_PARA_TRANS_T para;
		switch (key_id) {
		case BUTTON_ID_COUNTER_1:
			para.min = 1;
			break;
		case BUTTON_ID_COUNTER_2:
			para.min = 3;
			break;
		case BUTTON_ID_COUNTER_3:
			para.min = 5;
			break;
		case BUTTON_ID_COUNTER_4:
			para.min = 15;
			break;
		case BUTTON_ID_MORE:
			app_timer_page_reset();
			windows_mgr_page_jump(&root_window.root_window_home_window, (GX_WINDOW *)&app_timer_window,
								  WINDOWS_OP_SWITCH_NEW);
			return 0;
		}
		para.hour = 0;
		para.sec = 0;
		para.tenth_of_second = 0;
		app_timer_main_page_show(GX_TRUE, (GX_WINDOW *)&root_window.root_window_home_window, NULL, para);
		return 0;
	}
	return gx_widget_event_process(widget, event_ptr);
}
void widget_countdown_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_widget, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);
	gx_widget_event_process_set(&curr_widget, event_processing_function);

	gx_utility_rectangle_define(&childSize, 27, 23, 27 + 120 - 1, 23 + 120 - 1);
	child1.button_id = BUTTON_ID_COUNTER_1;
	child1.counter_value = "1";
	child1.unit_value = "min";
	timer_widget_create_helper(&child1, &childSize, &curr_widget);

	gx_utility_rectangle_define(&childSize, 172, 23, 172 + 120 - 1, 23 + 120 - 1);
	child2.button_id = BUTTON_ID_COUNTER_2;
	child2.counter_value = "3";
	child2.unit_value = "min";
	timer_widget_create_helper(&child2, &childSize, &curr_widget);

	gx_utility_rectangle_define(&childSize, 27, 153, 27 + 120 - 1, 153 + 120 - 1);
	child3.button_id = BUTTON_ID_COUNTER_3;
	child3.counter_value = "5";
	child3.unit_value = "min";
	timer_widget_create_helper(&child3, &childSize, &curr_widget);

	gx_utility_rectangle_define(&childSize, 172, 153, 172 + 120 - 1, 153 + 120 - 1);
	child4.button_id = BUTTON_ID_COUNTER_4;
	child4.counter_value = "15";
	child4.unit_value = "min";
	timer_widget_create_helper(&child4, &childSize, &curr_widget);
	gx_prompt_text_color_set(&child4.prompt_counter_value, GX_COLOR_ID_HONBOW_ORANGE, GX_COLOR_ID_HONBOW_ORANGE,
							 GX_COLOR_ID_HONBOW_ORANGE);

	gx_utility_rectangle_define(&childSize, 12, 283, 12 + 295 - 1, 283 + 65 - 1);
	GX_STRING string;
	string.gx_string_ptr = "more";
	string.gx_string_length = strlen(string.gx_string_ptr);
	custom_rounded_button_create2(&child5, BUTTON_ID_MORE, &curr_widget, &childSize, GX_COLOR_ID_HONBOW_DARK_GRAY,
								  GX_PIXELMAP_NULL, &string, GX_FONT_ID_SIZE_30, CUSTOM_ROUNDED_BUTTON_TEXT_MIDDLE);

	home_widget_type_reg(&curr_widget, HOME_WIDGET_TYPE_COUNTER_DOWN);
}