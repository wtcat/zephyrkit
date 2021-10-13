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
#include "app_today_window.h"
#include "custom_animation.h"
#include "windows_manager.h"

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

GX_WIDGET sleep_data_widget;
static GX_PROMPT sleep_prompt;
static GX_PROMPT time_prompt;
extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];
static GX_VERTICAL_LIST sleep_data_list;
extern APP_TODAY_WINDOW_CONTROL_BLOCK app_today_window;

#define SLEEP_DATA_LIST_NUM  7
typedef struct SLEEP_DATA_ROW_STRUCT {
	GX_WIDGET widget;	
	UCHAR id;
} SLEEP_DATA_LIST_ROW_T;
SLEEP_DATA_LIST_ROW_T sleep_data_list_row_memory[SLEEP_DATA_LIST_NUM];

typedef struct SLEEP_TOTAL_ICON_STRUCT {
	GX_WIDGET child;
	GX_RESOURCE_ID icon;
	GX_PROMPT desc[4];
}SLEEP_DATA_ICON_WIDGET_T;
SLEEP_DATA_ICON_WIDGET_T child_total_widget;

typedef struct DETAIL_DATA_BAR_WIDGET_STRUCT {
	GX_WIDGET widget;
	GX_PROMPT desc[2];
	SLEEP_DATA_STRUCT_DEF *sleep_data;
}DETAIL_DATA_BAR_WIDGET_T;
DETAIL_DATA_BAR_WIDGET_T detail_data_bar_memory;

typedef struct SLEEP_SEGMENT_DATA_WIDGET_STRUCT {
	GX_PROMPT desc[5];
}SLEEP_SEGMENT_DATA_WIDGET_T;
SLEEP_SEGMENT_DATA_WIDGET_T segment_data_widget_memory[4];

typedef struct SLEEP_MULTI_LINE_WIDGET_STRUCT {
	GX_WIDGET widget;
	GX_MULTI_LINE_TEXT_INPUT multi_line;
	CHAR multi_line_buf[100];
}SLEEP_MULTI_LINE_WIDGET_T;
SLEEP_MULTI_LINE_WIDGET_T multi_line_memory;

SLEEP_NO_RECORD_WIDGET_T no_record_widget_memory;

static CHAR total_hour_buf[10];
static CHAR total_min_buf[5];
static CHAR start_buf[10];
static CHAR end_buf[10];
static CHAR seg_hour[4][10];
static CHAR seg_min[4][10];

static Custom_title_icon_t icon = {
	.bg_color = GX_COLOR_ID_HONBOW_BLUE,
	.icon = GX_PIXELMAP_ID_BTN_BACK,
};

VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
static const GX_CHAR *lauguage_en_sport_summary_element[9] = { "Light",
					"Deep", "Awake", "REM"};

static const GX_CHAR **sport_today_summary_string[] = {
	[LANGUAGE_ENGLISH] = lauguage_en_sport_summary_element
};

static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return sport_today_summary_string[id];
}

void app_sleep_segment_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 3
UINT app_sleep_data_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {		
		gx_system_timer_start(window, TIME_FRESH_TIMER_ID, 50, 50);		
	} 
	break;
	case GX_EVENT_HIDE: {
		gx_system_timer_stop(window, TIME_FRESH_TIMER_ID);
	} 
	break;
	case GX_EVENT_PEN_DOWN: {
		gx_point_x_first = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		break;
	}	
	case GX_EVENT_PEN_UP: {
	 	delta_x = (event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);

		if (delta_x <= -50) {
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);
			windows_mgr_page_jump((GX_WINDOW *)&app_today_window, NULL, WINDOWS_OP_BACK);
			gx_widget_delete(&sleep_data_widget);
			gx_widget_delete(&today_main_page);
		}
		if (delta_x >= 50) {
			jump_today_page(&today_main_page, &sleep_data_widget, SCREEN_SLIDE_RIGHT);
			gx_widget_delete(&sleep_data_widget);
		}
		break;
	}
	case GX_EVENT_KEY_DOWN:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			return 0;
		}
		break;
	case GX_EVENT_KEY_UP:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);
			windows_mgr_page_jump((GX_WINDOW *)&app_today_window, NULL, WINDOWS_OP_BACK);
			gx_widget_delete(&sleep_data_widget);
			gx_widget_delete(&today_main_page); 
			return 0;
		}
		break;
	case GX_EVENT_TIMER: {
		if (event_ptr->gx_event_payload.gx_event_timer_id == TIME_FRESH_TIMER_ID) {
			struct timespec ts;
			GX_STRING string;
			clock_gettime(CLOCK_REALTIME, &ts);
			time_t now = ts.tv_sec;
			struct tm *tm_now = gmtime(&now);
			snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
			time_prompt_buff[7] = 0;
			string.gx_string_ptr = time_prompt_buff;
			string.gx_string_length = strlen(string.gx_string_ptr);
			gx_prompt_text_set_ext(&time_prompt, &string);			
			return 0;
		}
	} break;
	default:
		break;
	}
	return gx_widget_event_process(window, event_ptr);
}

