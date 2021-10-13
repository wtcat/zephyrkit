#include "home_window.h"
#include "custom_pixelmap_mirror.h"
#include "custom_util.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "wf_window.h"
#include "sys/util.h"
#include "custom_animation.h"
#include "home_widget_misc.h"
#include "gx_animation.h"
#include "logging/log.h"
#include "windows_manager.h"

LOG_MODULE_DECLARE(guix_log);

#define DRAG_MIN_DELTA 50
typedef struct {
	GX_WIDGET widget;
	GX_RESOURCE_ID color_id;
} dot_t;

typedef struct {
	GX_WIDGET parent;
	dot_t left2_window;
	dot_t left1_window;
	dot_t center_window;
	dot_t right1_window;
	dot_t right2_window;
} dot_tips_union_t;

dot_tips_union_t circle_tips;
extern void circle_tips_disp_adjust(void);

/* Define menu screen list. */
GX_WIDGET *slide_ver_screen_list[] = {
	(GX_WIDGET *)&message_window,
	(GX_WIDGET *)&root_window.root_window_wf_window,
	(GX_WIDGET *)&control_center_window,
	GX_NULL,
};
#define ANIMATION_ID_SLIDE_VER 2

extern VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen);

GX_ANIMATION slide_ver_animation;

VOID start_ver_drag_animation(GX_WINDOW *window)
{
	GX_ANIMATION_INFO slide_animation_info;

	memset(&slide_animation_info, 0, sizeof(GX_ANIMATION_INFO));
	slide_animation_info.gx_animation_parent = (GX_WIDGET *)window;
	slide_animation_info.gx_animation_style = GX_ANIMATION_SCREEN_DRAG | GX_ANIMATION_VERTICAL;
	slide_animation_info.gx_animation_id = ANIMATION_ID_SLIDE_VER;
	slide_animation_info.gx_animation_frame_interval = 1;
	slide_animation_info.gx_animation_slide_screen_list = slide_ver_screen_list;

	slide_animation_info.gx_animation_range_limit_type = GX_ANIMATION_LIMIT_STYLE_NONE;
	gx_animation_drag_enable(&slide_ver_animation, (GX_WIDGET *)window, &slide_animation_info);
}

static GX_WIDGET *silde_hor_screen_list[] = {
	GX_NULL, GX_NULL, (GX_WIDGET *)&root_window.root_window_wf_window, GX_NULL, GX_NULL, GX_NULL,
};
static GX_WIDGET *home_widget_mgr[HOME_WIDGET_TYPE_MAX];

void home_widget_type_reg(GX_WIDGET *widget, HOME_WIDGET_TYPE type)
{
	if (type < HOME_WIDGET_TYPE_MAX) {
		home_widget_mgr[type] = widget;
	}
}

void home_widget_edit(HOME_WIDGET_INDEX index, HOME_WIDGET_TYPE type)
{
	if ((index >= HOME_WIDGET_INDEX_MAX) || (index == HOME_WIDGET_INDEX_CENTER) || (type >= HOME_WIDGET_TYPE_MAX)) {
		return;
	}

	if (home_widget_mgr[type] != NULL) {
		silde_hor_screen_list[index] = home_widget_mgr[type];
	}
}

#define ANIMATION_ID_SLIDE_HOR 1
GX_ANIMATION slide_hor_animation;
VOID start_hor_drag_animation(GX_WINDOW *window)
{
	GX_ANIMATION_INFO slide_animation_info;

	memset(&slide_animation_info, 0, sizeof(GX_ANIMATION_INFO));
	slide_animation_info.gx_animation_parent = (GX_WIDGET *)window;
	slide_animation_info.gx_animation_style = GX_ANIMATION_SCREEN_DRAG | GX_ANIMATION_HORIZONTAL;
	slide_animation_info.gx_animation_id = ANIMATION_ID_SLIDE_HOR;
	slide_animation_info.gx_animation_frame_interval = 1;
	uint8_t index = 0;
	for (index = 0; index < sizeof(silde_hor_screen_list) / sizeof(GX_WIDGET *); index++) {
		if (silde_hor_screen_list[index] != NULL) {
			break;
		}
	}
	slide_animation_info.gx_animation_slide_screen_list = &silde_hor_screen_list[index];
	gx_animation_drag_enable(&slide_hor_animation, (GX_WIDGET *)window, &slide_animation_info);
}

