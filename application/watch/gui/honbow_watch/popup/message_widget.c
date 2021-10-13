#include "ux/ux_api.h"
#include "gx_widget.h"
#include "gx_context.h"
#include "gx_window.h"
#include "gx_system.h"
#include "gx_scrollbar.h"

#include "base/list.h"

#include "app_list/app_list_define.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"

#include <stdio.h>

struct message_window;

struct message_icon {
    GX_WIDGET widget;
    GX_WIDGET client;
    GX_PROMPT tips;
    GX_RESOURCE_ID pixmap;
};

struct delete_dialog {
    UX_PIXELMAP_BUTTON cancel;
    UX_PIXELMAP_BUTTON enter;
    GX_PROMPT info;
    GX_WIDGET widget;
};

struct message_data {
    uint32_t timestamp;
    uint32_t type;
    char *name;
    char *text;
    size_t len;
};

struct message_widget {
    GX_WIDGET widget;
    GX_PROMPT type;
    GX_PROMPT time;
    GX_PROMPT name;
    GX_MULTI_LINE_TEXT_VIEW text;
    GX_RESOURCE_ID icon;
    GX_VALUE x_start;
    GX_VALUE y_start;
    struct list_head node;
    const struct message_data *data;
    UINT (*state)(struct message_widget *, GX_WIDGET *, GX_EVENT *);
    struct message_window *win;
};

struct message_viewer {
    struct message_widget widget;
    GX_SCROLLBAR vscrollbar;
    struct message_widget *mw;
};

struct message_notifier {
    GX_WIDGET main;
    struct message_widget widget;
    UX_TEXT_BUTTON cmp_button;
    UX_TEXT_BUTTON ign_button;
};

struct message_window {
    GX_VERTICAL_LIST list;
    GX_SCROLLBAR vscrollbar;
    struct message_viewer viewer;
    struct message_notifier notifier;
    struct message_icon empty;
    struct message_icon news;
    struct delete_dialog dialog;
    UX_PIXELMAP_BUTTON clr_button;
    UX_PIXELMAP_BUTTON del_button;
    struct message_widget *del_widget;
    GX_WIDGET *parent;
    INT y_clrbtn_origin;
    INT y_clrbtn_limit;
    INT y_list_limit;
    INT x_left_limit;
    INT x_right_limit;
    UINT flags;
#define MSG_DELBTN_BUSY    0x0001
#define MSG_DELBTN_DEL     0x0002
#define MSG_CLRBTN_BUSY    0x0004
#define MSG_CLRBTN_CLR     0x0008

#define MSG_VIEWER_OPENED  0x0080
};

#define MESSAGE_CLEAR_BUTTON_HEIGHT 65

#define MESSAGE_CLEAR_BUTTON_ID        0x8001
#define MESSAGE_DELETE_BUTTON_ID       0x8002
#define MESSAGE_CANCEL_BUTTON_ID       0x8003
#define MESSAGE_ENTER_BUTTON_ID        0x8004
#define MESSAGE_COMPLETE_BUTTON_ID     0x8005
#define MESSAGE_IGNORE_BUTTON_ID       0x8006

#define MESSAGE_HDRAG_ANIMATION_ID     0x9001

#define MESSAGE_DELETE_BUTTON_TIMER_ID 0x1001 
#define MESSAGE_CLEAR_BUTTON_TIMER_ID  0x1002

/* Message widget allocate flags */
#define MW_ALLOC_APPEND_ACTIVE 0x01

#define MAX_MESSAGES 10

static struct message_widget message_pool[MAX_MESSAGES];
static LIST_HEAD(active_list);
static LIST_HEAD(free_list);
static struct message_window message_win;
static const GX_RECTANGLE message_viewer_size = {
    .gx_rectangle_left = 12,
    .gx_rectangle_top = 12,
    .gx_rectangle_right = 12 + 296,
    .gx_rectangle_bottom = 12 + 260
};
static GX_ANIMATION_INFO message_alpha_animation = {
    (GX_WIDGET *)NULL,
    (GX_WIDGET *)NULL,
    GX_NULL,
    GX_ANIMATION_TRANSLATE, 0, 0, 1,
    {0, 0}, {0, 0}, 0, 255, 2
};

#define _TEST_TEXT \
    "weather is bad, it\n"\
    "may rain, take an\n" \
    "umbrella when you\n" \
    "go out."

#define _TEST_TEXT_2 \
    "Uniprocessor systems have long been used in embedded systems." \
    "In this hardware model, there are some system execution " \
    "characteristics which have long been taken for granted"

//FOR test
static struct message_data _test_data_array[] = {
    {
        .type = GX_PIXELMAP_ID_IC_WECHAT,
        .name = "TOM:",
        .text = _TEST_TEXT,
        .len = sizeof(_TEST_TEXT) - 1
    },{
        .type = GX_PIXELMAP_ID_IC_MESSENGER,
        .text = _TEST_TEXT_2,
        .len = sizeof(_TEST_TEXT_2) - 1
    }, {
        .type = GX_PIXELMAP_ID_IC_QQ,
        .name = "TOM:",
        .text = _TEST_TEXT,
        .len = sizeof(_TEST_TEXT) - 1
    }, {
        .type = GX_PIXELMAP_ID_IC_SMS,
        .name = "LUCY:",
        .text = _TEST_TEXT,
        .len = sizeof(_TEST_TEXT) - 1
    }, {
        .type = GX_PIXELMAP_ID_IC_TWITTER,
        .name = "LILY:",
        .text = _TEST_TEXT,
        .len = sizeof(_TEST_TEXT) - 1
    }, {
        .type = GX_PIXELMAP_ID_IC_WHATSAPP,
        .name = "JACK:",
        .text = _TEST_TEXT,
        .len = sizeof(_TEST_TEXT) - 1
    }
};

/*
 * Message language strings
 */
USTR_START(message)
    _USTR(LANGUAGE_ENGLISH, "Clear All?", "Complete", "Ignore", "No Messages"),
#ifndef _WIN32
    _USTR(LANGUAGE_CHINESE, "全部清除?",    "完成",     "忽略",    "无消息")
#endif
USTR_END(message)


static void message_widget_switch(GX_WIDGET *from, GX_WIDGET *to, GX_ANIMATION_INFO *ani)
{
    GX_WIDGET *parent = from->gx_widget_parent;
    if (parent) {
        struct ux_action action = {
            .flags = 0,
            .animation = ani
        };
        ani->gx_animation_parent = parent;
        ani->gx_animation_target = to;
        ani->gx_animation_start_position.gx_point_x = to->gx_widget_size.gx_rectangle_left;
        ani->gx_animation_start_position.gx_point_y = to->gx_widget_size.gx_rectangle_top;
        ani->gx_animation_end_position = ani->gx_animation_start_position;
        ux_screen_move(from, to, NULL);
        ux_animation_execute(NULL, &action);
    } 
}

