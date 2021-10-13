#include "gx_api.h"
#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "windows_manager.h"
static GX_WIDGET curr_widget;
static GX_PROMPT song_title;
static GX_PIXELMAP_BUTTON prev_button;
static GX_PIXELMAP_BUTTON next_button;
static GX_PIXELMAP_BUTTON play_pause_button;
static GX_PIXELMAP_BUTTON volume_increase_button;
static GX_PIXELMAP_BUTTON volume_decrease_button;
static GX_WIDGET volume_info;

#define BUTTON_PREV_ID 1
#define BUTTON_NEXT_ID 2
#define BUTTON_PLAY_PAUSE_ID 3
#define BUTTON_VOL_INC_ID 4
#define BUTTON_VOL_DEC_ID 5

static uint8_t curr_volume; // 0~10
static UINT event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	static WM_QUIT_JUDGE_T pen_info_judge;

	switch (event_ptr->gx_event_type) {
	case GX_EVENT_SHOW:
		// do something to get curr music info
		break;
	case GX_EVENT_PEN_DOWN:
		wm_point_down_record(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata);
		break;
	case GX_EVENT_PEN_UP:
		if (wm_quit_judge(&pen_info_judge, &event_ptr->gx_event_payload.gx_event_pointdata) == GX_TRUE) {
			windows_mgr_page_jump((GX_WINDOW *)&app_list_window, NULL, WINDOWS_OP_BACK);
		}
		break;
	case GX_SIGNAL(BUTTON_PREV_ID, GX_EVENT_CLICKED):
		return GX_SUCCESS;
	case GX_SIGNAL(BUTTON_NEXT_ID, GX_EVENT_CLICKED):
		return GX_SUCCESS;
	case GX_SIGNAL(BUTTON_PLAY_PAUSE_ID, GX_EVENT_CLICKED):
		return GX_SUCCESS;
	case GX_SIGNAL(BUTTON_VOL_INC_ID, GX_EVENT_CLICKED):
		if (curr_volume < 10)
			curr_volume++;
		gx_system_dirty_mark(&volume_info);
		return GX_SUCCESS;
	case GX_SIGNAL(BUTTON_VOL_DEC_ID, GX_EVENT_CLICKED):
		if (curr_volume > 0)
			curr_volume--;
		gx_system_dirty_mark(&volume_info);
		return GX_SUCCESS;
	default:
		break;
	}
	return gx_widget_event_process(widget, event_ptr);
}

static void volume_info_draw_func(GX_WIDGET *widget)
{
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);

	GX_COLOR color = 0;

	gx_context_color_get(GX_COLOR_ID_HONBOW_DARK_GRAY, &color);

	uint16_t width = abs(widget->gx_widget_size.gx_rectangle_bottom - widget->gx_widget_size.gx_rectangle_top) + 1;

	gx_brush_define(&current_context->gx_draw_context_brush, color, 0, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
	gx_context_brush_width_set(width);
	uint16_t half_line_width = (width >> 1) + 1;
	gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + half_line_width + 1,
						widget->gx_widget_size.gx_rectangle_top + half_line_width - 1,
						widget->gx_widget_size.gx_rectangle_right - half_line_width - 1,
						widget->gx_widget_size.gx_rectangle_top + half_line_width - 1);

	gx_context_color_get(GX_COLOR_ID_HONBOW_ORANGE, &color);
	gx_brush_define(&current_context->gx_draw_context_brush, color, 0, GX_BRUSH_ALIAS | GX_BRUSH_ROUND);
	if (curr_volume > 0) {
		gx_canvas_line_draw(widget->gx_widget_size.gx_rectangle_left + half_line_width + 1,
							widget->gx_widget_size.gx_rectangle_top + half_line_width - 1,
							widget->gx_widget_size.gx_rectangle_left + half_line_width + 1 +
								curr_volume *
									(widget->gx_widget_size.gx_rectangle_right -
									 widget->gx_widget_size.gx_rectangle_left - 2 * half_line_width - 2) /
									10,
							widget->gx_widget_size.gx_rectangle_top + half_line_width - 1);
	}
}

