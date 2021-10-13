#include "ux/ux_api.h"


const GX_ANIMATION_INFO popup_show_animation[] = {
    {
        .gx_animation_style = GX_ANIMATION_ELASTIC_EASE_IN_OUT,
        .gx_animation_id = 0x1001,
        .gx_animation_start_delay = 0,
        .gx_animation_frame_interval = 1,
        .gx_animation_start_position = {0, 0},
        .gx_animation_end_position = MOV_DOWN_DIRECTION,
        .gx_animation_start_alpha = 255,
        .gx_animation_end_alpha = 255,
        .gx_animation_steps = 15
    }
};

const GX_ANIMATION_INFO popup_hide_animation[] = {
    {
        .gx_animation_style = GX_ANIMATION_ELASTIC_EASE_IN_OUT,
        .gx_animation_id = 0x1001,
        .gx_animation_start_delay = 0,
        .gx_animation_frame_interval = 1,
        .gx_animation_start_position = {0, 0},
        .gx_animation_end_position = MOV_UP_DIRECTION,
        .gx_animation_start_alpha = 255,
        .gx_animation_end_alpha = 255,
        .gx_animation_steps = 15
    }
};