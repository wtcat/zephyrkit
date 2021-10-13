#include "custom_screen_stack.h"
#include "custom_animation.h"

static GX_WIDGET *custom_screen_stack_list[] = {
	GX_NULL,
	GX_NULL,
	GX_NULL,
};

UINT custom_screen_stack_create(GX_SCREEN_STACK_CONTROL *control, GX_WIDGET **memory, INT size)
{
	control->gx_screen_stack_control_memory = memory;
	control->gx_screen_stack_control_max = (INT)((UINT)size / (sizeof(GX_WIDGET *) << 1));
	control->gx_screen_stack_control_top = -1;

	return (GX_SUCCESS);
}
UINT custom_screen_stack_reset(GX_SCREEN_STACK_CONTROL *control)
{
	control->gx_screen_stack_control_top = -1;
	return (GX_SUCCESS);
}
UINT custom_screen_stack_push(GX_SCREEN_STACK_CONTROL *control, GX_WIDGET *screen, GX_WIDGET *new_screen)
{
	if (GX_TRUE == custom_animation_busy_check()) {
		return GX_FAILURE;
	}

	INT top;
	control->gx_screen_stack_control_top++;
	top = control->gx_screen_stack_control_top;
	GX_WIDGET *parent = screen->gx_widget_parent;

	if (top <= control->gx_screen_stack_control_max - 1) {
		top <<= 1;

		custom_screen_stack_list[0] = screen;
		custom_screen_stack_list[1] = new_screen;
		if (GX_SUCCESS == custom_animation_start(custom_screen_stack_list, parent, SCREEN_SLIDE_LEFT)) {
			control->gx_screen_stack_control_memory[top] = screen;
			control->gx_screen_stack_control_memory[top + 1] = parent;
			return (GX_SUCCESS);
		} else {
			control->gx_screen_stack_control_top--;
			return (GX_FAILURE);
		}

	} else {
		control->gx_screen_stack_control_top--;
		return (GX_FAILURE);
	}
}

UINT custom_screen_stack_pop(GX_SCREEN_STACK_CONTROL *control)
{
	if (GX_TRUE == custom_animation_busy_check()) {
		return GX_FAILURE;
	}

	INT top;
	GX_WIDGET *screen;
	GX_WIDGET *parent;

	top = control->gx_screen_stack_control_top;

	if (top >= 0) {
		top <<= 1;
		screen = control->gx_screen_stack_control_memory[top];
		parent = control->gx_screen_stack_control_memory[top + 1];

		if (!parent->gx_widget_first_child) {
			return (GX_FAILURE);
		}

		custom_screen_stack_list[0] = parent->gx_widget_first_child;
		custom_screen_stack_list[1] = screen;
		if (GX_SUCCESS == custom_animation_start(custom_screen_stack_list, parent, SCREEN_SLIDE_RIGHT)) {
			control->gx_screen_stack_control_top--;
			return (GX_SUCCESS);
		} else {
			return (GX_FAILURE);
		}

		return (GX_SUCCESS);
	} else {
		return (GX_FAILURE);
	}
}