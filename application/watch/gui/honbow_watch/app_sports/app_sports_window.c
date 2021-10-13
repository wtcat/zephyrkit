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
#include "app_outdoor_run_window.h"
#include "guix_language_resources_custom.h"
#include "windows_manager.h"
Sport_cfg_t gui_sport_cfg = {0};

extern UINT _gx_system_input_capture(GX_WIDGET *owner);
extern UINT _gx_system_input_release(GX_WIDGET *owner);

GX_WIDGET sport_main_page;
static GX_PROMPT sport_prompt;
static GX_PROMPT time_prompt;
extern GX_WINDOW_ROOT *root;
static char time_prompt_buff[8];
static GX_VERTICAL_LIST app_sport_list;
extern APP_SPORT_WINDOW_CONTROL_BLOCK app_sport_window;
typedef struct APP_LIST_ROW_STRUCT {
	GX_WIDGET widget;
	Custom_RoundedIconButton_TypeDef child;
} APP_SPORT_LIST_ROW;

GX_SCROLLBAR_APPEARANCE app_sport_list_scrollbar_properties = {
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

static GX_SCROLLBAR app_sport_list_scrollbar;
#define APP_SPORT_LIST_VISIBLE_ROWS 2
static APP_SPORT_LIST_ROW app_sport_list_row_memory[APP_SPORT_LIST_VISIBLE_ROWS + 1];
extern GX_WINDOW_ROOT *root;

static const GX_CHAR *lauguage_en_setting_element[14] = {
	"Outdoor Run", "Outdoor Walk", "Outdoor Bicycle", "Indoor Run",
	"Indoor Walk", "On Foot", "Indoor Bicycle", "Elliptical Machine",
	"Rowing machine", "Yoga", "Climbing", "Pool Swimming","Open Swimming", 
	"Other"};

static const GX_CHAR **setting_string[] = {
	//[LANGUAGE_CHINESE] = lauguage_ch_setting_element,
	[LANGUAGE_ENGLISH] = lauguage_en_setting_element};
static const GX_CHAR **get_lauguage_string(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	return setting_string[id];
}

VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);
static GX_VALUE gx_point_x_first;
static GX_VALUE delta_x;
#define TIME_FRESH_TIMER_ID 3
UINT app_sport_window_event(GX_WIDGET *window, GX_EVENT *event_ptr)
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
		delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - gx_point_x_first);
		if (delta_x >= 50) {
			// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&root_window.root_window_home_window);
			windows_mgr_page_jump((GX_WINDOW *)&app_sport_window, NULL, WINDOWS_OP_BACK);
			return 0;
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
			windows_mgr_page_jump((GX_WINDOW *)&app_sport_window, NULL, WINDOWS_OP_BACK);
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

static const GX_CHAR *get_language_sport_title(void)
{
	GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
	switch (id) {
	case LANGUAGE_ENGLISH:
		return "Sports";
	default:
		return "Sports";
	}
}

void app_sport_list_prompt_get(INT index, GX_STRING *prompt)
{
	const GX_CHAR **setting_string = get_lauguage_string();
	prompt->gx_string_ptr = setting_string[index];
	prompt->gx_string_length = strlen(prompt->gx_string_ptr);
}

static GX_RESOURCE_ID sport_icon[] = {
	GX_PIXELMAP_ID_APP_SPORT_IC_RUN,
	GX_PIXELMAP_ID_APP_SPORT_IC_WALK,
	GX_PIXELMAP_ID_APP_SPORT_IC__BICYCLE,
	GX_PIXELMAP_ID_APP_SPORT_IC_INDOOR_RUNNING,
	GX_PIXELMAP_ID_APP_SPORT_IC_INDOOR_WALKING_T,
	GX_PIXELMAP_ID_APP_SPORT_IC_ON_FOOT_T,
	GX_PIXELMAP_ID_APP_SPORT_IC_NDOOR_BICYCLE,
	GX_PIXELMAP_ID_APP_SPORT_IC_ELLIPTICAL_MACHINE,
	GX_PIXELMAP_ID_APP_SPORT_IC_ROWING_MACHINE,
	GX_PIXELMAP_ID_APP_SPORT_IC_YOGA,
	GX_PIXELMAP_ID_APP_SPORT_IC_MOUNTAIN_CLIMBING,
	GX_PIXELMAP_ID_APP_SPORT_IC_OUTDOOR_SWIMMING,
	GX_PIXELMAP_ID_APP_SPORT_IC_SWIMMING,
	GX_PIXELMAP_ID_APP_SPORT_IC_OTHER_SPORT
};

GX_WIDGET *sport_child[14] = {0};
void sport_main_page_jump_reg(GX_WIDGET *dst, SPORT_CHILD_PAGE_T id)
{
	sport_child[id] = dst;
}

GX_WIDGET *app_sport_menu_screen_list[] = {
	&sport_main_page,
	GX_NULL,
	GX_NULL,
};

#include "custom_animation.h"
void jump_sport_page(GX_WIDGET *next, GX_WIDGET *prev, uint8_t dir)
{
	app_sport_menu_screen_list[0] = prev;
	app_sport_menu_screen_list[1] = next;
	custom_animation_start(app_sport_menu_screen_list, 
		(GX_WIDGET *)&app_sport_window, dir);
}


UINT app_sport_list_widget_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	APP_SPORT_LIST_ROW *row = (APP_SPORT_LIST_ROW *)widget;
	switch (event_ptr->gx_event_type) {
	case GX_SIGNAL(1, GX_EVENT_CLICKED):		
		if (sport_child[row->child.id] != NULL) {		
			set_gps_connetting_titlle_prompt(row->child.id);
			app_sport_menu_screen_list[0] = &sport_main_page;
			app_sport_menu_screen_list[1] = sport_child[row->child.id];
			custom_animation_start(app_sport_menu_screen_list, 
				(GX_WIDGET *)&app_sport_window, SCREEN_SLIDE_LEFT);

			switch (row->child.id)
			{
			case OUTDOOR_RUN_CHILD_PAGE:
			case OUTDOOR_WALK_CHILD_PAGE:
			case ON_FOOT_CHILD_PAGE:
				gui_sport_cfg.mode = ACTIVITY_MODE_OUTDOOR_RUNNING;
			
			break;
			case OUTDOOR_BICYCLE_CHILD_PAGE:
				gui_sport_cfg.mode = ACTIVITY_MODE_OUTDOOR_BIKING;
			break;
			case INDOOR_RUN_CHILD_PAGE:
			case INDOOR_WALK_CHILD_PAGE:
				gui_sport_cfg.mode = ACTIVITY_MODE_TREADMILL;
			break;		
			default:
				break;
			}	
			gui_sport_cfg.goal_time = 0;
			gui_sport_cfg.goal_calorise = 0;
			gui_sport_cfg.goal_distance = 0;		
		}
		return 0;
		break;
	case GX_SIGNAL(2, GX_EVENT_CLICKED):		
		if (sport_child[row->child.id] != NULL) {		
			app_sport_menu_screen_list[0] = &sport_main_page;
			app_outdoor_run_window_init(NULL, row->child.id);			
			app_sport_menu_screen_list[1] = &outdoor_run_window;
			custom_animation_start(app_sport_menu_screen_list, 
				(GX_WIDGET *)&app_sport_window, SCREEN_SLIDE_LEFT);		
				
			switch (row->child.id)
			{
			case OUTDOOR_RUN_CHILD_PAGE:
			case OUTDOOR_WALK_CHILD_PAGE:
			case ON_FOOT_CHILD_PAGE:
				gui_sport_cfg.mode = ACTIVITY_MODE_OUTDOOR_RUNNING;
			
			break;
			case OUTDOOR_BICYCLE_CHILD_PAGE:
				gui_sport_cfg.mode = ACTIVITY_MODE_OUTDOOR_BIKING;
			break;
			case INDOOR_RUN_CHILD_PAGE:
			case INDOOR_WALK_CHILD_PAGE:
				gui_sport_cfg.mode = ACTIVITY_MODE_TREADMILL;
			break;		
			default:
				break;
			}	
		}
		return 0;
		break;
	default :
	break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

static VOID app_sport_list_callback(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	APP_SPORT_LIST_ROW *row = CONTAINER_OF(widget, APP_SPORT_LIST_ROW, widget);
	GX_STRING string;
	app_sport_list_prompt_get(index, &string);
	gx_prompt_text_set_ext(&row->child.desc, &string);
	row->child.icon0 = sport_icon[index];
	row->child.id = index;
}

void goal_setting_window_icon_draw_func(GX_WIDGET *widget)
{
	INT xpos;
	INT ypos;
	GX_RECTANGLE fillrect;
	Sport_goal_icon_t *goal_setting = CONTAINER_OF(widget, Sport_goal_icon_t, widget);
	gx_widget_client_get(widget, 0, &fillrect);
	GX_COLOR color = goal_setting->bg_color;

	if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
		color = GX_COLOR_ID_HONBOW_BLACK;
	}
	
	gx_context_brush_define(color, color, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
	gx_context_brush_width_set(1);
	gx_canvas_circle_draw(
		((fillrect.gx_rectangle_right - fillrect.gx_rectangle_left + 1) >> 1) + 
			fillrect.gx_rectangle_left+1,
		((fillrect.gx_rectangle_bottom - fillrect.gx_rectangle_top + 1) >> 1) + 
			fillrect.gx_rectangle_top+1, 29);	
	GX_PIXELMAP *map;
	gx_context_pixelmap_get(goal_setting->icon, &map);
	xpos = widget->gx_widget_size.gx_rectangle_right - 
		widget->gx_widget_size.gx_rectangle_left - map->gx_pixelmap_width + 1;
	xpos /= 2;
	xpos += widget->gx_widget_size.gx_rectangle_left;

	ypos = widget->gx_widget_size.gx_rectangle_bottom - 
		widget->gx_widget_size.gx_rectangle_top - map->gx_pixelmap_height + 1;
	ypos /= 2;
	ypos += widget->gx_widget_size.gx_rectangle_top;
	gx_canvas_pixelmap_draw(xpos, ypos, map);
}

UINT goal_setting_icon_widget_event(GX_WIDGET *widget, GX_EVENT *event_ptr)
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
			GX_RECTANGLE size;
			size.gx_rectangle_left = widget->gx_widget_size.gx_rectangle_left - 50;
			size.gx_rectangle_right = widget->gx_widget_size.gx_rectangle_right + 10;
			size.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_top - 10;
			size.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_bottom + 40;		
			if (gx_utility_rectangle_point_detect(&size,
				event_ptr->gx_event_payload.gx_event_pointdata)) {
				gx_widget_event_generate(widget, GX_EVENT_CLICKED, 0);
			}
		}	
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

void goal_setting_window_icon_create(GX_WIDGET *parent, Sport_goal_icon_t *icon)
{
	if(icon) {
		GX_RECTANGLE size;
		gx_utility_rectangle_define(&size, 226, 10, 286, 70);
		gx_widget_create(&icon->widget,  NULL, parent, 
			GX_STYLE_ENABLED|GX_STYLE_BORDER_NONE, 2, &size);
		gx_widget_event_process_set(&icon->widget, goal_setting_icon_widget_event);
		gx_widget_fill_color_set(&icon->widget, GX_COLOR_ID_HONBOW_DARK_GRAY, 
			GX_COLOR_ID_HONBOW_DARK_GRAY,GX_COLOR_ID_HONBOW_DARK_GRAY);
		gx_widget_draw_set(&icon->widget, goal_setting_window_icon_draw_func);
	}
}

static VOID app_sport_list_row_create(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
	GX_RECTANGLE childsize;
	APP_SPORT_LIST_ROW *row = (APP_SPORT_LIST_ROW *)widget;
	GX_BOOL result;

	gx_widget_created_test(&row->widget, &result);
	if (!result) {
		gx_utility_rectangle_define(&childsize, 0, 0, 299, 129);
		gx_widget_create(&row->widget, NULL, list, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, 
			GX_ID_NONE, &childsize);
		gx_widget_event_process_set(&row->widget, app_sport_list_widget_event);
		gx_widget_fill_color_set(&row->widget, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		row->child.goal_set.icon = GX_PIXELMAP_ID_APP_SPORT_BTN_AIMS;
		row->child.goal_set.bg_color = GX_COLOR_ID_HONBOW_DARK_BLACK;
		childsize.gx_rectangle_bottom -= 10;
		GX_STRING string;
		app_sport_list_prompt_get(index, &string);
		custom_rounded_button_align_create(&row->child, 1, &row->widget, 
			&childsize, GX_COLOR_ID_HONBOW_DARK_GRAY,
			sport_icon[index], GX_ID_NONE, &string, 20, 100);
		row->child.id = index;
		goal_setting_window_icon_create(&row->widget, &row->child.goal_set);
	}
}

static VOID app_sport_list_children_create(GX_VERTICAL_LIST *list)
{
	INT count;
	for (count = 0; count < APP_SPORT_LIST_VISIBLE_ROWS + 1; count++) {
		app_sport_list_row_create(list, (GX_WIDGET *)&app_sport_list_row_memory[count], count);
	}
}

void app_sport_window_init(GX_WINDOW *window)
{
	gx_widget_create(&sport_main_page, "sport_main_page", window, 
					GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
					&window->gx_widget_size);
	GX_WIDGET *parent = &sport_main_page;
	custom_window_title_create(parent, NULL, &sport_prompt, &time_prompt);

	GX_STRING string;
	string.gx_string_ptr = get_language_sport_title();
	string.gx_string_length = strlen(string.gx_string_ptr);
	gx_prompt_text_set_ext(&sport_prompt, &string);

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
	gx_widget_created_test(&app_sport_list, &result);
	if (!result) {
		GX_RECTANGLE child_size;
		gx_utility_rectangle_define(&child_size, 12, 68, 315, 359);
		gx_vertical_list_create(&app_sport_list, NULL, 
				parent, 14, app_sport_list_callback,
				GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, 
				GX_ID_NONE, &child_size);
		gx_widget_fill_color_set(&app_sport_list, GX_COLOR_ID_HONBOW_BLACK, 
			GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
		app_sport_list_children_create(&app_sport_list);
		gx_vertical_scrollbar_create(&app_sport_list_scrollbar,
			NULL, &app_sport_list, &app_sport_list_scrollbar_properties,
			GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB | GX_SCROLLBAR_VERTICAL);
		gx_widget_fill_color_set(&app_sport_list_scrollbar, GX_COLOR_ID_HONBOW_DARK_GRAY, 
			GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY);
			VOID custom_scrollbar_vertical_draw(GX_SCROLLBAR *scrollbar);
		gx_widget_draw_set(&app_sport_list_scrollbar,custom_scrollbar_vertical_draw);
	}
	gx_widget_event_process_set(parent, app_sport_window_event);
	sport_main_page_jump_reg(&gps_connecting_window, OUTDOOR_RUN_CHILD_PAGE);
	sport_main_page_jump_reg(&gps_connecting_window, OUTDOOR_WALK_CHILD_PAGE);
	sport_main_page_jump_reg(&gps_connecting_window, OUTDOOR_BICYCLE_CHILD_PAGE);
	sport_main_page_jump_reg(&sport_start_window, INDOOR_BICYCLE_CHILD_PAGE);
	sport_main_page_jump_reg(&gps_connecting_window, ON_FOOT_CHILD_PAGE);
	sport_main_page_jump_reg(&sport_start_window, INDOOR_RUN_CHILD_PAGE);
	sport_main_page_jump_reg(&sport_start_window, INDOOR_WALK_CHILD_PAGE);
	app_gps_connecting_window_init(NULL, 1);
	app_sport_start_window_init(NULL);
	app_sport_paused_window_init(NULL);	
	app_sport_time_short_windown_init(NULL);
	app_gps_confirm_window_init(NULL);
	app_gps_not_connectd_windown_init(NULL);
}

void clear_sport_widow(void)
{
	GX_BOOL result;
	gx_widget_created_test(&sport_summary_window, &result);
	if (result) {
		gx_widget_delete(&sport_summary_window);
	}

	gx_widget_created_test(&count_down_window, &result);
	if (result) {
		gx_widget_delete(&count_down_window);
	}

	gx_widget_created_test(&sport_data_window, &result);
	if (result) {
		gx_widget_delete(&sport_data_window);
	}	
 	clear_gps_connect_status();
}

GUI_APP_DEFINE_WITH_INIT(sport, APP_ID_02_SPORT, (GX_WIDGET *)&app_sport_window,
	GX_PIXELMAP_ID_APP_LIST_ICON_SPORT_90_90, GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY,
	app_sport_window_init);