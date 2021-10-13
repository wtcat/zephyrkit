#ifndef _WIN32
#include <sys/__assert.h>
#endif

#include "gx_api.h"
#include "gx_system.h"
#include "gx_widget.h"

#include "ux/_ux_utils.h"
#include "ux/_ux_move.h"


struct animation_complete {
    UINT(*event_process)(GX_WIDGET *, GX_EVENT *);
    GX_VALUE x_ofs;
    GX_VALUE y_ofs;
    struct animation_complete *next;
};



static struct animation_complete _ani_complete_memory[GX_ANIMATION_POOL_SIZE];
static struct animation_complete *_ani_complete_free;
static bool _ani_complete_ready;

const GX_POINT _ux_dir_left  = MOV_LEFT_DIRECTION;
const GX_POINT _ux_dir_up    = MOV_UP_DIRECTION;
const GX_POINT _ux_dir_right = MOV_RIGHT_DIRECTION;
const GX_POINT _ux_dir_down  = MOV_DOWN_DIRECTION;
const GX_POINT _ux_dir_zero  = MOV_ZERO_DIRECTION;
const GX_ANIMATION_INFO _ux_move_down_animation[] = {
    {
        .gx_animation_style = 0,
        .gx_animation_id = 0x1001,
        .gx_animation_start_delay = 0,
        .gx_animation_frame_interval = 1,
        .gx_animation_start_position = {0, 0},
        .gx_animation_end_position = MOV_DOWN_DIRECTION,
        .gx_animation_start_alpha = 255,
        .gx_animation_end_alpha = 255,
        .gx_animation_steps = 10
    }
};
const GX_ANIMATION_INFO _ux_move_up_animation[] = {
    {
        .gx_animation_style = 0,
        .gx_animation_id = 0x1001,
        .gx_animation_start_delay = 0,
        .gx_animation_frame_interval = 1,
        .gx_animation_start_position = {0, 0},
        .gx_animation_end_position = MOV_UP_DIRECTION,
        .gx_animation_start_alpha = 255,
        .gx_animation_end_alpha = 255,
        .gx_animation_steps = 10
    }
};
const GX_ANIMATION_INFO _ux_move_left_animation[] = {
    {
        .gx_animation_style = 0,
        .gx_animation_id = 0x1001,
        .gx_animation_start_delay = 0,
        .gx_animation_frame_interval = 1,
        .gx_animation_start_position = {0, 0},
        .gx_animation_end_position = MOV_LEFT_DIRECTION,
        .gx_animation_start_alpha = 255,
        .gx_animation_end_alpha = 255,
        .gx_animation_steps = 10
    }
};
const GX_ANIMATION_INFO _ux_move_right_animation[] = {
    {
        .gx_animation_style = 0,
        .gx_animation_id = 0x1001,
        .gx_animation_start_delay = 0,
        .gx_animation_frame_interval = 1,
        .gx_animation_start_position = {0, 0},
        .gx_animation_end_position = MOV_RIGHT_DIRECTION,
        .gx_animation_start_alpha = 255,
        .gx_animation_end_alpha = 255,
        .gx_animation_steps = 10
    }
};
const GX_ANIMATION_INFO _ux_move_alpha_animation[] = {
    {
        .gx_animation_style = 0,
        .gx_animation_id = 0x1001,
        .gx_animation_start_delay = 0,
        .gx_animation_frame_interval = 1,
        .gx_animation_start_position = {0, 0},
        .gx_animation_end_position = MOV_ZERO_DIRECTION,
        .gx_animation_start_alpha = 0,
        .gx_animation_end_alpha = 255,
        .gx_animation_steps = 3
    }
};

static void animation_complete_init(void)
{
    struct animation_complete **iter = &_ani_complete_free;

    for (int i = 0; i < ARRAY_SIZE(_ani_complete_memory); i++) {
        *iter = &_ani_complete_memory[i];
        iter = &(*iter)->next;
    }
}

static struct animation_complete *animation_complete_alloc(void)
{
    struct animation_complete *ptr;

    if (!_ani_complete_ready) {
        animation_complete_init();
        _ani_complete_ready = true;
    }
    GX_ENTER_CRITICAL
    ptr = _ani_complete_free;
    if (ptr) {
        _ani_complete_free = ptr->next;
        ptr->next = NULL;
    }
    GX_EXIT_CRITICAL
    return ptr;
}

static void animation_complete_free(struct animation_complete *ptr)
{
    GX_ENTER_CRITICAL
    ptr->next = _ani_complete_free;
    _ani_complete_free = ptr;
    GX_EXIT_CRITICAL
}

static UINT ux_animation_event_wrapper(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
    struct animation_complete *cmp = 
        (struct animation_complete *)widget->gx_widget_user_data;

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_ANIMATION_COMPLETE:
        if (widget->gx_widget_id == MOV_DEFAULT_ID)
            widget->gx_widget_id = 0;
        gx_widget_event_process_set(widget, cmp->event_process);
        if (!(widget->gx_widget_status & GX_STATUS_VISIBLE)) {
            _gx_widget_detach(widget);
            _gx_widget_shift(widget, 
                cmp->x_ofs - widget->gx_widget_size.gx_rectangle_left,
                cmp->y_ofs - widget->gx_widget_size.gx_rectangle_top,
                GX_FALSE);
        }
        animation_complete_free(cmp);
        break;
    default:
        break;
    }
    return cmp->event_process(widget, event_ptr);
}

