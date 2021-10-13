#include <stdlib.h>

#include "ux/ux_api.h"
#include "gx_widget.h"
#include "gx_canvas.h"

#include "popup/widget.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"

#define DELTA_ANGLE 30
#define ICON_GROUPS 11

struct goal_widget {
    GX_WIDGET widget;
    GX_WIDGET circle;
    GX_PROMPT value;
    GX_PROMPT unit;
    union {
        GX_PROMPT tips;
        GX_MULTI_LINE_TEXT_VIEW view;
    };
    GX_RECTANGLE client;
    GX_RESOURCE_ID icon;
    GX_RESOURCE_ID color;
    GX_POINT center;
    GX_VALUE r;
    GX_VALUE curr_angle;
    GX_VALUE next_angle;
    INT target_value;
    INT curr_value;
    GX_BOOL is_view;
    void (*draw)(struct goal_widget *goal);
    void (*timer_expired)(GX_WIDGET *widget, struct goal_widget *goal);
};

struct icon_descs {
    GX_RECTANGLE rect[3];
    GX_RESOURCE_ID ids[ICON_GROUPS][3];
};

static struct goal_widget goal_arrived_widget;
static float float_delta;
static float float_current;
USTR_START(goal)
_USTR(LANGUAGE_ENGLISH, "Goal Arrived", "min", "hour", "Goal Arrived All"),
#ifndef _WIN32
_USTR(LANGUAGE_CHINESE, "Ŀ�����", "����")
#endif
USTR_END(goal)


static struct icon_descs icon_desc_table = {
    .rect = {
        {34,     10,     34+126, 10+161},
        {34+126, 10,     320-34, 10+161},
        {53,     10+161, 53+214, 10+161+91}
    },
    .ids = {
        {GX_PIXELMAP_ID_WORK_OUT_A, GX_PIXELMAP_ID_ACTIVITY_A, GX_PIXELMAP_ID_STEP_A},
        {GX_PIXELMAP_ID_WORK_OUT_B, GX_PIXELMAP_ID_ACTIVITY_B, GX_PIXELMAP_ID_STEP_B},
        {GX_PIXELMAP_ID_WORK_OUT_C, GX_PIXELMAP_ID_ACTIVITY_C, GX_PIXELMAP_ID_STEP_C},
        {GX_PIXELMAP_ID_WORK_OUT_D, GX_PIXELMAP_ID_ACTIVITY_D, GX_PIXELMAP_ID_STEP_D},
        {GX_PIXELMAP_ID_WORK_OUT_E, GX_PIXELMAP_ID_ACTIVITY_E, GX_PIXELMAP_ID_STEP_E},
        {GX_PIXELMAP_ID_WORK_OUT_F, GX_PIXELMAP_ID_ACTIVITY_F, GX_PIXELMAP_ID_STEP_F},
        {GX_PIXELMAP_ID_WORK_OUT_G, GX_PIXELMAP_ID_ACTIVITY_G_1_, GX_PIXELMAP_ID_STEP_G},
        {GX_PIXELMAP_ID_WORK_OUT_H, GX_PIXELMAP_ID_ACTIVITY_H, GX_PIXELMAP_ID_STEP_H},
        {GX_PIXELMAP_ID_WORK_OUT_I, GX_PIXELMAP_ID_ACTIVITY_G, GX_PIXELMAP_ID_STEP_I},
        {GX_PIXELMAP_ID_WORK_OUT_J, GX_PIXELMAP_ID_ACTIVITY_I, GX_PIXELMAP_ID_STEP_J},
        {GX_PIXELMAP_ID_WORK_OUT_K, GX_PIXELMAP_ID_ACTIVITY_J, GX_PIXELMAP_ID_STEP_K}
    }
};

extern char *itoa (int value, char *str, int base );

static void prompt_value_update(GX_PROMPT *prompt, INT value)
{
    static char buffer[16];
    GX_STRING str;

    memset(buffer, 0, sizeof(buffer));
    str.gx_string_ptr = itoa(value, buffer, 10);
    str.gx_string_length = strlen(str.gx_string_ptr);
    gx_prompt_text_set_ext(prompt, &str);
}

static void goal_draw_1_3(struct goal_widget *goal)
{
    if (goal->next_angle != goal->curr_angle) {
        ux_arc_progress_render(goal->center.gx_point_x, goal->center.gx_point_y,
            goal->r, 20, goal->curr_angle, goal->next_angle, goal->color);
        goal->curr_angle = goal->next_angle;
    }
    ux_rectangle_pixelmap_draw(&goal->client, goal->icon,
        GX_STYLE_HALIGN_CENTER | GX_STYLE_VALIGN_CENTER);
}

