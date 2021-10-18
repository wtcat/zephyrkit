#include "guix_api.h"

#define GUIX_ANIMATION_ID_SCREEN_LIST_SLIDE 2

GX_WIDGET *current_screen_ptr;

VOID guix_screen_switch_to(GX_WIDGET *parent, GX_WIDGET *next)
{
	gx_widget_detach(current_screen_ptr);
	gx_widget_attach(parent, next);
	current_screen_ptr = next;
}

VOID guix_screen_slide_list_create(GUIX_SLIDE_LIST *list, USHORT id)
{
	gx_animation_create(&list->anim[0]);
	gx_animation_create(&list->anim[1]);
	gx_animation_landing_speed_set(&list->anim[0], 50);
	gx_animation_landing_speed_set(&list->anim[1], 50);

	memset(list->anim_info, 0, sizeof(list->anim_info));
	for (int i = 0; i < 2; i++) {
		list->anim_info[i].gx_animation_frame_interval = 1;
		list->anim_info[i].gx_animation_id = id;
		list->anim_info[i].gx_animation_steps = 10;
		list->anim_info[i].gx_animation_start_alpha = 255;
		list->anim_info[i].gx_animation_end_alpha = 255;
	}
}

UINT guix_screen_slide_list_set(GUIX_SLIDE_LIST *list, USHORT frame_interval, GX_UBYTE steps, GX_UBYTE start_alpha,
								GX_UBYTE end_alpha)
{
	if (frame_interval < 1 || steps < 2)
		return GX_INVALID_VALUE;

	for (int i = 0; i < 2; i++) {
		list->anim_info[i].gx_animation_frame_interval = frame_interval;
		list->anim_info[i].gx_animation_steps = steps;
		list->anim_info[i].gx_animation_start_alpha = start_alpha;
		list->anim_info[i].gx_animation_end_alpha = end_alpha;
	}
	return GX_SUCCESS;
}

VOID guix_screen_slide_list_start(GUIX_SLIDE_LIST *handle, GX_WIDGET *parent, GX_WIDGET **list,
								  enum slide_direction dir)
{
	GX_ANIMATION_INFO *anim_info = handle->anim_info;
	GX_WIDGET *widget = list[0];
	INT idx = 0;
	INT idx_1 = 0;
	INT idx_2;
	INT width;
	INT sign = 0;
	INT height;
	INT sign_height = 0;
	;

	while (widget) {
		if (widget->gx_widget_status & GX_STATUS_VISIBLE)
			idx_1 = idx;
		idx++;
		widget = list[idx];
	}

	switch (dir) {
	case SCREEN_SLIDE_LEFT:
		if (idx_1 == idx - 1)
			idx_2 = 0;
		else
			idx_2 = idx_1 + 1;
		sign = -1;
		break;
	case SCREEN_SLIDE_RIGHT:
		if (idx_1 == 0)
			idx_2 = idx - 1;
		else
			idx_2 = idx_1 - 1;
		sign = 1;
		break;
	case SCREEN_SLIDE_TOP:
		if (idx_1 == idx - 1)
			idx_2 = 0;
		else
			idx_2 = idx_1 + 1;
		sign_height = -1;
		break;
	case SCREEN_SLIDE_BOTTOM:
		if (idx_1 == 0)
			idx_2 = idx - 1;
		else
			idx_2 = idx_1 - 1;
		sign_height = 1;
		break;
	default:
		return;
	}

	widget = list[idx_1];
	width = widget->gx_widget_size.gx_rectangle_right - widget->gx_widget_size.gx_rectangle_left + 1;
	width *= sign;

	height = widget->gx_widget_size.gx_rectangle_bottom - widget->gx_widget_size.gx_rectangle_top + 1;
	height *= sign_height;

	anim_info[0].gx_animation_parent = parent;
	anim_info[0].gx_animation_start_position.gx_point_x = widget->gx_widget_size.gx_rectangle_left;
	anim_info[0].gx_animation_start_position.gx_point_y = widget->gx_widget_size.gx_rectangle_top;
	anim_info[0].gx_animation_end_position.gx_point_x = widget->gx_widget_size.gx_rectangle_left + width;
	anim_info[0].gx_animation_end_position.gx_point_y = widget->gx_widget_size.gx_rectangle_top + height;
	anim_info[0].gx_animation_target = widget;
	anim_info[0].gx_animation_style = GX_ANIMATION_TRANSLATE | GX_ANIMATION_DETACH;

	width *= -1;
	height *= -1;

	anim_info[1].gx_animation_parent = parent;
	anim_info[1].gx_animation_start_position.gx_point_x = widget->gx_widget_size.gx_rectangle_left + width;
	anim_info[1].gx_animation_start_position.gx_point_y = widget->gx_widget_size.gx_rectangle_top + height;
	anim_info[1].gx_animation_end_position.gx_point_x = widget->gx_widget_size.gx_rectangle_left;
	anim_info[1].gx_animation_end_position.gx_point_y = widget->gx_widget_size.gx_rectangle_top;
	anim_info[1].gx_animation_target = list[idx_2];

	gx_animation_start(&handle->anim[0], &anim_info[0]);
	gx_animation_start(&handle->anim[1], &anim_info[1]);
}