/*****************************************************************
	root window event for slide left right up down control
 *****************************************************************/
static GX_VALUE last_x = 0;
static GX_VALUE last_y = 0;
static volatile uint8_t enter_slide_ver = 0;
static volatile uint8_t enter_slide_hor = 0;
static GX_VALUE delta_y;
static GX_VALUE delta_x;
volatile static uint8_t home_window_enter_pen_down = 0;

bool home_window_is_pen_down(void)
{
	return home_window_enter_pen_down == 1 ? true : false;
}

bool home_window_drag_anim_busy(void)
{
	if (!enter_slide_ver & !enter_slide_hor) {
		return false;
	} else {
		return true;
	}
}

static GX_BOOL find_curr_visible_widget_from_hor_list(uint8_t *id)
{
	GX_WIDGET *widget = NULL;
	for (uint8_t i = 0; i < slide_hor_animation.gx_animation_slide_screen_list_size; i++) {
		widget = slide_hor_animation.gx_animation_info.gx_animation_slide_screen_list[i];
		if (widget != NULL) {
			if (widget->gx_widget_status & GX_STATUS_VISIBLE) {
				*id = i;
				return GX_TRUE;
			}
		}
	}
	return GX_FALSE;
}
static GX_BOOL find_wf_widget_id(uint8_t *id)
{
	for (uint8_t i = 0; i < slide_hor_animation.gx_animation_slide_screen_list_size; i++) {
		if (slide_hor_animation.gx_animation_info.gx_animation_slide_screen_list[i] ==
			(GX_WIDGET *)&root_window.root_window_wf_window) {
			*id = i;
			return GX_TRUE;
		}
	}
	return GX_FALSE;
}

static GX_BOOL ver_animation_start_judge(void)
{
	GX_WIDGET *widget = NULL;
	for (uint8_t i = 0; i < sizeof(slide_ver_screen_list) / sizeof(GX_WIDGET *); i++) {
		widget = slide_ver_screen_list[i];
		if (widget != NULL) {
			if (widget->gx_widget_status & GX_STATUS_VISIBLE) {
				return GX_TRUE;
			}
		}
	}
	return GX_FALSE;
}

static GX_BOOL hor_animation_start_judge(void)
{
	GX_WIDGET *widget = NULL;
	for (uint8_t i = 0; i < sizeof(silde_hor_screen_list) / sizeof(GX_WIDGET *); i++) {
		widget = silde_hor_screen_list[i];
		if (widget != NULL) {
			if (widget->gx_widget_status & GX_STATUS_VISIBLE) {
				return GX_TRUE;
			}
		}
	}
	return GX_FALSE;
}
static uint8_t wf_mirror_created = 0;
#define CLOCK_SHOW_WF_TIMER 1
#define CLOCK_DOT_DISP_TIMER 2

