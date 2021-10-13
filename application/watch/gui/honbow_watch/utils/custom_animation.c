#include "guix_api.h"
#include "zephyr.h"
GUIX_SLIDE_LIST slide_list;

#define ANIMATION_ID 0x10

void custom_animation_init(void)
{
	guix_screen_slide_list_create(&slide_list, ANIMATION_ID);
	guix_screen_slide_list_set(&slide_list, 1, 4, 255, 255);
}

static GX_WIDGET *list_tmp[2] = {GX_NULL, GX_NULL};
UINT (*custom_anim_target1_event_process_function)(struct GX_WIDGET_STRUCT *, GX_EVENT *);
UINT (*custom_anim_target2_event_process_function)(struct GX_WIDGET_STRUCT *, GX_EVENT *);
static UINT custom_anim_event_processing_function(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
	switch (event_ptr->gx_event_type) {
	case GX_EVENT_ANIMATION_COMPLETE: {
		USHORT animation_id = event_ptr->gx_event_sender;
		if (animation_id == ANIMATION_ID) {
			if (widget == list_tmp[0]) {
				gx_widget_event_process_set(list_tmp[0], custom_anim_target1_event_process_function);
				custom_anim_target1_event_process_function = NULL;
				list_tmp[0] = NULL;
				if ((widget->gx_widget_status & GX_STATUS_VISIBLE) == 0) {
					gx_widget_detach(widget);
				}
				else{
					_gx_system_focus_claim(widget);
					// _gx_widget_child_focus_assign(widget->gx_widget_parent);
				}
			} else if (widget == list_tmp[1]) {
				gx_widget_event_process_set(list_tmp[1], custom_anim_target2_event_process_function);
				custom_anim_target2_event_process_function = NULL;
				list_tmp[1] = NULL;
				if ((widget->gx_widget_status & GX_STATUS_VISIBLE) == 0) {
					gx_widget_detach(widget);
				}
				else{
					_gx_system_focus_claim(widget);
					//_gx_widget_child_focus_assign(widget->gx_widget_parent);
				}
			} else {
				return GX_PTR_ERROR;
			}
			return GX_SUCCESS;
		}
	}
	default:
		if (widget == list_tmp[0]) {
			if (custom_anim_target1_event_process_function) {
				return custom_anim_target1_event_process_function(widget, event_ptr);
			} else {
				return GX_PTR_ERROR;
			}
		} else if (widget == list_tmp[1]) {
			if (custom_anim_target2_event_process_function) {
				return custom_anim_target2_event_process_function(widget, event_ptr);
			} else {
				return GX_PTR_ERROR;
			}
		} else {
			return GX_PTR_ERROR; // ERROR!
		}
	}
}

int custom_animation_start(GX_WIDGET **list, GX_WIDGET *parent, enum slide_direction dir)
{
	if ((list_tmp[0] == NULL) && (list_tmp[1] == NULL)) {
		list_tmp[0] = list[0];
		list_tmp[1] = list[1];
	} else {
		return -1;
	}
	custom_anim_target1_event_process_function = list[0]->gx_widget_event_process_function;
	custom_anim_target2_event_process_function = list[1]->gx_widget_event_process_function;
	gx_widget_event_process_set(list[0], custom_anim_event_processing_function);
	gx_widget_event_process_set(list[1], custom_anim_event_processing_function);
	guix_screen_slide_list_start(&slide_list, parent, list, dir);
	return 0;
}

GX_BOOL custom_animation_busy_check(void)
{
	if ((list_tmp[0] == NULL) && (list_tmp[1] == NULL)) {
		return GX_FALSE;
	} else {
		return GX_TRUE;
	}
}
