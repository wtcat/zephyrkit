#include <string.h>

#include "ux/ux_api.h"
#include "gx_system.h"
#include "gx_widget.h"

#include "guix_simple_resources.h"
#include "guix_simple_specifications.h"
#include "guix_language_resources_custom.h"

#include "qrcode/qrcodegen.h"


struct pairing_window {
    GX_VERTICAL_LIST list;
    GX_WINDOW guide;
    GX_WINDOW pairing;
    GX_WINDOW win;
    GX_MULTI_LINE_TEXT_VIEW prompt_install;
    GX_MULTI_LINE_TEXT_VIEW pairing_info;
    GX_PROMPT swipe_up;
    GX_WIDGET swipe_icon;
    GX_PROMPT pariring_1;
    GX_WIDGET qcode;
    GX_PROMPT pariring_2;
    UX_ANIMATION animation;
    GX_WIDGET *anilist[2];
    bool switched;
};

#define PAIRING_GUIDE_WIN_ID 0x01
#define PAIRING_INFO_WIN_ID  0x02

#define PAIRING_ANIMATION_ID 0x2001

#define QR_BUFSZ qrcodegen_BUFFER_LEN_FOR_VERSION(30)

static uint8_t qcode_buffer[QR_BUFSZ];
static struct pairing_window mod_window;
USTR_START(pairing)
    _USTR(LANGUAGE_ENGLISH, 
        "Please install the\n\"Letsfit\" app on your\nphone",
        "Swipe up",
        "Pairing 1",
        "Pairing 2",
        "Search for this watch\nin Letsfit\nApp: TW1 8888"
    ),
    _USTR(LANGUAGE_CHINESE,
        "请安装Letsfit app",
        "上滑",
        "配对1",
        "配对2",
        "在Letsfit App中\n搜索这块手表\nApp: TW1 8888"
    )
USTR_END(pairing)

static void arrow_window_draw(GX_WIDGET *widget)
{
    //struct pairing_window *mwin = &mod_window;
    //GX_PIXELMAP *pixelmap;

    //gx_context_pixelmap_get(sb->pixelmap_id, &pixelmap);
    //if (pixelmap != NULL) {
    //    gx_canvas_pixelmap_draw(widget->gx_widget_size.gx_rectangle_left,
    //        widget->gx_widget_size.gx_rectangle_top, pixelmap);
    //}
}

static void qrcode_widget_show(GX_WIDGET *widget)
{
    int qr_size = qrcodegen_getSize(qcode_buffer);
    GX_RECTANGLE fill_rect, rect = widget->gx_widget_size;
    const int pixel_dot_size = 8;
    int x_border, y_border;
    int x_center, y_center;
    int x_origin;

    //rect.gx_rectangle_top += 50;
    fill_rect = rect;
    gx_context_brush_width_set(0);
    gx_context_brush_define(GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE,
        GX_BRUSH_SOLID_FILL | GX_BRUSH_ROUND);
    gx_canvas_rectangle_draw(&rect);
    gx_context_brush_define(GX_COLOR_ID_HONBOW_BLACK, GX_COLOR_ID_HONBOW_BLACK,
        GX_BRUSH_SOLID_FILL | GX_BRUSH_ROUND);
    x_center = (rect.gx_rectangle_right - rect.gx_rectangle_left + 1) / 2;
    y_center = (rect.gx_rectangle_bottom - rect.gx_rectangle_top + 1) / 2;
    x_border = x_center / pixel_dot_size - (qr_size / 2);
    y_border = y_center / pixel_dot_size - (qr_size / 2);
    x_origin = rect.gx_rectangle_left;
    fill_rect.gx_rectangle_left = x_origin;

    for (int y = -y_border; y < qr_size; y++) {
        for (int x = -x_border; x < qr_size; x++) {
            if (qrcodegen_getModule(qcode_buffer, x, y)) {
                fill_rect.gx_rectangle_right = fill_rect.gx_rectangle_left + 
                    (pixel_dot_size - 1);
                fill_rect.gx_rectangle_bottom = fill_rect.gx_rectangle_top + 
                    (pixel_dot_size - 1);
                gx_canvas_rectangle_draw(&fill_rect);
            }
            fill_rect.gx_rectangle_left += pixel_dot_size;
        }

        fill_rect.gx_rectangle_left = x_origin;
        fill_rect.gx_rectangle_top += pixel_dot_size;
    }

    gx_widget_children_draw(widget);
}