static void goal_timer_fire_1_3(GX_WIDGET *widget, struct goal_widget *goal)
{
    if (goal->next_angle < 360) {
        if (goal->curr_angle == goal->next_angle) {
            goal->next_angle = goal->curr_angle + DELTA_ANGLE;
            float_current += float_delta;
            if (float_current > 1.0f) {
                INT iv = (INT)float_current;
                float_current -= iv;
                goal->curr_value += iv;
            }
            prompt_value_update(&goal->value, goal->curr_value);
            gx_system_dirty_mark(&goal->circle);
        }
    } else {
        prompt_value_update(&goal->value, goal->target_value);
        gx_system_timer_stop(widget, UX_TIMER_ID(widget));
    }
}

static void goal_draw_4(struct goal_widget *goal)
{
    if (goal->next_angle) {
        for (int i = 0; i < 3; i++) {
            ux_rectangle_pixelmap_draw(&icon_desc_table.rect[i],
                icon_desc_table.ids[goal->next_angle][i], 0);
        }
    }
}

static void goal_timer_fire_4(GX_WIDGET *widget, struct goal_widget *goal)
{
    if (goal->next_angle < ICON_GROUPS-1) {
        goal->target_value++;
        if (!(goal->target_value & 3)) {
            goal->next_angle++;
            gx_system_dirty_mark(&goal->circle);
        }
    } else {
        gx_system_timer_stop(widget, UX_TIMER_ID(widget));
    }
}