static const GX_CHAR *get_language_today_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_ENGLISH:
		return "Sleep";
	default:
		return "Sleep";
	}
}

void single_icon_widget_draw(GX_WIDGET *widget)
{
	SLEEP_DATA_ICON_WIDGET_T *row = CONTAINER_OF(widget, SLEEP_DATA_ICON_WIDGET_T, child);
	GX_RECTANGLE fillrect;
	INT xpos;
	INT ypos;

	if (row->icon) {
		gx_widget_client_get(widget, 0, &fillrect);
		gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
		gx_canvas_rectangle_draw(&fillrect);
		GX_PIXELMAP *map = NULL;

		gx_context_pixelmap_get(row->icon, &map);
		if (map) {		
			xpos = widget->gx_widget_size.gx_rectangle_left;
			ypos = widget->gx_widget_size.gx_rectangle_top;
			gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
		}
	}	
}

void sleep_data_detail_bar_draw(GX_WIDGET *widget)
{
	DETAIL_DATA_BAR_WIDGET_T *row = CONTAINER_OF(widget, DETAIL_DATA_BAR_WIDGET_T, widget);
	struct Sleep_segment_Node *seqment_data = row->sleep_data->seg_data->next;
	GX_RECTANGLE fillrect;
	GX_COLOR color;
	uint8_t end_min = 0, end_hour = 0, end_day = 0, end_mon = 0;
	uint8_t start_min = 0, start_hour = 0, start_day = 0, start_mon = 0;
	uint32_t diff_min = 0;
	INT i = 0;
	INT x_pos = widget->gx_widget_size.gx_rectangle_left;
	int brush_width = 0;
	INT half_line_width = ((widget->gx_widget_size.gx_rectangle_bottom - \
	widget->gx_widget_size.gx_rectangle_top - 1) >> 1) + 1;

	gx_widget_client_get(widget, 0, &fillrect);
	gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
					GX_COLOR_ID_HONBOW_BLACK, 
					GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
	gx_canvas_rectangle_draw(&fillrect);

	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);
	gx_context_color_get(GX_COLOR_ID_HONBOW_BLACK, &color);
	gx_brush_define(&current_context->gx_draw_context_brush, 
				color, color, GX_BRUSH_ALIAS);
	gx_context_brush_width_set(56);
	gx_canvas_line_draw(
			widget->gx_widget_size.gx_rectangle_left,
			widget->gx_widget_size.gx_rectangle_top + half_line_width,
			widget->gx_widget_size.gx_rectangle_right,
			widget->gx_widget_size.gx_rectangle_top + half_line_width);

	for(i = 0; i < row->sleep_data->segment_num; i++) {
		end_min = seqment_data->data.end.min & 0xff;
		end_hour = seqment_data->data.end.min & 0xff;
		end_day = seqment_data->data.end.min & 0xff;
		end_mon = seqment_data->data.end.min & 0xff;

		start_min = seqment_data->data.start.min & 0xff;
		start_hour = seqment_data->data.start.hour & 0xff;
		start_day = seqment_data->data.start.day & 0xff;
		start_mon = seqment_data->data.start.mon & 0xff;	

		if (end_day == start_day) {
			diff_min = (end_hour * 60 + end_min) - (start_hour * 60 + start_min);
		} else {
			diff_min = ((end_hour + 24) * 60 + end_min) - 
				((start_hour) * 60 + start_min); 
		}

		brush_width = (diff_min * 296)/row->sleep_data->total_time;
		
		switch (seqment_data->data.type) {
			case LINGHT_SLEEP:
				gx_context_color_get(GX_COLOR_ID_HONBOW_LIGHT_SLEEP_BLUE, &color);
			break;
			case DEEP_SLEEP:
				gx_context_color_get(GX_COLOR_ID_HONBOW_DEEP_SLEEP_BLUE, &color);
			break;
			case REM:
				gx_context_color_get(GX_COLOR_ID_HONBOW_REM_SLEEP_BULE, &color);
			break;
			case WAKE:
				gx_context_color_get(GX_COLOR_ID_HONBOW_WHITE, &color);
			break;
			default:
			break;
		}
		seqment_data = seqment_data->next;
		gx_brush_define(&current_context->gx_draw_context_brush, 
				color, color, GX_BRUSH_ALIAS);
		gx_context_brush_width_set(brush_width);		
		gx_canvas_line_draw(x_pos + (brush_width>>1), 
						widget->gx_widget_size.gx_rectangle_top,
						x_pos + (brush_width>>1),
						widget->gx_widget_size.gx_rectangle_bottom);
		x_pos += brush_width;
	}	
}