#define TEST_TIMER 3
UINT home_window_event(GX_WINDOW *window, GX_EVENT *event_ptr)
{
	static uint8_t go_wf_page_flag = 0;
	static GX_BOOL need_gen_evt = GX_FALSE;
	static uint8_t target_id;
	static GX_EVENT tmp;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW: {
		if (!wf_mirror_created) {
			GX_CANVAS *canvas_tmp;
			gx_widget_canvas_get((GX_WIDGET *)window, &canvas_tmp);
			if (!mirror_obj_create_copy(&wf_mirror_mgr.wf_mirror_obj, canvas_tmp)) {
				wf_mirror_created = 1;
			}
		}
	} break;
	case GX_EVENT_HIDE:
		break;
	case GX_EVENT_TIMER:
		if (event_ptr->gx_event_payload.gx_event_timer_id == CLOCK_SHOW_WF_TIMER) {
			if ((slide_ver_animation.gx_animation_status == GX_ANIMATION_IDLE) &&
				(slide_hor_animation.gx_animation_status == GX_ANIMATION_IDLE) &&
				(root_window.root_window_wf_window.gx_widget_status & GX_STATUS_VISIBLE)) {
				// screen_switch(window->gx_widget_parent, (GX_WIDGET *)&wf_list);
				windows_mgr_page_jump(window, (GX_WINDOW *)&wf_list, WINDOWS_OP_SWITCH_NEW);
				home_window_enter_pen_down = 0;
			}
			return 0;
		} else if (event_ptr->gx_event_payload.gx_event_timer_id == CLOCK_DOT_DISP_TIMER) {
			gx_widget_hide(&circle_tips.parent);
			return 0;
		} else {
			break;
		}
	case GX_EVENT_PEN_DOWN:
	case GX_EVENT_PEN_DRAG: {
		wf_window_mirror_set(true);
		if ((home_window_enter_pen_down) && !custom_animation_busy_check()) {
			if ((slide_ver_animation.gx_animation_original_event_process_function == NULL) &&
				(slide_hor_animation.gx_animation_original_event_process_function == NULL)) {
				delta_y = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y - last_y);
				delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - last_x);
				if ((delta_x > DRAG_MIN_DELTA) || (delta_y > DRAG_MIN_DELTA)) {
					need_gen_evt = GX_FALSE;
					if (delta_y > delta_x) {
						if (ver_animation_start_judge()) {
							gx_system_timer_stop(window, CLOCK_SHOW_WF_TIMER);
							start_ver_drag_animation(window);
							enter_slide_ver = 1;
							gx_widget_hide(&circle_tips.parent);
							need_gen_evt = GX_TRUE;
						}
					} else {
						if (hor_animation_start_judge()) {
							gx_system_timer_stop(window, CLOCK_SHOW_WF_TIMER);
							start_hor_drag_animation(window);
							enter_slide_hor = 1;
							gx_system_timer_stop(window, CLOCK_DOT_DISP_TIMER);
							gx_widget_front_move(&circle_tips.parent, NULL);
							circle_tips_disp_adjust();
							gx_widget_show(&circle_tips.parent);
							need_gen_evt = GX_TRUE;
						}
					}

					if (need_gen_evt == GX_TRUE) {
						need_gen_evt = GX_FALSE;
						tmp = *event_ptr;
						tmp.gx_event_payload.gx_event_pointdata.gx_point_x = last_x;
						tmp.gx_event_payload.gx_event_pointdata.gx_point_y = last_y;
						tmp.gx_event_type = GX_EVENT_PEN_DOWN;
						tmp.gx_event_target = (struct GX_WIDGET_STRUCT *)window;
						gx_system_event_send(&tmp);

						tmp.gx_event_payload.gx_event_pointdata.gx_point_x =
							event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
						tmp.gx_event_payload.gx_event_pointdata.gx_point_y =
							event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y;
						tmp.gx_event_type = GX_EVENT_PEN_DRAG;
						gx_system_event_send(&tmp);
					}
				}
			}
		}

		if (slide_hor_animation.gx_animation_original_event_process_function != NULL) {
			gx_widget_front_move(&circle_tips.parent, NULL);
			circle_tips_disp_adjust();
			gx_widget_show(&circle_tips.parent);
		}

		if (!home_window_enter_pen_down) {
			home_window_enter_pen_down = 1;
			gx_system_timer_start(window, CLOCK_SHOW_WF_TIMER, 100, 0);
			last_y = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y;
			last_x = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
		}

		// deal display limit!!!!
		if (slide_ver_animation.gx_animation_original_event_process_function != NULL) {
			if (slide_ver_animation.gx_animation_slide_target_index_2 == -1) { // we should limit it!!!
				if (slide_ver_animation.gx_animation_slide_target_index_1 >= 0) {
					gx_widget_resize(slide_ver_screen_list[slide_ver_animation.gx_animation_slide_target_index_1],
									 &window->gx_widget_size);
				}
			}
		}
		break;
	}
	case GX_EVENT_PEN_UP: {
		gx_system_timer_stop(window, CLOCK_SHOW_WF_TIMER);
		home_window_enter_pen_down = 0;
		circle_tips_disp_adjust();
		gx_widget_front_move(&circle_tips.parent, NULL);
		gx_system_timer_start(window, CLOCK_DOT_DISP_TIMER, 100, 0);
		break;
	}
	case GX_EVENT_ANIMATION_COMPLETE:
		if (root_window.root_window_wf_window.gx_widget_status & GX_STATUS_VISIBLE) {
			enter_slide_ver = 0;
			enter_slide_hor = 0;
		}
		if (event_ptr->gx_event_target == (GX_WIDGET *)window) {
			if (event_ptr->gx_event_sender == ANIMATION_ID_SLIDE_HOR) {
				gx_animation_drag_disable(&slide_hor_animation, (GX_WIDGET *)window);
			} else if (event_ptr->gx_event_sender == ANIMATION_ID_SLIDE_VER) {
				gx_animation_drag_disable(&slide_ver_animation, (GX_WIDGET *)window);
			}
			if (go_wf_page_flag) {
				uint8_t curr_id = 0;
				GX_RECTANGLE childSize;
				if (find_curr_visible_widget_from_hor_list(&curr_id)) {
					uint8_t new_id;
					if (curr_id < target_id) {
						new_id = curr_id + 1;
						slide_hor_animation.gx_animation_slide_direction = GX_ANIMATION_SLIDE_RIGHT;
						gx_utility_rectangle_define(&childSize, HONBOW_DISP_X_RESOLUTION, 0,
													HONBOW_DISP_X_RESOLUTION + HONBOW_DISP_X_RESOLUTION - 1,
													HONBOW_DISP_Y_RESOLUTION - 1);
					} else {
						new_id = curr_id - 1;
						slide_hor_animation.gx_animation_slide_direction = GX_ANIMATION_SLIDE_LEFT;
						gx_utility_rectangle_define(&childSize, -HONBOW_DISP_X_RESOLUTION + 1, 0, 0,
													HONBOW_DISP_Y_RESOLUTION - 1);
					}
					if (new_id == target_id) {
						go_wf_page_flag = 0;
					} else {
						go_wf_page_flag = 1;
					}
					start_hor_drag_animation(window);
					gx_widget_attach(slide_hor_animation.gx_animation_info.gx_animation_parent,
									 slide_hor_animation.gx_animation_info.gx_animation_slide_screen_list[new_id]);
					gx_widget_resize(slide_hor_animation.gx_animation_info.gx_animation_slide_screen_list[new_id],
									 &childSize);
					slide_hor_animation.gx_animation_slide_target_index_1 = curr_id;
					slide_hor_animation.gx_animation_slide_target_index_2 = new_id;
					_gx_animation_slide_landing_start(&slide_hor_animation);
				}
			}
			circle_tips_disp_adjust();
			gx_widget_front_move(&circle_tips.parent, NULL);
			gx_system_timer_start(window, CLOCK_DOT_DISP_TIMER, 100, 0);
			return 0;
		}
		break;
	case GX_EVENT_KEY_DOWN:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			return 0;
		}
		break;
	case GX_EVENT_KEY_UP:
		if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME) {
			if ((!enter_slide_ver) && (!enter_slide_hor) && !custom_animation_busy_check()) {
				// screen_switch((GX_WIDGET *)&root_window, (GX_WIDGET *)&app_list_window);
				windows_mgr_page_jump(window, (GX_WINDOW *)&app_list_window, WINDOWS_OP_SWITCH_NEW);
			} else {
				if (!home_window_enter_pen_down) {
					if (enter_slide_ver) {
#if 0
						start_ver_drag_animation(window);
						gx_widget_attach(slide_ver_animation.gx_animation_info.gx_animation_parent,
										 slide_ver_animation.gx_animation_info.gx_animation_slide_screen_list[0]);
						slide_ver_animation.gx_animation_slide_target_index_1 = 1;
						slide_ver_animation.gx_animation_slide_target_index_2 = 0;
						slide_ver_animation.gx_animation_slide_direction = GX_ANIMATION_SLIDE_DOWN;
						_gx_animation_slide_landing_start(&slide_ver_animation);
#endif
					} else if (enter_slide_hor) {
						uint8_t curr_id = 0;
						GX_RECTANGLE childSize;
						if (find_curr_visible_widget_from_hor_list(&curr_id) && find_wf_widget_id(&target_id)) {
							uint8_t new_id;
							if (curr_id < target_id) {
								new_id = curr_id + 1;
								slide_hor_animation.gx_animation_slide_direction = GX_ANIMATION_SLIDE_LEFT;
								gx_utility_rectangle_define(&childSize, HONBOW_DISP_X_RESOLUTION, 0,
															HONBOW_DISP_X_RESOLUTION + HONBOW_DISP_X_RESOLUTION - 1,
															HONBOW_DISP_Y_RESOLUTION - 1);
							} else {
								new_id = curr_id - 1;
								slide_hor_animation.gx_animation_slide_direction = GX_ANIMATION_SLIDE_RIGHT;
								gx_utility_rectangle_define(&childSize, -HONBOW_DISP_X_RESOLUTION + 1, 0, 0,
															HONBOW_DISP_Y_RESOLUTION - 1);
							}
							if (new_id == target_id) {
								go_wf_page_flag = 0;
							} else {
								go_wf_page_flag = 1;
							}
							start_hor_drag_animation(window);
							gx_widget_attach(
								slide_hor_animation.gx_animation_info.gx_animation_parent,
								slide_hor_animation.gx_animation_info.gx_animation_slide_screen_list[new_id]);
							gx_widget_resize(
								slide_hor_animation.gx_animation_info.gx_animation_slide_screen_list[new_id],
								&childSize);
							slide_hor_animation.gx_animation_slide_target_index_1 = curr_id;
							slide_hor_animation.gx_animation_slide_target_index_2 = new_id;
							_gx_animation_slide_landing_start(&slide_hor_animation);
						}
					}
				}
			}
#if 0
			GX_EVENT event;
			event.gx_event_type = GX_EVENT_TERMINATE;
			event.gx_event_target = NULL;
			// printk("GUIX enter in sleep\n");
			gx_system_event_send(&event);
#endif
			return 0;
		}
		break;
	default:
		break;
	}
	return gx_window_event_process(window, event_ptr);
}

