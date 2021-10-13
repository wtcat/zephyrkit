#include "ux/ux_api.h"
#include "gx_widget.h"
#include "gx_window.h"
#include "gx_system.h"
#include "gx_scrollbar.h"

#include "popup/widget.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"

enum sport_button_id {
    SPORT_BTN_0,
    SPORT_BTN_1,
    SPORT_BTN_2,
    SPORT_BTN_MAX
};

struct sport_main {
    GX_WIDGET widget;
    GX_PROMPT tips;
    GX_RESOURCE_ID pixelmap;
};

struct sport_widget {
    GX_WIDGET widget;
    GX_VERTICAL_LIST list;
    struct sport_main main;
    UX_TEXT_BUTTON buttons[SPORT_BTN_MAX];
};

#define SPORT_BTN_CLICKED(m) GX_SIGNAL(m+1, GX_EVENT_CLICKED)

static struct sport_widget popup_widget;
USTR_START(sport)
    _USTR(LANGUAGE_ENGLISH, 
        "Sporting Now",
        "Outdoor Steps",
        "Change Sporting",
        "Don't Remind"
    )
USTR_END(sport)

static void sport_main_draw(struct sport_main *main)
{
    GX_RECTANGLE *prect = &main->widget.gx_widget_size;
    GX_RECTANGLE client;
    INT r;

    client.gx_rectangle_left = prect->gx_rectangle_left + 87;
    client.gx_rectangle_top = prect->gx_rectangle_top + 40;
    client.gx_rectangle_right = client.gx_rectangle_left + 100;
    client.gx_rectangle_bottom = client.gx_rectangle_top + 100;
    r = (client.gx_rectangle_right - client.gx_rectangle_left + 1) >> 1;
    _gx_widget_background_draw(&main->widget);
    //ux_circle_render(client.gx_rectangle_left + r, client.gx_rectangle_top + r,
    //    r, GX_COLOR_ID_HONBOW_WHITE);
    ux_rectangle_pixelmap_draw(&client, main->pixelmap, 
        GX_STYLE_VALIGN_CENTER| GX_STYLE_HALIGN_CENTER);
    _gx_widget_children_draw(&main->widget);
}

static void sport_main_create(GX_WIDGET *parent, struct sport_main *main)
{
    GX_RECTANGLE client;

    /* Setup widget base class */
    gx_utility_rectangle_define(&client, 13, 0, 320-12, 228-10);
    gx_widget_create(&main->widget, "sport-main", parent,
        GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&main->widget, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_draw_set(&main->widget, sport_main_draw);

    gx_utility_rectangle_define(&client, 40, 161, 320-40, 161+45);
    gx_prompt_create(&main->tips, "sport-tips", &main->widget, 0,
        GX_STYLE_BORDER_NONE | GX_STYLE_TRANSPARENT | GX_STYLE_ENABLED,
        0, &client);
    gx_prompt_text_color_set(&main->tips, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&main->tips, GX_FONT_ID_SIZE_26);
    ux_widget_string_set(sport, 0, &main->tips, gx_prompt_text_set_ext);
    main->pixelmap = GX_PIXELMAP_ID_APP_LIST_ICON_SPORT_90_90; // GX_PIXELMAP_ID_APP_SPORT_IC_RUN;
}

static UINT sport_event_proccess(GX_WIDGET *wiget, GX_EVENT *event_ptr)
{
    switch (event_ptr->gx_event_type) {
    case SPORT_BTN_CLICKED(SPORT_BTN_0):
        break;
    case SPORT_BTN_CLICKED(SPORT_BTN_1):
        break;
    case SPORT_BTN_CLICKED(SPORT_BTN_2):
        return ux_screen_pop_and_switch(wiget, popup_hide_animation);
        break;
    default:
        break;
    }
    return _gx_widget_event_process(wiget, event_ptr);
}

static UINT sport_vertical_list_children_position(GX_VERTICAL_LIST *vertical_list)
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
        vertical_list->gx_vertical_list_visible_rows = 
            (GX_VALUE)((client_height + vertical_list->gx_vertical_list_child_height - 1) /
            vertical_list->gx_vertical_list_child_height);
    } else {
        vertical_list->gx_vertical_list_visible_rows = 1;
    }
    return GX_SUCCESS;
}

static UINT sport_list_event_process(GX_VERTICAL_LIST *list, GX_EVENT *event_ptr)
{
    GX_SCROLLBAR *pScroll;
    UINT ret = GX_SUCCESS;

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_SHOW:
        ret = _gx_window_event_process((GX_WINDOW *)list, event_ptr);
        if (!list->gx_vertical_list_child_count)
            sport_vertical_list_children_position(list);
        _gx_window_scrollbar_find((GX_WINDOW *)list, GX_TYPE_VERTICAL_SCROLL, &pScroll);
        if (pScroll)
            _gx_scrollbar_reset(pScroll, GX_NULL);
        break;
    default:
        ret = _gx_vertical_list_event_process(list, event_ptr);
        break;
    }
    return ret;
}

GX_WIDGET *sport_widget_get(void)
{
    return &popup_widget.widget;
}

UINT sport_widget_create(GX_WIDGET *parent, GX_WIDGET **ptr)
{
    struct sport_widget *w = &popup_widget;
    GX_RECTANGLE client;

    /* Setup widget base class */
    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_widget_create(&w->widget, "sport", parent,
        GX_STYLE_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_fill_color_set(&w->widget, GX_COLOR_ID_HONBOW_BLACK,
        GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK);
    gx_widget_event_process_set(&w->widget, sport_event_proccess);

    gx_utility_rectangle_define(&client, 13, 0, 320-12, 360);
    _gx_vertical_list_create(&w->list, "sport-list", &w->widget, 0,
        NULL, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT,
        GX_ID_NONE, &client);
    gx_widget_event_process_set(&w->list, sport_list_event_process);

    sport_main_create((GX_WIDGET *)&w->list, &w->main);
    for (int i = 0; i < SPORT_BTN_MAX; i++) {
        UX_TEXT_BUTTON *button = w->buttons + i;

        gx_utility_rectangle_define(&client, 13, 228, 320 - 12, 228+65);
        ux_text_button_create(button, "sport-button", (GX_WIDGET *)&w->list, 0,
            GX_COLOR_ID_HONBOW_DARK_GRAY, GX_COLOR_ID_HONBOW_GREEN, i + 1,
            GX_STYLE_ENABLED, &client, NULL);
        gx_text_button_font_set(&button->button, GX_FONT_ID_SIZE_26);
        gx_text_button_text_color_set(&button->button,
            GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
        ux_widget_string_set(sport, i+1, &button->button, gx_text_button_text_set_ext);
    }
    if (ptr)
        *ptr = &w->widget;
    return GX_SUCCESS;
}