void sleep_total_icon_widget_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, 
	INT index, SLEEP_DATA_STRUCT_DEF *sleep_data)
{
	SLEEP_DATA_LIST_ROW_T *row = (SLEEP_DATA_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 58, 308, 178);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
			index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		SLEEP_DATA_ICON_WIDGET_T *icon_widget = &child_total_widget;
		icon_widget->icon = GX_PIXELMAP_ID_IC_MOON;
		gx_utility_rectangle_define(&childsize, 135, 58, 185, 108);
		gx_widget_create(&icon_widget->child, "icon_widget", 
						&row->widget, 
						GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
						GX_ID_NONE, &childsize);
		gx_widget_fill_color_set(&icon_widget->child, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_draw_set(&icon_widget->child, single_icon_widget_draw);

		snprintf(total_hour_buf, 10, "%d", sleep_data->total_time/60);
		gx_utility_rectangle_define (&childsize, 
					82, 
					120, 
					82 + 27 * strlen(total_hour_buf), 
					166);
		gx_prompt_create(&icon_widget->desc[0], NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&icon_widget->desc[0], GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&icon_widget->desc[0], GX_FONT_ID_SIZE_46);		
		string.gx_string_ptr = total_hour_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&icon_widget->desc[0], &string);	

		gx_utility_rectangle_define (&childsize, 
					82 + 27 * strlen(total_hour_buf) + 2, 
					134, 
					82 + 27 * strlen(total_hour_buf) + 2 + 17, 
					164);
		gx_prompt_create(&icon_widget->desc[1], NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&icon_widget->desc[1], GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&icon_widget->desc[1], GX_FONT_ID_SIZE_30);
		string.gx_string_ptr = "h";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&icon_widget->desc[1], &string);	

		snprintf(total_min_buf, 5, "%d", sleep_data->total_time%60);
		gx_utility_rectangle_define (&childsize, 
					82 + 27 * strlen(total_hour_buf) + 2 + 17 + 2, 
					120, 
					82 + 27 * (strlen(total_hour_buf) + strlen(total_min_buf)) + 2 + 17 + 2, 
					166);
		gx_prompt_create(&icon_widget->desc[2], NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&icon_widget->desc[2], GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);	
		gx_prompt_font_set(&icon_widget->desc[2], GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = total_min_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&icon_widget->desc[2], &string);

		gx_utility_rectangle_define (&childsize, 
					82 + 27 * (strlen(total_hour_buf) + strlen(total_min_buf)) + 2 + 17 + 2, 
					134, 
					308,
					164);
		gx_prompt_create(&icon_widget->desc[3], NULL, &row->widget, 0,
			GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
			0, &childsize);
		gx_prompt_text_color_set(&icon_widget->desc[3], GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&icon_widget->desc[3], GX_FONT_ID_SIZE_30);
		string.gx_string_ptr = "min";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&icon_widget->desc[3], &string);	
	}
}