void circle_tips_draw(GX_WIDGET *widget)
{
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);

	dot_t *dot = CONTAINER_OF(widget, dot_t, widget);
	GX_COLOR color = 0;
	gx_context_color_get(dot->color_id, &color);

	if (dot == &circle_tips.center_window) {
		gx_brush_define(&current_context->gx_draw_context_brush, color, color, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
	} else {
		gx_brush_define(&current_context->gx_draw_context_brush, color, color, GX_BRUSH_SOLID_FILL | GX_BRUSH_ALIAS);
	}

	gx_context_brush_width_set(1);
	INT r = 4;
	INT x = widget->gx_widget_size.gx_rectangle_left + r;
	INT y = widget->gx_widget_size.gx_rectangle_top + r;
	gx_canvas_circle_draw(x, y, r);
}

void home_window_draw(GX_WIDGET *home)
{
	gx_widget_draw(home);
}

void circle_window_draw(GX_WIDGET *widget)
{
	gx_widget_children_draw(widget);
}

void home_window_init(GX_WIDGET *home)
{
#if 1
	GX_RECTANGLE child;
	gx_utility_rectangle_define(&child, 123, 8, 123 + 74 - 1, 8 + 10 - 1);
	gx_widget_create(&circle_tips.parent, NULL, home, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT,
					 GX_ID_NONE, &child);

	GX_VALUE x_start = 123; // need absolut pos
	GX_VALUE y_start = 8;
	gx_utility_rectangle_define(&child, x_start, y_start, x_start + 9, y_start + 9);
	gx_widget_create(&circle_tips.left2_window.widget, NULL, &circle_tips.parent,
					 GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child);
	gx_widget_draw_set(&circle_tips.left2_window.widget, circle_tips_draw);

	x_start += 16;
	gx_utility_rectangle_define(&child, x_start, y_start, x_start + 9, y_start + 9);
	gx_widget_create(&circle_tips.left1_window.widget, NULL, &circle_tips.parent,
					 GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child);
	gx_widget_draw_set(&circle_tips.left1_window.widget, circle_tips_draw);

	x_start += 16;
	gx_utility_rectangle_define(&child, x_start, y_start, x_start + 9, y_start + 9);
	gx_widget_create(&circle_tips.center_window.widget, NULL, &circle_tips.parent,
					 GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child);
	gx_widget_draw_set(&circle_tips.center_window.widget, circle_tips_draw);

	x_start += 16;
	gx_utility_rectangle_define(&child, x_start, y_start, x_start + 9, y_start + 9);
	gx_widget_create(&circle_tips.right1_window.widget, NULL, &circle_tips.parent,
					 GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child);
	gx_widget_draw_set(&circle_tips.right1_window.widget, circle_tips_draw);

	x_start += 16;
	gx_utility_rectangle_define(&child, x_start, y_start, x_start + 9, y_start + 9);
	gx_widget_create(&circle_tips.right2_window.widget, NULL, &circle_tips.parent,
					 GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, GX_ID_NONE, &child);
	gx_widget_draw_set(&circle_tips.right2_window.widget, circle_tips_draw);

	gx_widget_hide(&circle_tips.parent);
#endif

	home_widget_init();

	home_widget_edit(HOME_WIDGET_INDEX_RIGHT_2, HOME_WIDGET_TYPE_MUSIC);
	home_widget_edit(HOME_WIDGET_INDEX_RIGHT_1, HOME_WIDGET_TYPE_COMPASS);
	home_widget_edit(HOME_WIDGET_INDEX_LEFT_2, HOME_WIDGET_TYPE_COUNTER_DOWN);
	home_widget_edit(HOME_WIDGET_INDEX_LEFT_1, HOME_WIDGET_TYPE_STOPWATCH);
}