static void ux_animation_event_override(GX_WIDGET *widget, 
    struct animation_complete *cmp)
{
    cmp->event_process = widget->gx_widget_event_process_function;
    widget->gx_widget_event_process_function = ux_animation_event_wrapper;
    widget->gx_widget_user_data = (INT)cmp;
}

UINT __ux_animation_run(GX_WIDGET *parent, GX_WIDGET *from, GX_WIDGET *to, 
    const GX_ANIMATION_INFO *info)
{
    const GX_POINT *start = &info->gx_animation_start_position;
    const GX_POINT *end = &info->gx_animation_end_position;
    struct animation_complete *from_cmp, *to_cmp;
    GX_ANIMATION *ani_from, *ani_to;
    GX_ANIMATION_INFO ani[2];
    GX_VALUE width;
    GX_VALUE height;
    USHORT style;
    UINT ret;

    _gx_system_animation_get(&ani_from);
    if (_gx_system_animation_get(&ani_to)) {
        _gx_system_animation_free(ani_from);
        return GX_OUT_OF_ANIMATIONS;
    }
    from_cmp = animation_complete_alloc();
    to_cmp = animation_complete_alloc();
#ifndef _WIN32
    __ASSERT(from_cmp != NULL, "");
    __ASSERT(to_cmp != NULL, "");
#endif
    GX_VALUE sign_x = end->gx_point_x - start->gx_point_x;
    GX_VALUE sign_y = end->gx_point_y - start->gx_point_y;
    width = (from->gx_widget_size.gx_rectangle_right -
        from->gx_widget_size.gx_rectangle_left + 1) * sign_x;
    height = (from->gx_widget_size.gx_rectangle_bottom -
        from->gx_widget_size.gx_rectangle_top + 1) * sign_y;

    style = info->gx_animation_style & ~GX_ANIMATION_DETACH;
    ani[0] = *info;
    ani[0].gx_animation_parent = parent;
    ani[0].gx_animation_start_alpha = info->gx_animation_end_alpha;
    ani[0].gx_animation_end_alpha = info->gx_animation_start_alpha;
    ani[0].gx_animation_start_position.gx_point_x = from->gx_widget_size.gx_rectangle_left;
    ani[0].gx_animation_start_position.gx_point_y = from->gx_widget_size.gx_rectangle_top;
    ani[0].gx_animation_end_position.gx_point_x = from->gx_widget_size.gx_rectangle_left + width;
    ani[0].gx_animation_end_position.gx_point_y = from->gx_widget_size.gx_rectangle_top + height;
    ani[0].gx_animation_target = from;
    ani[0].gx_animation_style = GX_ANIMATION_TRANSLATE | GX_ANIMATION_DETACH | style;
    from_cmp->x_ofs = from->gx_widget_size.gx_rectangle_left;
    from_cmp->y_ofs = from->gx_widget_size.gx_rectangle_top;
    
    //width = (to->gx_widget_size.gx_rectangle_right -
    //    to->gx_widget_size.gx_rectangle_left + 1) * sign_x;
    //height = (to->gx_widget_size.gx_rectangle_bottom -
    //    to->gx_widget_size.gx_rectangle_top + 1) * sign_y;
    ani[1] = *info;
    ani[1].gx_animation_parent = parent;
    ani[1].gx_animation_start_position.gx_point_x = to->gx_widget_size.gx_rectangle_left - width;
    ani[1].gx_animation_start_position.gx_point_y = to->gx_widget_size.gx_rectangle_top - height;
    ani[1].gx_animation_end_position.gx_point_x = to->gx_widget_size.gx_rectangle_left;
    ani[1].gx_animation_end_position.gx_point_y = to->gx_widget_size.gx_rectangle_top;
    ani[1].gx_animation_target = to;
    ani[1].gx_animation_id += 1;
    ani[1].gx_animation_style = style;
    to_cmp->x_ofs = to_cmp->y_ofs = 0;

    ux_animation_event_override(from, from_cmp);
    ux_animation_event_override(to, to_cmp);
    ret = gx_animation_start(ani_from, &ani[0]);
    if (ret)
        goto _free_cmp;
    ret = gx_animation_start(ani_to, &ani[1]);
    if (ret) {
        gx_animation_stop(ani_from);
        goto _free_cmp;
    }
    if (!from->gx_widget_id)
        from->gx_widget_id = MOV_DEFAULT_ID;
    if (!to->gx_widget_id)
        to->gx_widget_id = MOV_DEFAULT_ID;
    return ret;

_free_cmp:
    animation_complete_free(to_cmp);
    animation_complete_free(from_cmp);
    _gx_system_animation_free(ani_to);
    _gx_system_animation_free(ani_from);
    return ret;
}