void sleep_detail_data_bar_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, 
	INT index, SLEEP_DATA_STRUCT_DEF *sleep_data)
{
	SLEEP_DATA_LIST_ROW_T *row = (SLEEP_DATA_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 178, 308, 292);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
			index+1, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

		DETAIL_DATA_BAR_WIDGET_T *detail_data_widget = &detail_data_bar_memory;
		detail_data_widget->sleep_data = sleep_data;

		gx_utility_rectangle_define(&childsize, 12, 178, 308, 234);
		gx_widget_create(&detail_data_widget->widget, "detail_data_widget", &row->widget,
							GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_fill_color_set(&detail_data_widget->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		gx_widget_draw_set(&detail_data_widget->widget, sleep_data_detail_bar_draw);

		snprintf(start_buf, 10, "%02d:%02d%s", sleep_data->start_time.hour % 12, 
							sleep_data->start_time.min,
							sleep_data->start_time.hour > 12 ? "PM" : "AM");
		gx_utility_rectangle_define (&childsize, 12, 240, 118, 266);
		gx_prompt_create(&detail_data_widget->desc[0], NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&detail_data_widget->desc[0], GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&detail_data_widget->desc[0], GX_FONT_ID_SIZE_26);		
		string.gx_string_ptr = start_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&detail_data_widget->desc[0], &string);	

		snprintf(end_buf, 10, "%02d:%02d%s", sleep_data->end_time.hour % 12, 
							sleep_data->end_time.min,
							sleep_data->end_time.hour > 12 ? "PM" : "AM");
		gx_utility_rectangle_define (&childsize,202, 240, 308, 266);
		gx_prompt_create(&detail_data_widget->desc[1], NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&detail_data_widget->desc[1], GX_COLOR_ID_HONBOW_DARK_BLACK,
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);	
		gx_prompt_font_set(&detail_data_widget->desc[1], GX_FONT_ID_SIZE_26);
		string.gx_string_ptr = end_buf;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&detail_data_widget->desc[1], &string);
	}
}

void sleep_list_light_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, 
	INT index, 	SLEEP_DATA_STRUCT_DEF *sleep_data)
{
	SLEEP_DATA_LIST_ROW_T *row = (SLEEP_DATA_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	static GX_RESOURCE_ID brush_color;
	INT i = index -2;
	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 292 + (i * 112), 308, 404 + (i * 112));
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
			index, &childsize);
		gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
		 						GX_COLOR_ID_HONBOW_BLACK);

		SLEEP_SEGMENT_DATA_WIDGET_T *light_widget = &segment_data_widget_memory[i];

		switch (i) {
			case 0:
				snprintf(seg_hour[i], 10, "%d", sleep_data->light/60);
				snprintf(seg_min[i], 10, "%02d", sleep_data->light%60);
				brush_color = GX_COLOR_ID_HONBOW_LIGHT_SLEEP_BLUE;
			break;
			case 1:
				snprintf(seg_hour[i], 10, "%d", sleep_data->deep_sleep/60);
				snprintf(seg_min[i], 10, "%02d", sleep_data->deep_sleep%60);
				brush_color = GX_COLOR_ID_HONBOW_DEEP_SLEEP_BLUE;
			break;
			case 2:
				snprintf(seg_hour[i], 10, "%d", sleep_data->awake/60);
				snprintf(seg_min[i], 10, "%02d", sleep_data->awake%60);
				brush_color = GX_COLOR_ID_HONBOW_WHITE;
			break;
			case 3:
				snprintf(seg_hour[i], 10, "%d", sleep_data->rem/60);
				snprintf(seg_min[i], 10, "%02d", sleep_data->rem%60);
				brush_color = GX_COLOR_ID_HONBOW_REM_SLEEP_BULE;
			break;
			default :
			break;
		}
		gx_utility_rectangle_define(&childsize, 12, 292 + (i * 112), 308, 322 + (i * 112));
		gx_prompt_create(&light_widget->desc[0], NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&light_widget->desc[0], GX_COLOR_ID_HONBOW_WHITE, 
			GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
		gx_prompt_font_set(&light_widget->desc[0], GX_FONT_ID_SIZE_30);			
		app_sleep_segment_list_prompt_get(i, &string);
		gx_prompt_text_set_ext(&light_widget->desc[0], &string);
		
		gx_utility_rectangle_define(&childsize, 12, 
					332 + (i * 112), 
					12 + 27 * strlen(seg_hour[i]), 
					378 + (i * 112));
		gx_prompt_create(&light_widget->desc[1], NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&light_widget->desc[1], brush_color, brush_color,
									brush_color);
		gx_prompt_font_set(&light_widget->desc[1], GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = seg_hour[i];
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&light_widget->desc[1], &string);	

		gx_utility_rectangle_define(&childsize, 
					14 + 27 * strlen(seg_hour[i]), 
					346 + (i * 112), 
					14 + 27 * strlen(seg_hour[i]) + 17, 
					376 + (i * 112));
		gx_prompt_create(&light_widget->desc[2], NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&light_widget->desc[2],GX_COLOR_ID_HONBOW_DARK_BLACK, 
			GX_COLOR_ID_HONBOW_DARK_BLACK, GX_COLOR_ID_HONBOW_DARK_BLACK);
		gx_prompt_font_set(&light_widget->desc[2], GX_FONT_ID_SIZE_30);
		string.gx_string_ptr = "h";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&light_widget->desc[2], &string);	

		gx_utility_rectangle_define(&childsize, 
					16 + 27 * strlen(seg_hour[i]) + 17, 
					332 + (i * 112), 
					16 + 27 * (strlen(seg_hour[i]) + strlen(seg_min[i])) + 17, 
					378 + (i * 112));
		gx_prompt_create(&light_widget->desc[3], NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
						0, &childsize);
		gx_prompt_text_color_set(&light_widget->desc[3], brush_color, brush_color, brush_color);
		gx_prompt_font_set(&light_widget->desc[3], GX_FONT_ID_SIZE_46);
		string.gx_string_ptr = seg_min[i];
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&light_widget->desc[3], &string);

		gx_utility_rectangle_define(&childsize, 
					18 + 27 * (strlen(seg_hour[i]) + strlen(seg_min[i])) + 17, 
					346 + (i * 112), 308, 376 + (i * 112));
		gx_prompt_create(&light_widget->desc[4], NULL, &row->widget, 0,
			GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT | GX_STYLE_BORDER_NONE, 
			0, &childsize);
		gx_prompt_text_color_set(&light_widget->desc[4], 
					GX_COLOR_ID_HONBOW_DARK_BLACK, 
					GX_COLOR_ID_HONBOW_DARK_BLACK,
					GX_COLOR_ID_HONBOW_DARK_BLACK);
		gx_prompt_font_set(&light_widget->desc[4], GX_FONT_ID_SIZE_30);
		string.gx_string_ptr = "min";
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&light_widget->desc[4], &string);
	}
}