static inline GX_RESOURCE_ID message_icon_get(const struct message_data *data)
{
    return data->type;//TODO: 
}

static inline void message_widget_flags_set(struct message_window *win, UINT flags)
{
    win->flags |= flags;
}

static inline void message_widget_flags_clr(struct message_window *win, UINT flags)
{
    win->flags &= ~flags;
}

static inline int message_widget_flags_tst(struct message_window *win, UINT flags)
{
    return win->flags & flags;
}

static UINT message_widget_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr);

static inline void message_list_request_refresh(void)
{
    message_win.list.gx_vertical_list_child_count = 0;
}

static struct message_widget *message_widget_alloc(UINT flags)
{
    struct message_widget *widget;
    struct list_head *p;

    GX_ENTER_CRITICAL
    if (!list_empty(&free_list)) {
        p = free_list.next;
        widget = container_of(p, struct message_widget, node);
    }
    else {
        p = active_list.next;
        widget = container_of(p, struct message_widget, node);
        gx_widget_detach(widget);
        message_list_request_refresh();
    }
    list_del(p);
    if (flags & MW_ALLOC_APPEND_ACTIVE)
        list_add_tail(p, &active_list);
    GX_EXIT_CRITICAL
    return widget;
}

static inline bool message_active_empty(void)
{
    return list_empty_careful(&active_list);
}

static void message_all_clear(void)
{
    struct message_widget *widget;
    struct list_head *p;

    GX_ENTER_CRITICAL
        list_for_each(p, &active_list) {
        widget = container_of(p, struct message_widget, node);
        gx_widget_detach(widget);
    }
    list_splice_init(&active_list, &free_list);
    GX_EXIT_CRITICAL
    message_list_request_refresh();
}

static void message_delete(struct message_widget *mw)
{
    gx_widget_detach(&mw->widget);
    list_del(&mw->node);
    list_add_tail(&mw->node, &free_list);
    message_list_request_refresh();
}

static void message_widget_draw(GX_WIDGET *widget)
{
    struct message_widget *mw = CONTAINER_OF(widget, struct message_widget, widget);
    GX_RECTANGLE client;

    ux_round_rectangle_render2(&widget->gx_widget_size, 6, 76,
        GX_COLOR_ID_SHADOW, GX_COLOR_ID_HONBOW_DARK_GRAY);
    client.gx_rectangle_left = mw->widget.gx_widget_size.gx_rectangle_left + 10;
    client.gx_rectangle_top = mw->widget.gx_widget_size.gx_rectangle_top + 12;
    ux_rectangle_pixelmap_draw(&client, mw->icon, 0);
    _gx_widget_children_draw(widget);
}

static void message_widget_create(struct message_widget *mw, GX_WIDGET *parent,
    GX_CONST GX_RECTANGLE *size, GX_VALUE text_bottom_gap)
{
    GX_WIDGET *widget = &mw->widget;
    GX_RECTANGLE client;

    gx_widget_create(widget, "message", parent, GX_STYLE_NONE | GX_STYLE_ENABLED,
        0, size);
    widget->gx_widget_normal_fill_color = GX_COLOR_ID_HONBOW_GRAY;
    widget->gx_widget_selected_fill_color = GX_COLOR_ID_HONBOW_GRAY;
    widget->gx_widget_disabled_fill_color = GX_COLOR_ID_HONBOW_GRAY;
    gx_widget_draw_set(widget, message_widget_draw);
    gx_widget_event_process_set(widget, message_widget_event_process);

    /* Message type */
    client.gx_rectangle_left = widget->gx_widget_size.gx_rectangle_left + 66;
    client.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_top + 10;
    client.gx_rectangle_right = widget->gx_widget_size.gx_rectangle_right - 10;
    client.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_top + 30;
    gx_prompt_create(&mw->type, "msg-type", &mw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT, 0, &client);
    gx_prompt_text_color_set(&mw->type, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&mw->type, GX_FONT_ID_SIZE_26);

    /* Time */
    client.gx_rectangle_left = widget->gx_widget_size.gx_rectangle_left + 66;
    client.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_top + 40;
    client.gx_rectangle_right = widget->gx_widget_size.gx_rectangle_right - 10;
    client.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_top + 40 + 26;
    gx_prompt_create(&mw->time, "msg-time", &mw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT, 0, &client);
    gx_prompt_text_color_set(&mw->time, GX_COLOR_ID_HONBOW_GRAY,
        GX_COLOR_ID_HONBOW_GRAY, GX_COLOR_ID_HONBOW_GRAY);
    gx_prompt_font_set(&mw->time, GX_FONT_ID_SIZE_26);

    /* Name */
    client.gx_rectangle_left = widget->gx_widget_size.gx_rectangle_left + 10;
    client.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_top + 80;
    client.gx_rectangle_right = widget->gx_widget_size.gx_rectangle_right - 10;
    client.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_top + 80 + 30;
    gx_prompt_create(&mw->name, "contact", &mw->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT, 0, &client);
    gx_prompt_text_color_set(&mw->name, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&mw->name, GX_FONT_ID_SIZE_26);

    /* Text */
    client.gx_rectangle_left = widget->gx_widget_size.gx_rectangle_left + 10;
    client.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_top + 110;
    client.gx_rectangle_right = widget->gx_widget_size.gx_rectangle_right - 10;
    client.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_bottom - text_bottom_gap;
    gx_multi_line_text_view_create(&mw->text, "msg-text", &mw->widget,
        0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT, 0, &client);
    gx_multi_line_text_view_text_color_set(&mw->text, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_multi_line_text_view_font_set(&mw->text, GX_FONT_ID_SIZE_26);
    gx_multi_line_text_view_line_space_set(&mw->text, 1);
}

static void message_data_attach(struct message_widget *mw, const struct message_data *data)
{
    GX_WIDGET *widget = &mw->widget;
    GX_RECTANGLE client;
    GX_STRING str;

    /* Set message icon */
    mw->icon = message_icon_get(data);

    /* Set type*/
    str.gx_string_ptr = "WeChat";
    str.gx_string_length = strlen(str.gx_string_ptr);
    gx_prompt_text_set_ext(&mw->type, &str);

    /* Set time */
    str.gx_string_ptr = "10:10 AM";
    str.gx_string_length = strlen(str.gx_string_ptr);
    gx_prompt_text_set_ext(&mw->time, &str);

    /* */
    if (data->name) {
        str.gx_string_ptr = data->name;
        str.gx_string_length = strlen(data->name);
        gx_prompt_text_set_ext(&mw->name, &str);
        gx_widget_show(&mw->name);

        /* Message */
        client.gx_rectangle_left = widget->gx_widget_size.gx_rectangle_left + 10;
        client.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_top + 110;
        client.gx_rectangle_right = widget->gx_widget_size.gx_rectangle_right - 10;
        client.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_bottom - 12;
    }
    else {
        client.gx_rectangle_left = widget->gx_widget_size.gx_rectangle_left + 10;
        client.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_top + 80;
        client.gx_rectangle_right = widget->gx_widget_size.gx_rectangle_right - 10;
        client.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_bottom - 12;
        gx_widget_hide(&mw->name);
    }
    str.gx_string_ptr = data->text;
    str.gx_string_length = data->len;
    gx_multi_line_text_view_text_set_ext(&mw->text, &str);
    gx_widget_resize(&mw->text, &client);
    mw->data = data;
}

static void message_widget_construct(GX_WIDGET *parent, const struct message_data *data)
{
    struct message_widget *mw = message_widget_alloc(MW_ALLOC_APPEND_ACTIVE);
    GX_WIDGET *widget = &mw->widget;

    message_data_attach(mw, data);
    gx_widget_attach(parent, widget);
    message_list_request_refresh();
}

static inline void message_viewer_open(struct message_window *win, 
    struct message_widget *mw, GX_WIDGET *current)
{
    message_widget_flags_set(win, MSG_VIEWER_OPENED);
    message_data_attach(&win->viewer.widget, mw->data);
    win->viewer.mw = mw;
    message_widget_switch(current, (GX_WIDGET *)&win->viewer,
        &message_alpha_animation);
}

static inline void message_viewer_close(struct message_window *win, 
    struct message_viewer *viewer, GX_WIDGET *next)
{
    message_widget_flags_clr(win, MSG_VIEWER_OPENED);
    message_widget_switch((GX_WIDGET *)viewer, next,
        &message_alpha_animation);
}

static UINT message_viewer_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
    return _gx_widget_event_process(widget, event_ptr);
}