static bool qcode_encode_info(GX_WIDGET *widget, GX_WIDGET *parent, 
    const char *info, GX_RECTANGLE *rect)
{
    uint8_t tmpbuf[QR_BUFSZ];
    bool ok;

    gx_widget_create(widget, "QR-CODE", parent,
        GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, 0, rect);
    memset(tmpbuf, 0, sizeof(tmpbuf));
    ok = qrcodegen_encodeText(info, tmpbuf, qcode_buffer, qrcodegen_Ecc_LOW,
        qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
    if (ok)
        gx_widget_draw_set(widget, qrcode_widget_show);
    return ok;
}

static void pairing_list_callback(GX_VERTICAL_LIST *list, GX_WIDGET *widget, INT index)
{
    struct pairing_window *mwin = (struct pairing_window *)list;
    GX_RECTANGLE client;
    GX_BOOL result;

    gx_widget_created_test(&mwin->win, &result);
    if (!result) {
        gx_utility_rectangle_define(&client, 0, 0, 320, 500);
        gx_window_create(&mwin->win, "pairing-win", &mwin->list,
            GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, 0, &client);

        /* Pairing-1 */
        gx_utility_rectangle_define(&client, 15, 25, 15 + 120, 25 + 30);
        gx_prompt_create(&mwin->pariring_1, "pairing-1", &mwin->win, 0,
            GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT, 0, &client);
        gx_prompt_text_color_set(&mwin->pariring_1, GX_COLOR_ID_HONBOW_WHITE,
            GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
        gx_prompt_font_set(&mwin->pariring_1, GX_FONT_ID_SIZE_26);
        ux_widget_string_set(pairing, 2, &mwin->pariring_1, 
            gx_prompt_text_set_ext);

        /* QR-Code */
        gx_utility_rectangle_define(&client, 54, 71, 54 + 212, 71 + 212);
        qcode_encode_info(&mwin->qcode, (GX_WIDGET *)&mwin->win, 
            "https://www.hao123.com", &client);

        /* Pairing-2 */
        gx_utility_rectangle_define(&client, 10, 314, 10 + 128, 314 + 30);
        gx_prompt_create(&mwin->pariring_2, "pairing-2", &mwin->win, 0,
            GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT, 0, &client);
        gx_prompt_text_color_set(&mwin->pariring_2, GX_COLOR_ID_HONBOW_WHITE,
            GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
        gx_prompt_font_set(&mwin->pariring_2, GX_FONT_ID_SIZE_26);
        ux_widget_string_set(pairing, 3, &mwin->pariring_2, 
            gx_prompt_text_set_ext);

        /* Pairing-info */
        gx_utility_rectangle_define(&client, 12, 353, 12 + 296, 353 + 121);
        gx_multi_line_text_view_create(&mwin->pairing_info, "pairing-info", 
            &mwin->win, 0, 
            GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_LEFT, 0, &client);
        gx_multi_line_text_view_text_color_set(&mwin->pairing_info, 
            GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE, 
            GX_COLOR_ID_HONBOW_WHITE);
        gx_multi_line_text_view_font_set(&mwin->pairing_info, GX_FONT_ID_SIZE_26);
        ux_widget_string_set(pairing, 4, &mwin->pairing_info, 
            gx_multi_line_text_view_text_set_ext);
    }
}

static UINT pairing_list_event_process(GX_VERTICAL_LIST *list, GX_EVENT *event_ptr)
{
    return gx_vertical_list_event_process(list, event_ptr);
}

static void pairing_infomation_window_setup(struct pairing_window *mw, GX_WIDGET *parent)
{
    GX_RECTANGLE client;

    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_window_create(&mw->pairing, "pairing", NULL,
        GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, 
        PAIRING_INFO_WIN_ID, &client);
    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_vertical_list_create(&mw->list, "pairing-list", &mw->pairing, 1,
        pairing_list_callback, GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT,
        GX_ID_NONE, &client);
    pairing_list_callback(&mw->list, (GX_WIDGET *)&mw->win, 0);
    gx_widget_event_process_set(&mw->list, pairing_list_event_process);
}

static void pairing_guide_window_setup(struct pairing_window *mw, GX_WIDGET *parent)
{
    GX_RECTANGLE client;

    gx_utility_rectangle_define(&client, 0, 0, 320, 360);
    gx_window_create(&mw->guide, "pairing-guide", parent,
        GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, 
        PAIRING_GUIDE_WIN_ID, &client);

    /* Install prompt */
    gx_utility_rectangle_define(&client, 24, 113, 24 + 273, 113 + 122);
    gx_multi_line_text_view_create(&mw->prompt_install, "prompt-install", &mw->guide,
        0, GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_multi_line_text_view_text_color_set(&mw->prompt_install, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_multi_line_text_view_font_set(&mw->prompt_install, GX_FONT_ID_SIZE_26);
    ux_widget_string_set(pairing, 0, &mw->prompt_install, 
        gx_multi_line_text_view_text_set_ext);

    /* Swipe up */
    gx_utility_rectangle_define(&client, 95, 260, 95 + 132, 260 + 42);
    gx_prompt_create(&mw->swipe_up, "swipe-up", &mw->guide, 0,
        GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER, 0, &client);
    gx_prompt_text_color_set(&mw->swipe_up, GX_COLOR_ID_HONBOW_WHITE,
        GX_COLOR_ID_HONBOW_WHITE, GX_COLOR_ID_HONBOW_WHITE);
    gx_prompt_font_set(&mw->swipe_up, GX_FONT_ID_SIZE_26);
    ux_widget_string_set(pairing, 1, &mw->swipe_up, 
        gx_prompt_text_set_ext);

    /* Arrow icon */
    gx_utility_rectangle_define(&client, 148, 309, 148 + 25, 309 + 14);
    gx_widget_create(&mw->swipe_icon, "arrow", &mw->guide,
        GX_STYLE_BORDER_NONE | GX_STYLE_ENABLED, 0, &client);
    gx_widget_draw_set(&mw->swipe_icon, arrow_window_draw);
}

static void pairing_window_start(GX_WIDGET *topwin, struct pairing_window *pw)
{
    GX_ANIMATION_INFO info;

    memset(&info, 0, sizeof(info));
    pw->anilist[0] = (GX_WIDGET *)&pw->guide;
    pw->anilist[1] = (GX_WIDGET *)&pw->pairing;
    info.gx_animation_parent = topwin;
    info.gx_animation_style = GX_ANIMATION_SCREEN_DRAG |
                              GX_ANIMATION_VERTICAL |
                              GX_ANIMATION_BACK_EASE_IN_OUT;
    info.gx_animation_id = PAIRING_ANIMATION_ID;
    info.gx_animation_frame_interval = 1;
    info.gx_animation_steps = 20;
    info.gx_animation_slide_screen_list = &pw->anilist[0];
    gx_animation_landing_speed_set((GX_ANIMATION *)&pw->animation, 15);
    ux_animation_drag_enable(&pw->animation, topwin, &info);
}

static UINT pairing_event_process(GX_WINDOW *window, GX_EVENT *event_ptr)
{
    struct pairing_window *mw = &mod_window;
    GX_WIDGET *w;

    switch (event_ptr->gx_event_type) {
    case GX_EVENT_SHOW:
        pairing_window_start((GX_WIDGET *)window, mw);
        break;
    case GX_EVENT_ANIMATION_COMPLETE:
        gx_widget_find(window, PAIRING_INFO_WIN_ID, 1, &w);
        if (w != NULL) {
            gx_animation_drag_disable((GX_ANIMATION *)&mw->animation, 
                (GX_WIDGET *)window);
        }
        break;
    case GX_EVENT_KEY_UP:
        if (event_ptr->gx_event_payload.gx_event_ushortdata[0] == GX_KEY_HOME)
            ux_screen_pop_and_switch((GX_WIDGET *)&pairing_window, NULL);
        break;
    default:
        break;
    }
    return gx_window_event_process(window, event_ptr);
}

UINT pairing_window_create(const char *name, GX_WIDGET *parent, GX_WIDGET **ptr)
{
    struct pairing_window *mw = &mod_window;
    GX_WIDGET *topwin;
    UINT ret;

    ret = gx_studio_named_widget_create((char *)name, parent, &topwin);
    if (ret != GX_SUCCESS)
        return ret;

    gx_widget_event_process_set(topwin, pairing_event_process);
    pairing_guide_window_setup(mw, topwin);
    pairing_infomation_window_setup(mw, topwin);
    gx_animation_create((GX_ANIMATION *)&mw->animation);
    return ret;
}