void circle_tips_disp_adjust(void)
{
	if (silde_hor_screen_list[0] == NULL) {
		gx_widget_hide(&circle_tips.left2_window.widget);
	} else {
		if (silde_hor_screen_list[0]->gx_widget_status & GX_STATUS_VISIBLE) {
			circle_tips.left2_window.color_id = GX_COLOR_ID_HONBOW_WHITE;
		} else {
			circle_tips.left2_window.color_id = GX_COLOR_ID_HONBOW_DARK_GRAY;
		}
	}

	if (silde_hor_screen_list[1] == NULL) {
		gx_widget_hide(&circle_tips.left1_window.widget);
	} else {
		if (silde_hor_screen_list[1]->gx_widget_status & GX_STATUS_VISIBLE) {
			circle_tips.left1_window.color_id = GX_COLOR_ID_HONBOW_WHITE;
		} else {
			circle_tips.left1_window.color_id = GX_COLOR_ID_HONBOW_DARK_GRAY;
		}
	}

	if (silde_hor_screen_list[2] == NULL) {
		gx_widget_hide(&circle_tips.center_window.widget);
	} else {
		if (silde_hor_screen_list[2]->gx_widget_status & GX_STATUS_VISIBLE) {
			circle_tips.center_window.color_id = GX_COLOR_ID_HONBOW_WHITE;
		} else {
			circle_tips.center_window.color_id = GX_COLOR_ID_HONBOW_DARK_GRAY;
		}
	}

	// menu_screen_list[2] always show
	if (silde_hor_screen_list[3] == NULL) {
		gx_widget_hide(&circle_tips.right1_window.widget);
	} else {
		if (silde_hor_screen_list[3]->gx_widget_status & GX_STATUS_VISIBLE) {
			circle_tips.right1_window.color_id = GX_COLOR_ID_HONBOW_WHITE;
		} else {
			circle_tips.right1_window.color_id = GX_COLOR_ID_HONBOW_DARK_GRAY;
		}
	}

	if (silde_hor_screen_list[4] == NULL) {
		gx_widget_hide(&circle_tips.right2_window.widget);
	} else {
		if (silde_hor_screen_list[4]->gx_widget_status & GX_STATUS_VISIBLE) {
			circle_tips.right2_window.color_id = GX_COLOR_ID_HONBOW_WHITE;
		} else {
			circle_tips.right2_window.color_id = GX_COLOR_ID_HONBOW_DARK_GRAY;
		}
	}
}