static void message_viewer_create(GX_WIDGET *parent, struct message_viewer *viewer)
{
    GX_SCROLLBAR_APPEARANCE scrollbar;

    message_widget_create(&viewer->widget, NULL, &message_viewer_size, 4);
    gx_widget_event_process_set(&viewer->widget, message_viewer_event_process);
    memset(&scrollbar, 0, sizeof(scrollbar));
    scrollbar.gx_scroll_width = 5;
    scrollbar.gx_scroll_thumb_width = 5;
    scrollbar.gx_scroll_thumb_border_style = 0;
    scrollbar.gx_scroll_thumb_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
    scrollbar.gx_scroll_thumb_border_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
    scrollbar.gx_scroll_button_color = GX_COLOR_ID_HONBOW_WHITE;
    gx_vertical_scrollbar_create(&viewer->vscrollbar, "messagetxt-bar", &viewer->widget.text, &scrollbar,
        GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB | GX_SCROLLBAR_VERTICAL);
}

static UINT message_state_vertical_slide(struct message_widget *mw,
    GX_WIDGET *widget, GX_EVENT *event_ptr)
{
    (void)mw;
    return _gx_widget_event_process(widget, event_ptr);
}

static int message_delete_button_shift_helper(struct message_widget *mw, GX_WIDGET *widget, 
    INT delta_x)
{
    if (widget == NULL)
        widget = &mw->widget;
    if (widget->gx_widget_size.gx_rectangle_left + delta_x < mw->win->x_left_limit) {
        delta_x = mw->win->x_left_limit - widget->gx_widget_size.gx_rectangle_left;
        mw->win->del_widget = mw;
        message_widget_flags_set(mw->win, MSG_DELBTN_DEL);

        gx_widget_front_move((GX_WIDGET *)&mw->win->del_button, NULL);
    } else if (widget->gx_widget_size.gx_rectangle_right + delta_x > mw->win->x_right_limit)  {
        delta_x = mw->win->x_right_limit - widget->gx_widget_size.gx_rectangle_right;
        message_widget_flags_clr(mw->win, MSG_DELBTN_BUSY | MSG_DELBTN_DEL);
    } else {
        message_widget_flags_clr(mw->win, MSG_DELBTN_DEL);
    }
    if (delta_x) {
        _gx_widget_shift(widget, delta_x, 0, GX_TRUE);
        _gx_widget_shift((GX_WIDGET *)&mw->win->del_button, delta_x, 0, GX_TRUE);
        mw->x_start += delta_x / 2;
        message_widget_flags_set(mw->win, MSG_DELBTN_BUSY);
    }
    return message_widget_flags_tst(mw->win, MSG_DELBTN_BUSY);
}

static UINT message_state_horizontal_slide(struct message_widget *mw,
    GX_WIDGET *widget, GX_EVENT *event_ptr)
{
    INT delta_x = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - mw->x_start;

    message_delete_button_shift_helper(mw, widget, delta_x);
    return GX_SUCCESS;
}

static UINT message_state_sliding_check(struct message_widget *mw,
    GX_WIDGET *widget, GX_EVENT *event_ptr)
{
    GX_RECTANGLE client;
    INT delta_x, delta_y;
    UINT ret;

    delta_x = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x - mw->x_start);
    delta_y = GX_ABS(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y - mw->y_start);
    if (delta_y >= delta_x && !message_widget_flags_tst(mw->win, MSG_DELBTN_BUSY)) {
        mw->state = message_state_vertical_slide;
    } else {
        GX_WIDGET *w = (GX_WIDGET *)&message_win.del_button;
        ux_input_capture_stack_release(widget);
        client.gx_rectangle_left = w->gx_widget_size.gx_rectangle_left;
        client.gx_rectangle_right = w->gx_widget_size.gx_rectangle_right;
        client.gx_rectangle_top = widget->gx_widget_size.gx_rectangle_top;
        client.gx_rectangle_bottom = widget->gx_widget_size.gx_rectangle_bottom;
        gx_widget_resize(&message_win.del_button, &client);
        mw->state = message_state_horizontal_slide;
        message_widget_flags_set(mw->win, MSG_DELBTN_BUSY);
        ret = GX_SUCCESS;
    }
    return mw->state(mw, widget, event_ptr);
}

