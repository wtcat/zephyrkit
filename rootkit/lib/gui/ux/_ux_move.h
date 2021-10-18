#ifndef UX_UX_MOVE_H_
#define UX_UX_MOVE_H_

#ifdef __cplusplus
extern "C"{
#endif

#define MOV_DEFAULT_ID 0xDEAD

/*
 * Move direction 
 */
#define MOV_LEFT_DIRECTION   {-1,  0 }
#define MOV_RIGHT_DIRECTION  { 1,  0 }
#define MOV_UP_DIRECTION     { 0, -1 }
#define MOV_DOWN_DIRECTION   { 0,  1 }
#define MOV_ZERO_DIRECTION   { 0,  0 }

extern const GX_POINT _ux_dir_left;
extern const GX_POINT _ux_dir_up;
extern const GX_POINT _ux_dir_right;
extern const GX_POINT _ux_dir_down;
extern const GX_POINT _ux_dir_zero;

#define move_animation_setup(ani, id, style, dir, steps) \
    _move_animation_setup(ani, id, style, dir, steps, 1)
#define _move_animation_setup(ani, id, style, dir, steps, interval) \
    __move_animation_setup(ani, id, style, dir, steps, interval, 255, 255)
#define __move_animation_setup(ani, id, style, dir, steps, interval, alpha0, alpha1) \
do { \
    GX_ANIMATION_INFO *__ani = ani; \
    __ani->gx_animation_style = style; \
    __ani->gx_animation_id = id; \
    __ani->gx_animation_start_delay = 0; \
    __ani->gx_animation_frame_interval = interval; \
    __ani->gx_animation_start_position =  (GX_POINT){ 0, 0 }; \
    __ani->gx_animation_end_position = dir; \
    __ani->gx_animation_start_alpha = alpha0; \
    __ani->gx_animation_end_alpha = alpha1; \
    __ani->gx_animation_steps = steps; \
} while (0)

extern const GX_ANIMATION_INFO _ux_move_down_animation[];
extern const GX_ANIMATION_INFO _ux_move_up_animation[];
extern const GX_ANIMATION_INFO _ux_move_left_animation[];
extern const GX_ANIMATION_INFO _ux_move_right_animation[];
extern const GX_ANIMATION_INFO _ux_move_alpha_animation[];

UINT __ux_animation_run(GX_WIDGET *parent, GX_WIDGET *from, GX_WIDGET *to, 
    const GX_ANIMATION_INFO *info);

#ifdef __cplusplus
}
#endif
#endif /* UX_UX_MOVE_H_ */