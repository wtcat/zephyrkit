#include "gx_api.h"
#include "gx_widget.h"

#include "ux/_ux_action.h"
#include "ux/_ux_utils.h"
#include "ux/_ux_pop.h"

void pop_widget_fire(GX_WIDGET *curr, GX_WIDGET *next, 
    const GX_ANIMATION_INFO *info)
{
    GX_WIDGET *parent = curr->gx_widget_parent;

    if (curr == next)
        return;
    if (next->gx_widget_status & GX_STATUS_VISIBLE)
        return;
    if (gx_system_screen_stack_push(curr) == GX_SUCCESS)
        __ux_screen_move(parent, curr, next, info);
}