static UINT message_widget_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
    struct message_widget *mw = CONTAINER_OF(widget, struct message_widget, widget);
    GX_EVENT event;
    UINT ret = GX_SUCCESS;

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_HIDE:
        /* Refresh the message list of parent */
        event.gx_event_target = widget->gx_widget_parent;
        event.gx_event_sender = widget->gx_widget_id;
        event.gx_event_type = GX_EVENT_SHOW;
        _gx_system_event_send(&event);
        ret = gx_widget_event_process(widget, event_ptr);
        break;
    case GX_EVENT_TIMER:
        if (event_ptr->gx_event_payload.gx_event_timer_id == MESSAGE_DELETE_BUTTON_TIMER_ID) {
            if (!message_delete_button_shift_helper(mw, &mw->win->del_widget->widget, 20))
                gx_system_timer_stop(widget, MESSAGE_DELETE_BUTTON_TIMER_ID);
        }
        break;
    case GX_EVENT_PEN_DOWN:
        if (widget->gx_widget_style & GX_STYLE_ENABLED) {
            mw->x_start = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_x;
            mw->y_start = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y;
            _gx_system_input_capture(widget);
            if (message_widget_flags_tst(mw->win, MSG_DELBTN_DEL)) {
                mw->state = NULL;
                ret = gx_system_timer_start(widget, MESSAGE_DELETE_BUTTON_TIMER_ID, 1, 1);
                break;
            }
            if (!message_widget_flags_tst(mw->win, MSG_DELBTN_BUSY))
                mw->state = message_state_sliding_check;
        }
        if (!message_widget_flags_tst(mw->win, MSG_DELBTN_BUSY))
            ret = gx_widget_event_to_parent(widget, event_ptr);
        break;
    case GX_EVENT_PEN_DRAG:
        if (mw->state)
            ret = mw->state(mw, widget, event_ptr);
        break;
    case GX_EVENT_PEN_UP:
        if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
            _gx_system_input_release(widget);
            if (mw->state == message_state_sliding_check)
                message_viewer_open(&message_win, mw, (GX_WIDGET *)&message_win.list);
        }
        if (!message_widget_flags_tst(mw->win, MSG_DELBTN_BUSY)) {
            ret = gx_widget_event_to_parent(widget, event_ptr);
        } else {
            if (!message_widget_flags_tst(mw->win, MSG_DELBTN_DEL))
                ret = gx_system_timer_start(widget, MESSAGE_DELETE_BUTTON_TIMER_ID, 1, 1);
        }
        break;
    case GX_EVENT_INPUT_RELEASE:
        if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
            _gx_system_input_release(widget);
        }
        break;
    default:
        ret = gx_widget_event_process(widget, event_ptr);
        break;
    }
    return ret;
}

static UINT message_vertical_list_children_position(GX_VERTICAL_LIST *vertical_list)
{
    GX_RECTANGLE childsize = vertical_list->gx_window_client;
    GX_WIDGET *child = vertical_list->gx_widget_first_child;
    INT index = vertical_list->gx_vertical_list_top_index;
    GX_VALUE height;
    GX_VALUE client_height;

    vertical_list->gx_vertical_list_child_height = 0;
    vertical_list->gx_vertical_list_child_count = 0;
    while (child) {
        if (!(child->gx_widget_status & GX_STATUS_NONCLIENT)) {
            vertical_list->gx_vertical_list_child_count++;
            if (!(child->gx_widget_id)) {
                child->gx_widget_id = (USHORT)(LIST_CHILD_ID_START +
                    vertical_list->gx_vertical_list_child_count);
            }
            if (index == vertical_list->gx_vertical_list_selected)
                child->gx_widget_style |= GX_STYLE_DRAW_SELECTED;
            else
                child->gx_widget_style &= ~GX_STYLE_DRAW_SELECTED;
            index++;
            child->gx_widget_status &= ~GX_STATUS_ACCEPTS_FOCUS;
            _gx_widget_height_get(child, &height);
            if (height > vertical_list->gx_vertical_list_child_height)
                vertical_list->gx_vertical_list_child_height = height;
            /* move this child into position */
            childsize.gx_rectangle_bottom = (GX_VALUE)(childsize.gx_rectangle_top + height - 1);
            _gx_widget_resize(child, &childsize);
            childsize.gx_rectangle_top = (GX_VALUE)(childsize.gx_rectangle_bottom + 10);
        }
        child = child->gx_widget_next;
    }

    /* calculate number of visible rows, needed for scrolling info */
    if (vertical_list->gx_vertical_list_child_height > 0) {
        _gx_window_client_height_get((GX_WINDOW *)vertical_list, &client_height);
        GX_VALUE average_height = (vertical_list->gx_vertical_list_child_height * (index - 1) 
            + MESSAGE_CLEAR_BUTTON_HEIGHT) / index;
        vertical_list->gx_vertical_list_visible_rows = 
            (GX_VALUE)((client_height + average_height - 1) / average_height);
    } else {
        vertical_list->gx_vertical_list_visible_rows = 1;
    }
    return GX_SUCCESS;
}

static INT message_vertical_list_back_distance(GX_VERTICAL_LIST *list)
{
    GX_WIDGET *first_child = _gx_widget_first_client_child_get((GX_WIDGET *)list);
    GX_WIDGET *last_child = _gx_widget_last_client_child_get((GX_WIDGET *)list);
    INT delta = 0;

    if (first_child) {
        if (first_child->gx_widget_size.gx_rectangle_top >
            list->gx_window_client.gx_rectangle_top) {
            delta = (GX_VALUE)(list->gx_window_client.gx_rectangle_top -
                first_child->gx_widget_size.gx_rectangle_top);
        }  else if (last_child->gx_widget_size.gx_rectangle_bottom < 
                    list->gx_window_client.gx_rectangle_bottom) {
            delta = (GX_VALUE)(list->gx_window_client.gx_rectangle_bottom -
                last_child->gx_widget_size.gx_rectangle_bottom);
        }
    }
    return delta;
}

static inline void message_list_height_get(GX_VERTICAL_LIST *list,
    INT *list_height, INT *widget_height)
{
    *list_height = (list->gx_vertical_list_child_count - 1) * 
        list->gx_vertical_list_child_height + MESSAGE_CLEAR_BUTTON_HEIGHT;
    *widget_height = list->gx_window_client.gx_rectangle_bottom - 
        list->gx_window_client.gx_rectangle_top + 1;
}