static void goal_prompt_tips_create(GX_WIDGET *parent, GX_PROMPT *prompt)
{
    GX_RECTANGLE client;

    memset(prompt, 0, sizeof(GX_PROMPT));
    gx_utility_rectangle_define(&client, 85, 282, 320 - 85, 360 - 33);
    gx_prompt_create(prompt, "tips", parent, 0,
        GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(prompt, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(prompt, GX_FONT_ID_SIZE_26);
    gx_widget_fill_color_set(prompt, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    ux_widget_string_set(goal, 0, prompt, gx_prompt_text_set_ext);
}

static void goal_multi_text_tips_create(GX_WIDGET *parent, GX_MULTI_LINE_TEXT_VIEW *view)
{
    GX_RECTANGLE client;

    memset(view, 0, sizeof(GX_MULTI_LINE_TEXT_VIEW));
    gx_utility_rectangle_define(&client, 70, 256+20, 320 - 70, 360 - 34 + 20);
    gx_multi_line_text_view_create(view, "goal-text", parent,
        0, GX_STYLE_TEXT_CENTER, 0, &client);
    gx_multi_line_text_view_text_color_set(view, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_multi_line_text_view_font_set(view, GX_FONT_ID_SIZE_26);
    gx_widget_fill_color_set(view, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    ux_widget_string_set(goal, 3, view, gx_multi_line_text_view_text_set_ext);
}

static void goal_create_tips(struct goal_widget *goal, GX_BOOL prompt_tips)
{
    GX_RECTANGLE client;

    if (prompt_tips) {
        if (goal->is_view) {
            goal->is_view = GX_FALSE;
            _gx_widget_detach((GX_WIDGET *)&goal->view);
            goal_prompt_tips_create(&goal->widget, &goal->tips);
            gx_utility_rectangle_define(&client, 81 - 30, 30, 320 - 80 + 30, 360 - 153 + 30);
            gx_widget_resize(&goal->circle, &client);
        }
        goal->draw = goal_draw_1_3;
        goal->timer_expired = goal_timer_fire_1_3;
    } else {
        if (!goal->is_view) {
            goal->is_view = GX_TRUE;
            _gx_widget_detach((GX_WIDGET *)&goal->tips);
            goal_multi_text_tips_create(&goal->widget, &goal->view);
            gx_utility_rectangle_define(&client, 34, 10, 320 - 34, 360 - 79);
            gx_widget_resize(&goal->circle, &client);
        }
        goal->draw = goal_draw_4;
        goal->timer_expired = goal_timer_fire_4;
    }
}

static void goal_arrived_draw(GX_WIDGET *widget)
{
    struct goal_widget *goal = CONTAINER_OF(widget, 
        struct goal_widget, circle);

    goal->draw(goal);
    _gx_widget_children_draw(&goal->circle);
}

static UINT goal_arrived_event_process(struct goal_widget *goal,
    GX_EVENT *event_ptr)
{
    GX_WIDGET *widget = &goal->widget;

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_SHOW:
        goal->next_angle = goal->curr_angle = 0;
        break;
    case GX_EVENT_TIMER:
        if (event_ptr->gx_event_payload.gx_event_timer_id ==
            UX_TIMER_ID(widget)) {
            goal->timer_expired(widget, goal);
            return GX_SUCCESS;
        } else if (event_ptr->gx_event_payload.gx_event_timer_id ==
            POP_WIDGET_TIMER_ID) {
            gx_system_timer_stop(widget, UX_TIMER_ID(widget));
        }
        break;
    case GX_EVENT_KEY_UP:
        if (event_ptr->gx_event_payload.gx_event_ushortdata[0] ==
            GX_KEY_HOME) {
            gx_system_timer_stop(widget, UX_TIMER_ID(widget));
        }
        break;
    case GX_EVENT_ANIMATION_COMPLETE:
        if (widget->gx_widget_status & GX_STATUS_VISIBLE)
            gx_system_timer_start(widget, UX_TIMER_ID(widget), 1, 1);
        break;
    case UX_EVENT_STOP:
        gx_system_timer_stop(widget, UX_TIMER_ID(widget));
        gx_system_timer_stop(widget, POP_WIDGET_TIMER_ID);
        break;
    default:
        break;
    }
    return pop_widget_event_process_wrapper(&goal->widget, event_ptr,
        1000, _gx_widget_event_process, popup_hide_animation);
}

GX_WIDGET *goal_arrived_widget_get(enum goal_type type, INT value)
{
    struct goal_widget *goal = &goal_arrived_widget;
    GX_RECTANGLE client;

    goal->curr_value = 0;
    float_current = 0;
    switch (type) {
    case GOAL_1:
        goal->target_value = value;
        goal->color = GX_COLOR_ID_HONBOW_GREEN;
        _gx_widget_detach((GX_WIDGET *)&goal->unit);
        _gx_widget_attach(&goal->widget, (GX_WIDGET *)&goal->value);
        gx_utility_rectangle_define(&client, 95, 236, 320 - 95, 360 - 78);
        gx_widget_resize(&goal->value, &client);
        gx_widget_style_set(&goal->value, GX_STYLE_TEXT_CENTER);
        prompt_value_update(&goal->value, 0);
        goal_create_tips(goal, GX_TRUE);
        float_delta = (float)goal->target_value * (DELTA_ANGLE / 360.0f);
        break;
    case GOAL_2:
    case GOAL_3:
        goal->target_value = value;
        goal->color = GX_COLOR_ID_HONBOW_ORANGE;
        _gx_widget_attach(&goal->widget, (GX_WIDGET *)&goal->value);
        _gx_widget_attach(&goal->widget, (GX_WIDGET *)&goal->unit);
        gx_utility_rectangle_define(&client, 85, 236, 320 - 161, 360 - 79);
        gx_widget_resize(&goal->value, &client);
        gx_widget_style_set(&goal->value, GX_STYLE_TEXT_RIGHT);
        prompt_value_update(&goal->value, 0);
        ux_widget_string_set(goal, type, &goal->unit, gx_prompt_text_set_ext);
        goal_create_tips(goal, GX_TRUE);
        float_delta = (float)goal->target_value * (DELTA_ANGLE / 360.0f);
        break;
    case GOAL_4:
        _gx_widget_detach((GX_WIDGET *)&goal->value);
        _gx_widget_detach((GX_WIDGET *)&goal->unit);
        goal_create_tips(goal, GX_FALSE);
        break;
    default:
        break;
    }
    return &goal->widget;
}

UINT goal_arrived_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
    struct goal_widget *goal = &goal_arrived_widget;
    GX_RECTANGLE client;

    /* Setup widget base class */
    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_widget_create(&goal->widget, "goal", parent,
        GX_STYLE_TRANSPARENT | GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&goal->widget, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_event_process_set(&goal->widget, goal_arrived_event_process);

    /* Setup circle widet */
    gx_utility_rectangle_define(&client, 81 - 30, 30, 320 - 80 + 30, 360 - 155+30);
    gx_widget_create(&goal->circle, "circle", &goal->widget,
        GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&goal->circle, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_draw_set(&goal->circle, goal_arrived_draw);

    goal->center.gx_point_x = (goal->circle.gx_widget_size.gx_rectangle_left +
        goal->circle.gx_widget_size.gx_rectangle_right + 1) >> 1;
    goal->center.gx_point_y = (goal->circle.gx_widget_size.gx_rectangle_top +
        goal->circle.gx_widget_size.gx_rectangle_bottom + 1) >> 1;
    goal->r = 70;

    /* Value */
    gx_utility_rectangle_define(&client, 95, 236, 320 - 95, 360 - 78);
    gx_prompt_create(&goal->value, "value", &goal->widget, 0,
        GX_STYLE_TEXT_RIGHT, 0, &client);
    gx_widget_fill_color_set(&goal->value, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_prompt_text_color_set(&goal->value, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&goal->value, GX_FONT_ID_SIZE_30);

    /* Unit */
    gx_utility_rectangle_define(&client, 160 + 10, 245, 320 - 99 + 10, 360 - 85);
    gx_prompt_create(&goal->unit, "unit", &goal->widget, 0,
        GX_STYLE_TEXT_LEFT, 0, &client);
    gx_widget_fill_color_set(&goal->unit, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_prompt_text_color_set(&goal->unit, GX_COLOR_ID_HONBOW_GRAY,
        GX_COLOR_ID_HONBOW_GRAY, GX_COLOR_ID_HONBOW_GRAY);
    gx_prompt_font_set(&goal->unit, GX_FONT_ID_SIZE_26);

    /* Goal tips */
    goal_prompt_tips_create(&goal->widget, &goal->tips);
    goal->is_view = GX_FALSE;
    if (ptr)
        *ptr = &goal->widget;
    return GX_SUCCESS;
}