static VOID get_sleep_data_info_string(GX_STRING *string)
{
	string->gx_string_ptr = "For more detailed data,please go to the app to view.";
	string->gx_string_length = strlen(string->gx_string_ptr);
}

void sleep_list_mult_info_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, 
	INT index, SLEEP_DATA_STRUCT_DEF *sleep_data)
{
	SLEEP_DATA_LIST_ROW_T *row = (SLEEP_DATA_LIST_ROW_T *)widget;
	GX_BOOL result;
	GX_RECTANGLE childsize;
	GX_STRING string;
	gx_widget_created_test(&row->widget, &result);

	if (!result) {
		gx_utility_rectangle_define(&childsize, 12, 628, 308, 738);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
			index, &childsize);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

		SLEEP_MULTI_LINE_WIDGET_T *multi_line_widget = &multi_line_memory;
		gx_utility_rectangle_define(&childsize, 12, 628, 308, 732);
		gx_widget_create(&multi_line_widget->widget, NULL, &row->widget, 
			GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childsize);
		gx_widget_fill_color_set(&multi_line_widget->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		UINT status;	
		status = gx_multi_line_text_input_create(&multi_line_widget->multi_line, "sleep_data_multi_text", 
					&multi_line_widget->widget, multi_line_widget->multi_line_buf, 100, 
					GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE|GX_STYLE_TEXT_LEFT,
					GX_ID_NONE, &childsize);
		gx_widget_status_remove(&multi_line_widget->multi_line, GX_STATUS_ACCEPTS_FOCUS);

		if (status == GX_SUCCESS) {
			gx_multi_line_text_view_font_set(
					(GX_MULTI_LINE_TEXT_VIEW *)&multi_line_widget->multi_line, GX_FONT_ID_SIZE_26);
			gx_multi_line_text_input_fill_color_set(
				&multi_line_widget->multi_line, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
			gx_multi_line_text_input_text_color_set(
				&multi_line_widget->multi_line, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, 
				GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
			gx_multi_line_text_view_whitespace_set(
				(GX_MULTI_LINE_TEXT_VIEW *)&multi_line_widget->multi_line, 0);
			gx_multi_line_text_view_line_space_set(
				(GX_MULTI_LINE_TEXT_VIEW *)&multi_line_widget->multi_line, 0);
			get_sleep_data_info_string(&string);
			gx_multi_line_text_input_text_set_ext(
				&multi_line_widget->multi_line, &string);
		}		
	}
}

void sleep_data_list_elempent_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index, SLEEP_DATA_STRUCT_DEF *sleep_data)
{
	GX_BOOL result;
	SLEEP_DATA_LIST_ROW_T *row = CONTAINER_OF(widget, SLEEP_DATA_LIST_ROW_T, widget);
	gx_widget_created_test(&row->widget, &result);
	if(!result) {
		switch (index) {
		case 0:
			sleep_total_icon_widget_create(list, widget, index, sleep_data);
		break;
		case 1:
			sleep_detail_data_bar_create(list, widget, index, sleep_data);
		break;
		case 2:
		case 3:
		case 4:
		case 5:
			sleep_list_light_create(list, widget, index, sleep_data);
		break;
		case 6:
			sleep_list_mult_info_create(list, widget, index, sleep_data);
		break;	
		default:
		break;
		}
	}
}

static VOID sleep_data_list_child_creat(GX_VERTICAL_LIST *list, SLEEP_DATA_STRUCT_DEF *sleep_data)
{
	INT count;
	for (count = 0; count < 7; count++) {
		sleep_data_list_elempent_create(list, 
			(GX_WIDGET *)&sleep_data_list_row_memory[count], count, sleep_data);
	}
}

void sleep_data_list_create(GX_WIDGET *widget, SLEEP_DATA_STRUCT_DEF *sleep_data)
{
	GX_RECTANGLE child_size;
	gx_utility_rectangle_define(&child_size, 12, 58, 308, 359);
	gx_vertical_list_create(&sleep_data_list, NULL, widget, 6, NULL,
		GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child_size);
	gx_widget_fill_color_set(&sleep_data_list, GX_COLOR_ID_HONBOW_BLACK, 
		GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
	sleep_data_list_child_creat(&sleep_data_list, sleep_data);
}

struct Sleep_segment_Node Seq_data;
struct Sleep_segment_Node Node_data[10];
void init_sleep_seg_data(struct Sleep_segment_Node *Head, INT index)
{
	int i = 0;
	Head->next = &Node_data[0];
	int temp = 0;
	for (i = 0; i < 10; i++)
	{
		Node_data[i].data.type = i % 3 + 1;
		if (i == 9)	{
			Node_data[i].data.type = 12;
			Node_data[i].next = NULL;
		}else {
			Node_data[i].next = &Node_data[i+1];
		}
		switch(Node_data[i].data.type)
		{
			case LINGHT_SLEEP :
				temp = 4;  //12/3
			break;
			case DEEP_SLEEP:
				temp = 14; //42/3
			break;
			case WAKE:
				temp = 3; //9/3
			break;
			case REM:
				temp = 15;
			break;
			default:
			break;
		}
		Node_data[i].data.start.mon = 5;
		Node_data[i].data.start.day = 11;
		Node_data[i].data.start.hour = 1;
		Node_data[i].data.start.min = 1;

		Node_data[i].data.end.mon = 5;
		Node_data[i].data.end.day = 11;
		Node_data[i].data.end.hour = 1;
		Node_data[i].data.end.min = (1 + temp * index);;
	
	}
}

void init_sleep_data_test(int index, SLEEP_DATA_STRUCT_DEF *data)
{
	init_sleep_seg_data(&Seq_data, index);
	data->awake = index * 9;
	data->deep_sleep = index * 42;
	data->light = index * 12;
	data->rem = index * 15;
	data->segment_num = 10;
	data->total_time = index * (9 + 42 + 12 + 15);
	if (10 == index) {
		data->total_time = 0;
	}	
	data->seg_data = &Seq_data;
	data->start_time.mon = 5;
	data->start_time.day = 11;
	data->start_time.hour = 21;
	data->start_time.min = 33;
	data->end_time.mon = 4;
	data->end_time.day = 21;
	data->end_time.hour = ((data->start_time.hour * 60 +  \
			data->start_time.min + data->total_time)/60)%24;
	data->end_time.min = (data->start_time.min + data->total_time) % 60;	
}

void record_icon_widget_draw(GX_WIDGET *widget)
{
	SLEEP_NO_RECORD_WIDGET_T *row = CONTAINER_OF(widget, SLEEP_NO_RECORD_WIDGET_T, child);
	GX_RECTANGLE fillrect;
	INT xpos;
	INT ypos;
	if (row->icon) {
		gx_widget_client_get(widget, 0, &fillrect);
		gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, 
				GX_COLOR_ID_HONBOW_BLACK, GX_BRUSH_SOLID_FILL);
		gx_canvas_rectangle_draw(&fillrect);

		GX_PIXELMAP *map = NULL;
		gx_context_pixelmap_get(row->icon, &map);
		if (map) {		
			xpos = widget->gx_widget_size.gx_rectangle_left;
			ypos = widget->gx_widget_size.gx_rectangle_top;
			gx_canvas_pixelmap_draw((GX_VALUE)xpos, (GX_VALUE)ypos, map);
		}
	}	
}

void app_sleep_no_record_widget(GX_WIDGET *widget)
{
	GX_RECTANGLE childsize;
	GX_STRING string;
	SLEEP_NO_RECORD_WIDGET_T *row = &no_record_widget_memory;
	gx_utility_rectangle_define(&childsize, 0, 58, 319, 359);
	gx_widget_create(&row->widget, NULL, widget, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
		GX_ID_NONE, &childsize);
	gx_widget_event_process_set(&row->widget, today_child_widget_common_event);
	gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
							GX_COLOR_ID_HONBOW_BLACK);
	
	gx_utility_rectangle_define(&childsize, 120, 120, 200, 200);
	gx_widget_create(&row->child, NULL, &row->widget, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
		GX_ID_NONE, &childsize);
	gx_widget_event_process_set(&row->child, today_child_widget_common_event);
	gx_widget_fill_color_set(&row->child, GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
							GX_COLOR_ID_HONBOW_BLACK);
	row->icon = GX_PIXELMAP_ID_IC_MOON_NO_GRAY;
	gx_widget_draw_set(&row->child,  record_icon_widget_draw);

	gx_utility_rectangle_define(&childsize, 12, 206, 308, 236);
	gx_prompt_create(&row->decs, NULL, &row->widget, 0,
						GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 
						0, &childsize);
	gx_prompt_text_color_set(&row->decs, 
				GX_COLOR_ID_HONBOW_DARK_GRAY, 
				GX_COLOR_ID_HONBOW_DARK_GRAY,
				GX_COLOR_ID_HONBOW_DARK_GRAY);
	gx_prompt_font_set(&row->decs, GX_FONT_ID_SIZE_30);
	string.gx_string_ptr = "No record";
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&row->decs, &string);
}