static UINT message_list_event_process(GX_VERTICAL_LIST *list, GX_EVENT *event_ptr)
{
    struct message_window *win = (struct message_window *)list;
    GX_WIDGET *widget = (GX_WIDGET *)list;
    GX_SCROLLBAR *pScroll;
    INT list_height;
    INT widget_height;
    INT new_pen_index;
    GX_WIDGET *child;
    UINT ret = GX_SUCCESS;

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_SHOW:
        ret = _gx_window_event_process((GX_WINDOW *)list, event_ptr);
        if (!list->gx_vertical_list_child_count)
            message_vertical_list_children_position(list);
        _gx_window_scrollbar_find((GX_WINDOW *)list, GX_TYPE_VERTICAL_SCROLL, &pScroll);
        if (pScroll)
            _gx_scrollbar_reset(pScroll, GX_NULL);
        break;
    case GX_EVENT_PEN_DOWN:
        _gx_widget_status_add(widget, GX_STATUS_OWNS_INPUT); //_gx_system_input_capture(widget);
        if (message_widget_flags_tst(win, MSG_CLRBTN_CLR)) {
            gx_widget_event_to_parent((GX_WIDGET *)list, event_ptr);
        }
        list->gx_window_move_start = event_ptr->gx_event_payload.gx_event_pointdata;
        child = _gx_system_top_widget_find((GX_WIDGET *)list, 
            event_ptr->gx_event_payload.gx_event_pointdata, GX_STATUS_SELECTABLE);
        while (child && child->gx_widget_parent != (GX_WIDGET *)list)
            child = child->gx_widget_parent;
        if (child) {
            list->gx_vertical_list_pen_index = list->gx_vertical_list_top_index + 
                _gx_widget_client_index_get(widget, child);
            if (list->gx_vertical_list_pen_index >= list->gx_vertical_list_total_rows)
                list->gx_vertical_list_pen_index -= list->gx_vertical_list_total_rows;
        }
        break;
    case GX_EVENT_PEN_UP:
        if (list->gx_widget_status & GX_STATUS_OWNS_INPUT) {
            _gx_widget_status_remove(widget, GX_STATUS_OWNS_INPUT); //ret = _gx_system_input_release(widget);
            message_list_height_get(list, &list_height, &widget_height);
            if (list_height > widget_height)
                _gx_vertical_list_slide_back_check(list);
            if (list->gx_vertical_list_pen_index >= 0 && 
                list->gx_vertical_list_snap_back_distance == 0) {
                /* test to see if pen-up is over same child widget as pen-down */
                child = _gx_system_top_widget_find((GX_WIDGET *)list,
                    event_ptr->gx_event_payload.gx_event_pointdata, GX_STATUS_SELECTABLE);
                while (child && child->gx_widget_parent != (GX_WIDGET *)list)
                    child = child->gx_widget_parent;
                if (child) {
                    new_pen_index = list->gx_vertical_list_top_index + 
                        _gx_widget_client_index_get(widget, child);
                    if (new_pen_index >= list->gx_vertical_list_total_rows)
                        new_pen_index -= list->gx_vertical_list_total_rows;
                    if (new_pen_index == list->gx_vertical_list_pen_index)
                        _gx_vertical_list_selected_set(list, list->gx_vertical_list_pen_index);
                }
            }
        } else {
           ret = _gx_widget_event_to_parent((GX_WIDGET *)list, event_ptr);
        }
        break;
    case GX_EVENT_PEN_DRAG:
        if ((widget->gx_widget_status & GX_STATUS_OWNS_INPUT) &&
            (event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y - 
                list->gx_window_move_start.gx_point_y)) {
            message_list_height_get(list, &list_height, &widget_height);
            INT delta = message_vertical_list_back_distance(list);
            if (GX_ABS(delta) > win->y_list_limit) {
                if (delta < 0 || list_height <= widget_height) {
                    event_ptr->gx_event_type = GX_EVENT_PEN_DOWN;
                    _gx_widget_event_to_parent((GX_WIDGET *)list, event_ptr);
                }
                break;
            }

            if (list_height > widget_height) {
                /* Start sliding, remove other widgets from input capture stack.  */
                ux_input_capture_stack_release(NULL);
                _gx_vertical_list_scroll(list,
                    event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y -
                    list->gx_window_move_start.gx_point_y);
                _gx_window_scrollbar_find((GX_WINDOW *)list, GX_TYPE_VERTICAL_SCROLL, &pScroll);
                if (pScroll)
                    _gx_scrollbar_reset(pScroll, GX_NULL);
                list->gx_window_move_start = event_ptr->gx_event_payload.gx_event_pointdata;
                list->gx_vertical_list_pen_index = -1;
            }
        } else {
            _gx_widget_event_to_parent((GX_WIDGET *)list, event_ptr);
        }
        break;
    case GX_EVENT_VERTICAL_FLICK:
        message_list_height_get(list, &list_height, &widget_height);
        if (list_height > widget_height) {
            list->gx_vertical_list_snap_back_distance = 
                (GX_VALUE)(GX_FIXED_VAL_TO_INT(event_ptr->gx_event_payload.gx_event_intdata[0]) * 8);
            _gx_system_timer_start((GX_WIDGET *)list, GX_FLICK_TIMER, 1, 1);
        }
        list->gx_vertical_list_pen_index = -1;
        break;
    default:
        ret = _gx_vertical_list_event_process(list, event_ptr);
        break;
    }
    return ret;
}

//static int message_clear_button_shift_helper(struct message_window *win, INT delta_y)
//{
//    GX_WIDGET *widget = (GX_WIDGET *)&win->clr_button.button;
//
//    if (widget->gx_widget_size.gx_rectangle_top + delta_y >= win->y_clrbtn_limit) {
//        delta_y = win->y_clrbtn_limit - widget->gx_widget_size.gx_rectangle_top;
//        message_widget_flags_set(win, MSG_CLRBTN_CLR);
//        //gx_widget_front_move((GX_WIDGET *)&win->clr_button, NULL);
//    } else if (widget->gx_widget_size.gx_rectangle_bottom + delta_y <= 0) {
//        delta_y = 0 - widget->gx_widget_size.gx_rectangle_bottom;
//        message_widget_flags_clr(win, MSG_CLRBTN_CLR | MSG_CLRBTN_BUSY);
//    } else {
//        message_widget_flags_clr(win, MSG_CLRBTN_CLR);
//    }
//    if (delta_y) {
//        message_widget_flags_set(win, MSG_CLRBTN_BUSY);
//        _gx_widget_shift((GX_WIDGET *)&win->list, 0, delta_y, GX_TRUE);
//        _gx_widget_shift((GX_WIDGET *)&win->clr_button, 0, delta_y, GX_TRUE);
//        win->y_clrbtn_origin += delta_y / 2;
//    }
//    return message_widget_flags_tst(win, MSG_CLRBTN_BUSY);
//}

