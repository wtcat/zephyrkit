#ifndef UX_UX_API_H_
#define UX_UX_API_H_

#include "gx_api.h"
#include "gx_system.h"
#include "gx_widget.h"

#include "ux/_ux_animation.h"
#include "ux/_ux_button.h"
#include "ux/_ux_utils.h"
#include "ux/_ux_action.h"
#include "ux/_ux_pop.h"
#include "ux/_ux_wrapper.h"
#include "ux/_ux_move.h"

#define _UX_EVENT_TYPE(ev) ((ev) + (GX_FIRST_APP_EVENT + 10000))

/*
 * User event type
 */
#define UX_EVENT_STOP _UX_EVENT_TYPE(0)

#if 0
#define UX_BUSY_TIMERID 0xFFFF0000

#define _ux_animation_assert(widget, event_ptr) \
do { \
    if (_gx_system_animation_list) { \
        _gx_system_timer_start(widget, UX_BUSY_TIMERID, 1, 0); \
        return GX_FAILURE; \
    } \
} while (0)

#define _ux_animation_
if (event_ptr->gx_event_type == GX_EVENT_TIMER) {
    if (event_ptr->gx_event_payload.gx_event_timer_id == UX_BUSY_TIMERID)
        _ux_animation_assert(widget);
}
#endif

#endif /* UX_UX_API_H_ */