extern VOID custom_pixelmap_button_draw(GX_PIXELMAP_BUTTON *button);
extern VOID custom_circle_pixelmap_button_draw(GX_PIXELMAP_BUTTON *button);
void app_music_main_page_init(GX_WINDOW *window)
{
	GX_RECTANGLE childSize;
	gx_utility_rectangle_define(&childSize, 0, 0, 319, 359);
	gx_widget_create(&curr_widget, NULL, window, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE, &childSize);

	gx_widget_event_process_set(&curr_widget, event_processing_function);

	GX_PROMPT *song_title_p = &song_title;
	gx_utility_rectangle_define(&childSize, 0, 83, 319, 83 + 30 - 1);
	gx_prompt_create(&song_title, GX_NULL, &curr_widget, GX_ID_NONE,
					 GX_STYLE_TRANSPARENT | GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TEXT_CENTER, GX_ID_NONE,
					 &childSize);
	gx_prompt_text_color_set(song_title_p, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
							 GX_COLOR_ID_HONBOW_WHITE);
	gx_prompt_font_set(song_title_p, GX_FONT_ID_SIZE_30);
	GX_STRING init_string;
	init_string.gx_string_ptr = "song name";
	init_string.gx_string_length = strlen(init_string.gx_string_ptr);
	gx_prompt_text_set_ext(song_title_p, &init_string);

	gx_utility_rectangle_define(&childSize, 25, 167, 25 + 60 - 1, 167 + 60 - 1);
	gx_pixelmap_button_create(&prev_button, NULL, &curr_widget, GX_PIXELMAP_ID_WIDGETS_LEFT_ICON,
							  GX_PIXELMAP_ID_WIDGETS_LEFT_ICON, GX_PIXELMAP_ID_WIDGETS_LEFT_ICON,
							  GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_VALIGN_CENTER | GX_STYLE_HALIGN_CENTER,
							  BUTTON_PREV_ID, &childSize);
	gx_widget_fill_color_set(&prev_button, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
							 GX_COLOR_ID_HONBOW_DARK_GRAY);
	gx_widget_draw_set(&prev_button, custom_circle_pixelmap_button_draw);

	gx_utility_rectangle_define(&childSize, 235, 167, 235 + 60 - 1, 167 + 60 - 1);
	gx_pixelmap_button_create(&next_button, NULL, &curr_widget, GX_PIXELMAP_ID_WIDGETS_RIGHT_ICON,
							  GX_PIXELMAP_ID_WIDGETS_RIGHT_ICON, GX_PIXELMAP_ID_WIDGETS_RIGHT_ICON,
							  GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_VALIGN_CENTER | GX_STYLE_HALIGN_CENTER,
							  BUTTON_NEXT_ID, &childSize);
	gx_widget_fill_color_set(&next_button, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
							 GX_COLOR_ID_HONBOW_DARK_GRAY);
	gx_widget_draw_set(&next_button, custom_circle_pixelmap_button_draw);

	gx_utility_rectangle_define(&childSize, 113, 150, 113 + 94 - 1, 150 + 94 - 1);
	gx_pixelmap_button_create(&play_pause_button, NULL, &curr_widget, GX_PIXELMAP_ID_WIDGETS_PLAY_ICON,
							  GX_PIXELMAP_ID_WIDGETS_PLAY_ICON, GX_PIXELMAP_ID_WIDGETS_PLAY_ICON,
							  GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_VALIGN_CENTER | GX_STYLE_HALIGN_CENTER,
							  BUTTON_PLAY_PAUSE_ID, &childSize);
	gx_widget_fill_color_set(&play_pause_button, GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_DARK_GRAY,
							 GX_COLOR_ID_HONBOW_DARK_GRAY);
	gx_widget_draw_set(&play_pause_button, custom_circle_pixelmap_button_draw);

	gx_utility_rectangle_define(&childSize, 15, 286, 25 + 59 - 1, 286 + 59 - 1);
	gx_pixelmap_button_create(&volume_decrease_button, NULL, &curr_widget, GX_PIXELMAP_ID_WIDGETS_LOW_VOLUME_ICON,
							  GX_PIXELMAP_ID_WIDGETS_LOW_VOLUME_ICON, GX_PIXELMAP_ID_WIDGETS_LOW_VOLUME_ICON,
							  GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_VALIGN_CENTER | GX_STYLE_HALIGN_CENTER,
							  BUTTON_VOL_DEC_ID, &childSize);
	gx_widget_fill_color_set(&volume_decrease_button, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);

	gx_utility_rectangle_define(&childSize, 246, 286, 256 + 59 - 1, 286 + 59 - 1);
	gx_pixelmap_button_create(&volume_increase_button, NULL, &curr_widget, GX_PIXELMAP_ID_WIDGETS_HIGH_VOLUME_ICON,
							  GX_PIXELMAP_ID_WIDGETS_HIGH_VOLUME_ICON, GX_PIXELMAP_ID_WIDGETS_HIGH_VOLUME_ICON,
							  GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE | GX_STYLE_VALIGN_CENTER | GX_STYLE_HALIGN_CENTER,
							  BUTTON_VOL_INC_ID, &childSize);
	gx_widget_fill_color_set(&volume_increase_button, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS, GX_COLOR_ID_CANVAS);

	gx_utility_rectangle_define(&childSize, 74, 312, 74 + 172 - 1, 312 + 8 - 1);
	gx_widget_create(&volume_info, NULL, &curr_widget, GX_STYLE_ENABLED | GX_STYLE_BORDER_NONE, GX_ID_NONE, &childSize);
	gx_widget_draw_set(&volume_info, volume_info_draw_func);
}