static UINT message_top_window_event_process(GX_WINDOW *win, GX_EVENT *event_ptr)
{
    struct message_window *mwin = &message_win;
    struct message_viewer *viewer = &mwin->viewer;
    //INT delta_y;

    switch (event_ptr->gx_event_type) {
    //case GX_EVENT_TIMER:
    //    if (event_ptr->gx_event_payload.gx_event_timer_id == MESSAGE_CLEAR_BUTTON_TIMER_ID) {
    //        if (!message_clear_button_shift_helper(mwin, -20)) {
    //            gx_system_timer_stop((GX_WIDGET *)win, MESSAGE_CLEAR_BUTTON_TIMER_ID);
    //        }
    //    }
    //    break;
    case GX_EVENT_PEN_DOWN:
        _gx_system_input_capture((GX_WIDGET *)win);
        //if (message_widget_flags_tst(mwin, MSG_CLRBTN_CLR))
        //    ux_input_capture_stack_release((GX_WIDGET *)win);
        //mwin->y_clrbtn_origin = event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y;
        break;
    case GX_EVENT_PEN_UP:
        if (win->gx_widget_status & GX_STATUS_OWNS_INPUT) {
            _gx_system_input_release((GX_WIDGET *)win);
            if (viewer->mw && message_widget_flags_tst(mwin, MSG_VIEWER_OPENED)) {
                GX_WIDGET *w = &viewer->widget.widget;
                gx_widget_status_remove(win, GX_STATUS_OWNS_INPUT);
                if (!gx_utility_rectangle_point_detect(&w->gx_widget_size,
                    event_ptr->gx_event_payload.gx_event_pointdata))
                    message_viewer_close(&message_win, &mwin->viewer, (GX_WIDGET *)&message_win.list);
            } else {
                //if (message_widget_flags_tst(mwin, MSG_CLRBTN_BUSY | MSG_CLRBTN_CLR) ==
                //    MSG_CLRBTN_BUSY) {
                //    gx_system_timer_start((GX_WIDGET *)win, MESSAGE_CLEAR_BUTTON_TIMER_ID, 1, 1);
                //}
            }
        }
        break;
    //case GX_EVENT_PEN_DRAG:
    //    if (win->gx_widget_status & GX_STATUS_OWNS_INPUT) {
    //        ux_input_capture_stack_release((GX_WIDGET *)win);
    //        if (!viewer->mw || !message_widget_flags_tst(mwin, MSG_VIEWER_OPENED)) {
    //            delta_y = (GX_VALUE)(event_ptr->gx_event_payload.gx_event_pointdata.gx_point_y
    //                    - mwin->y_clrbtn_origin);
    //            if (delta_y) 
    //                message_clear_button_shift_helper(mwin, delta_y);
    //        }
    //    }
    //    break;
    default:
        //ret = gx_window_event_process(win, event_ptr);
        break;
    }
    return gx_window_event_process(win, event_ptr);
}

static void message_delete_button_color_set(GX_WIDGET *widget, struct pixelmap_button *pb)
{
    if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED)
        ux_round_rectangle_render(&widget->gx_widget_size, pb->selected_fill_color, 6);
    else
        ux_round_rectangle_render(&widget->gx_widget_size, pb->normal_fill_color, 6);
}

static UINT message_clear_button_event_process(GX_PIXELMAP_BUTTON *button, GX_EVENT *event_ptr)
{
    struct message_window *win = CONTAINER_OF(button, struct message_window, clr_button.button);

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_PEN_UP:
        if (button->gx_widget_status & GX_STATUS_OWNS_INPUT) {
            gx_pixelmap_button_event_process(button, event_ptr);
            ux_screen_move((GX_WIDGET *)&win->list, &win->dialog.widget, NULL);
        }
        return GX_SUCCESS;
    case GX_EVENT_PEN_DRAG:
        if (button->gx_widget_status & GX_STATUS_OWNS_INPUT) {
            ux_input_capture_stack_release((GX_WIDGET *)button);
            if (button->gx_widget_style & GX_STYLE_BUTTON_PUSHED) {
                if (!(button->gx_widget_style & GX_STYLE_BUTTON_RADIO))
                    button->gx_button_deselect_handler((GX_WIDGET *)button, GX_FALSE);
            }
        }
        break;
    default:
        break;
    }
    return gx_pixelmap_button_event_process(button, event_ptr);
}

static UINT message_delete_button_event_process(GX_PIXELMAP_BUTTON *button, GX_EVENT *event_ptr)
{
    struct message_window *mwin = CONTAINER_OF(button, struct message_window, del_button.button);

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_TIMER:
        if (event_ptr->gx_event_payload.gx_event_timer_id == MESSAGE_DELETE_BUTTON_TIMER_ID) {
            if (!message_delete_button_shift_helper(mwin->del_widget, &mwin->del_widget->widget, 20)) {
                gx_system_timer_stop(button, MESSAGE_DELETE_BUTTON_TIMER_ID);
                message_delete(mwin->del_widget);
                if (!message_active_empty())
                    gx_widget_front_move(&mwin->list, NULL);
                else
                    ux_screen_move((GX_WIDGET *)&mwin->list, &mwin->empty.widget, NULL);
            }
        }
        break;
    case GX_EVENT_PEN_UP:
        gx_system_timer_start(button, MESSAGE_DELETE_BUTTON_TIMER_ID, 1, 1);
        break;
    default:
        break;
    }
    return gx_pixelmap_button_event_process(button, event_ptr);
}

static UINT message_dialog_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
    struct message_window *win = CONTAINER_OF(widget, struct message_window, dialog.widget);

    switch (event_ptr->gx_event_type) {
    case GX_SIGNAL(MESSAGE_CANCEL_BUTTON_ID, GX_EVENT_CLICKED):
        ux_screen_move(widget, (GX_WIDGET *)&win->list, NULL);
        break;
    case GX_SIGNAL(MESSAGE_ENTER_BUTTON_ID, GX_EVENT_CLICKED):
        message_all_clear();
        ux_screen_move(widget, (GX_WIDGET *)&win->empty.widget, NULL);
        break;
    }
    return gx_widget_event_process(widget, event_ptr);
}