void app_sleep_data_window_init(GX_WINDOW *window, SLEEP_DATA_STRUCT_DEF *sleep_data)
{
	GX_RECTANGLE win_size;
	GX_BOOL result;
	gx_widget_created_test(&sleep_data_widget, &result);
	if (!result)
	{
		gx_utility_rectangle_define(&win_size, 0, 0, 320 - 1, 359);
		gx_widget_create(&sleep_data_widget, "sleep_data_widget", window, 
			GX_STYLE_BORDER_NONE|GX_STYLE_ENABLED|GX_STYLE_TRANSPARENT, 
			0, &win_size);
		gx_widget_fill_color_set(&sleep_data_widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);	
		gx_widget_event_process_set(&sleep_data_widget, app_sleep_data_window_event);
		
		GX_WIDGET *parent = &sleep_data_widget;
		custom_window_title_create(parent, &icon, &sleep_prompt, &time_prompt);

		GX_STRING string;
		string.gx_string_ptr = get_language_today_title();
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&sleep_prompt, &string);
	
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		time_t now = ts.tv_sec;
		struct tm *tm_now = gmtime(&now);
		snprintf(time_prompt_buff, 8, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
		time_prompt_buff[7] = 0;
		string.gx_string_ptr = time_prompt_buff;
		string.gx_string_length = strlen(string.gx_string_ptr);
		gx_prompt_text_set_ext(&time_prompt, &string);

		if (sleep_data->total_time > 30) {
			sleep_data_list_create(parent, sleep_data);
		} else {
			app_sleep_no_record_widget(parent);
		}
	}	
}