static void message_delete_dialog_create(GX_WIDGET *parent, struct delete_dialog *dialog)
{
    GX_RECTANGLE size;

    gx_utility_rectangle_define(&size, 0, 0, 320, 360);
    gx_widget_create(&dialog->widget, "del-dialog", parent, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, 0, &size);
    gx_widget_event_process_set(&dialog->widget, message_dialog_event_process);
    gx_widget_fill_color_set(&dialog->widget, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

    /* Dialog information */
    gx_utility_rectangle_define(&size, 100, 140, 100 + 158, 140 + 30);
    gx_prompt_create(&dialog->info, "information", &dialog->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &size);
    gx_prompt_text_color_set(&dialog->info, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&dialog->info, GX_FONT_ID_SIZE_26);
    ux_widget_string_set(message, 0, &dialog->info, gx_prompt_text_set_ext);

    /* Cancel button */
    gx_utility_rectangle_define(&size, 12, 283, 12 + 143, 283 + 65);
    ux_pixelmap_button_create(&dialog->cancel, "cancel-button", (GX_WIDGET *)&dialog->widget,
        GX_PIXELMAP_ID_BTN_S_CANCEL,
        GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_GREEN, MESSAGE_CANCEL_BUTTON_ID,
        GX_STYLE_ENABLED, &size, message_delete_button_color_set);

    /* Enter button */
    gx_utility_rectangle_define(&size, 165, 283, 165 + 143, 283 + 65);
    ux_pixelmap_button_create(&dialog->enter, "enter-button", (GX_WIDGET *)&dialog->widget,
        GX_PIXELMAP_ID_BTN_S_CONFIRM,
        GX_COLOR_ID_HONBOW_GREEN, GX_COLOR_ID_HONBOW_DARK_GRAY, MESSAGE_ENTER_BUTTON_ID,
        GX_STYLE_ENABLED, &size, message_delete_button_color_set);
}

static void message_win_create(GX_WIDGET *parent, struct message_window *win)
{
    //GX_SCROLLBAR_APPEARANCE scrollbar;
    GX_RECTANGLE rect;
    GX_VALUE width;

    win->parent = parent;
    rect = parent->gx_widget_size;
    gx_utility_rectangle_resize(&rect, -12);
    //rect.gx_rectangle_right += 8;
    _gx_vertical_list_create(&win->list, "message-list", parent, 0,
        NULL, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT,
        GX_ID_NONE, &rect);
    gx_widget_event_process_set(&win->list, message_list_event_process);
    //memset(&scrollbar, 0, sizeof(scrollbar));
    //scrollbar.gx_scroll_width = 5;
    //scrollbar.gx_scroll_thumb_width = 5;
    //scrollbar.gx_scroll_thumb_border_style = 0;
    //scrollbar.gx_scroll_thumb_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
    //scrollbar.gx_scroll_thumb_border_color = GX_COLOR_ID_HONBOW_DARK_GRAY;
    //scrollbar.gx_scroll_button_color = GX_COLOR_ID_HONBOW_WHITE;
    //gx_vertical_scrollbar_create(&win->vscrollbar, "messagelist-bar", &win->list, &scrollbar,
    //    GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_SCROLLBAR_RELATIVE_THUMB | GX_SCROLLBAR_VERTICAL);

    gx_utility_rectangle_define(&rect, 12, 0, 12 + 296, 0+MESSAGE_CLEAR_BUTTON_HEIGHT);
    ux_pixelmap_button_create(&win->clr_button, "clr-button", (GX_WIDGET *)&win->list,
        GX_PIXELMAP_ID_BTN_ALL_DELETE,
        GX_COLOR_ID_HONBOW_ORANGE, GX_COLOR_ID_HONBOW_DARK_GRAY, MESSAGE_CLEAR_BUTTON_ID,
        GX_STYLE_ENABLED, &rect, message_delete_button_color_set);
    gx_widget_event_process_set(&win->clr_button, message_clear_button_event_process);

    /* Delete button */
    gx_utility_rectangle_define(&rect, 320, 12, 320 + 90, 12 + 192);
    ux_pixelmap_button_create(&win->del_button, "del-button", parent,
        GX_PIXELMAP_ID_BTN_R_DELETE,
        GX_COLOR_ID_HONBOW_ORANGE, GX_COLOR_ID_HONBOW_DARK_GRAY, MESSAGE_DELETE_BUTTON_ID,
        GX_STYLE_ENABLED, &rect, message_delete_button_color_set);
    gx_widget_event_process_set(&win->del_button, message_delete_button_event_process);

    gx_utility_rectangle_define(&rect, 12, 12, 12 + 296, 12 + 192);
    for (int i = 0; i < MAX_MESSAGES; i++) {
        message_widget_create(&message_pool[i], NULL, &rect, 12);
        message_pool[i].win = win;
        list_add_tail(&message_pool[i].node, &free_list);
    }

    gx_widget_width_get(&win->del_button, &width);
    win->x_left_limit = rect.gx_rectangle_left - width;
    win->x_right_limit = rect.gx_rectangle_right;
    win->y_list_limit = 0;
    win->y_clrbtn_limit = 1;
}

static void message_icon_win_draw(GX_WIDGET *widget)
{
    struct message_icon *e = CONTAINER_OF(widget, struct message_icon, client);
    ux_rectangle_pixelmap_draw(&widget->gx_widget_size, e->pixmap, 0);
}

static void message_icon_win_create(GX_WIDGET *parent, struct message_icon *e, 
    GX_RESOURCE_ID icon, GX_RESOURCE_ID text)
{
    GX_RECTANGLE size;

    gx_utility_rectangle_define(&size, 0, 0, 320, 345);
    gx_widget_create(&e->widget, "empty", parent, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, 0, &size);
    gx_widget_fill_color_set(&e->widget, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

    /* Prompt information */
    gx_utility_rectangle_define(&size, 12, 200, 12 + 296, 200 + 35);
    gx_prompt_create(&e->tips, "information", &e->widget, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &size);
    gx_prompt_text_color_set(&e->tips, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&e->tips, GX_FONT_ID_SIZE_26);
    ux_widget_string_set(message, text, &e->tips, gx_prompt_text_set_ext);

    /* Pixelmap rectangle */
    gx_utility_rectangle_define(&size, 120, 120, 120 + 80, 120 + 80);
    gx_widget_create(&e->client, "icon-show", &e->widget, 
        GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, 0, &size);
    gx_widget_fill_color_set(&e->client, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_draw_set(&e->client, message_icon_win_draw);
    e->pixmap = icon;
}

static void message_icon_win_attach(struct message_icon *e, struct message_data *data)
{
    if (message_icon_get(data) == GX_PIXELMAP_ID_IC_WECHAT) {
        GX_STRING str;
        str.gx_string_ptr = "WeChat";
        str.gx_string_length = strlen(str.gx_string_ptr);
        gx_prompt_text_set_ext(&e->tips, &str);
    }
}

static UINT message_icon_notifier_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
    struct message_window *win = CONTAINER_OF(widget, struct message_window, news.widget);

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_PEN_DOWN:
        _gx_system_input_capture(widget);
        break;
    case GX_EVENT_PEN_UP:
        if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
            _gx_system_input_release(widget);
            message_widget_switch(widget, (GX_WIDGET *)&win->notifier, 
                &message_alpha_animation);
        }
        break;
    }
    return _gx_widget_event_process(widget, event_ptr);
}

static UINT message_notifier_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
    switch (event_ptr->gx_event_type) {
    case GX_SIGNAL(MESSAGE_COMPLETE_BUTTON_ID, GX_EVENT_CLICKED):
        printf("Complete button\n");
        break;
    case GX_SIGNAL(MESSAGE_IGNORE_BUTTON_ID, GX_EVENT_CLICKED):
        printf("Ignore button\n");
        break;
    default:
        break;
    }
    return _gx_widget_event_process(widget, event_ptr);
}

static UINT message_notifier_widget_event_process(GX_WIDGET *widget, GX_EVENT *event_ptr)
{
    struct message_window *win = CONTAINER_OF(widget, struct message_window, notifier.widget.widget);
    struct message_notifier *notifier = &win->notifier;

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_SHOW:
        notifier->widget.win = win;
        break;
    case GX_EVENT_PEN_DOWN:
        _gx_system_input_capture(widget);
        break;
    case GX_EVENT_PEN_UP:
        if (widget->gx_widget_status & GX_STATUS_OWNS_INPUT) {
            _gx_system_input_release(widget);
            if (!message_widget_flags_tst(win, MSG_VIEWER_OPENED)) {
                gx_widget_hide((GX_WIDGET *)&notifier->cmp_button);
                gx_widget_hide((GX_WIDGET *)&notifier->ign_button);
                message_viewer_open(win, &notifier->widget, widget);
            }
        }
        break;
    default:
        break;
    }
    return _gx_widget_event_process(widget, event_ptr);
}

static UINT message_notifer_win_create(GX_WIDGET *parent, struct message_notifier *wnf)
{
    GX_RECTANGLE client;

    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_widget_create(&wnf->main, "message-notifer", parent, 
        GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_event_process_set(&wnf->main, message_notifier_event_process);
    gx_widget_fill_color_set(&wnf->main, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);

    gx_utility_rectangle_define(&client, 12, 12, 12 + 296, 12 + 192);
    message_widget_create(&wnf->widget, &wnf->main, &client, 12);
    gx_widget_event_process_set(&wnf->widget.widget, message_notifier_widget_event_process);

    /* Completed button */
    client.gx_rectangle_top = client.gx_rectangle_bottom + 12;
    client.gx_rectangle_bottom = client.gx_rectangle_top + 65;
    ux_text_button_create(&wnf->cmp_button, "cmp-button", &wnf->main,
        0,
        GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_GREEN, MESSAGE_COMPLETE_BUTTON_ID,
        GX_STYLE_ENABLED | GX_STYLE_BUTTON_EVENT_ON_PUSH, &client, NULL);
    gx_text_button_font_set(&wnf->cmp_button.button, GX_FONT_ID_SIZE_26);
    gx_text_button_text_color_set(&wnf->cmp_button.button,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    ux_widget_string_set(message, 1, &wnf->cmp_button, gx_text_button_text_set_ext);

    /* Ignore button */
    client.gx_rectangle_top = client.gx_rectangle_bottom + 12;
    client.gx_rectangle_bottom = client.gx_rectangle_top + 65;
    ux_text_button_create(&wnf->ign_button, "ign-button", &wnf->main,
        0,
        GX_COLOR_ID_HONBOW_ORANGE, GX_COLOR_ID_HONBOW_DARK_GRAY, MESSAGE_IGNORE_BUTTON_ID,
        GX_STYLE_ENABLED | GX_STYLE_BUTTON_EVENT_ON_PUSH, &client, NULL);
    gx_text_button_font_set(&wnf->ign_button.button, GX_FONT_ID_SIZE_26);
    gx_text_button_text_color_set(&wnf->ign_button.button,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    ux_widget_string_set(message, 2, &wnf->ign_button, gx_text_button_text_set_ext);
    return GX_SUCCESS;
}

static void message_window_draw(GX_WINDOW *win)
{
    GX_RECTANGLE *client = &win->gx_widget_size;
    GX_DRAW_CONTEXT *context;
    GX_COLOR color;
    GX_VALUE height;

    height = 6;
    _gx_window_background_draw(win);
    gx_system_draw_context_get(&context);
    gx_context_color_get(GX_COLOR_ID_HONBOW_WHITE, &color);
    gx_brush_define(&context->gx_draw_context_brush, color, 0, GX_BRUSH_ROUND| GX_BRUSH_ALIAS);
    gx_context_brush_width_set(height);
    height = height >> 1;
    gx_canvas_line_draw(client->gx_rectangle_left + 130 + height,
        client->gx_rectangle_top + 350 + height,
        client->gx_rectangle_right - 130 - height,
        client->gx_rectangle_top + 350 + height);
    _gx_widget_children_draw((GX_WIDGET *)win);
}

GX_WIDGET *message_pop_widget_get(void)
{
    return &message_win.news.widget;
}

UINT message_window_create(const char *name, GX_WIDGET *parent, GX_WIDGET **ptr)
{
    struct message_window *win = &message_win;
    //GX_SCROLLBAR_APPEARANCE scrollbar;
    GX_WIDGET *topwin;
    UINT ret;

    ret = gx_studio_named_widget_create((char *)name, parent, &topwin);
    if (ret != GX_SUCCESS)
        return ret;

    gx_widget_draw_set(topwin, message_window_draw);
    gx_widget_event_process_set(topwin, message_top_window_event_process);
    message_win_create(topwin, win);

    /* Message text viewer */
    message_viewer_create(NULL, &win->viewer);

    /* Message clear dialog */
    message_delete_dialog_create(NULL, &win->dialog);

    /* Empty message window */
    message_icon_win_create(NULL, &win->empty, GX_PIXELMAP_ID_IC_MESSAGES_NO, 
        3);

    /* New message icon window */
    message_icon_win_create(NULL, &win->news, GX_PIXELMAP_ID_IC_WECHAT,
        3);
    message_icon_win_attach(&win->news, &_test_data_array[0]); //TODO: remove (only for test)
    gx_widget_event_process_set(&win->news, message_icon_notifier_event_process);

    /* New message notifier window */
    message_notifer_win_create(NULL, &win->notifier);
    message_data_attach(&win->notifier.widget, &_test_data_array[1]); //TODO: remove (only for test)

    //TODO: remove (only for test)
    for (int i = 0; i < ARRAY_SIZE(_test_data_array); i++)
        message_widget_construct((GX_WIDGET *)&win->list, &_test_data_array[i]);
    //

    return ret;
}