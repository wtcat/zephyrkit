/* This is a small demo of the high-performance GUIX graphics framework. */

#include <zephyr.h>
#include "demo_guix_smart_watch.h"

//#include "base/clock_tod.h"


#define ID_HOME 0xFFFF
#define ROOT_WIN_TIMER  0x1234
#define SCREEN_IDLE_TIMEOUT (50*GX_TICKS_SECOND)

#define SCREEN_STACK_SIZE  5
#define SCRATCHPAD_PIXELS  MAIN_SCREEN_X_RESOLUTION * MAIN_SCREEN_Y_RESOLUTION * 2
#define ANIMATION_ID_MENU_SLIDE 1


/* */
struct date_string {
    const GX_CHAR **day;
    const GX_CHAR **mon;
};

/* Define prototypes.   */

VOID root_window_draw(GX_WINDOW *root);
UINT root_window_event_handler(GX_WINDOW *window, GX_EVENT *event_ptr);
VOID sys_time_update();
VOID clock_update();
//extern GX_CONST GX_THEME *Main_Screen_theme_table[];
//extern GX_CONST GX_STRING *Main_Screen_language_table[];

extern GX_STUDIO_DISPLAY_INFO guix_smart_watch_display_table[];
GX_COLOR           scratchpad[SCRATCHPAD_PIXELS];
GX_PIXELMAP_BUTTON home_button;

/* Define menu screen list. */
GX_WIDGET *menu_screen_list[] = {
    (GX_WIDGET *)&main_screen.main_screen_time_window,
    (GX_WIDGET *)&menu_window,
    GX_NULL
};

/* Define current visble menus screen index.  */
INT              current_menu = 0;

GX_WINDOW_ROOT  *root;
GX_WIDGET       *current_screen;
static struct observer_base gui_observer;

/* */
#ifdef CONFIG_GUI_SPLIT_BINRES
GX_CONST GX_THEME  *Main_Screen_theme_table[MAIN_SCREEN_THEME_TABLE_SIZE];
GX_CONST GX_STRING *Main_Screen_language_table[MAIN_SCREEN_LANGUAGE_TABLE_SIZE];
GX_PIXELMAP **Main_Screen_theme_default_pixelmap_table;
#endif

static const GX_CHAR *lauguage_en_day_names[7] = {
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"
};

static const GX_CHAR *lauguage_en_month_names[12] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

const GX_CHAR *lauguage_ch_day_names[7] = {
    "周末",
    "周一",
    "周二",
    "周三",
    "周四",
    "周五",
    "周六"
};

static const GX_CHAR *lauguage_ch_month_names[12] = {
    "一月",
    "二月",
    "三月",
    "四月",
    "五月",
    "六月",
    "七月",
    "八月",
    "九月",
    "十月",
    "十一月",
    "十二月"
};


static const struct date_string lauguage_en_string = {
    .day = lauguage_en_day_names,
    .mon = lauguage_en_month_names
};

static const struct date_string lauguage_ch_string = {
    .day = lauguage_ch_day_names,
    .mon = lauguage_ch_month_names
};

static const struct date_string lauguage_string_table[] = {
    [LANGUAGE_ENGLISH] = lauguage_en_string,
    [LANGUAGE_CHINESE] = lauguage_ch_string,
};

GX_ANIMATION slide_animation;
static GX_ANIMATION pulldown_animation;

GX_ANIMATION animation[2];


int __weak rand(void)
{
    return (int)k_uptime_ticks();
}

static const struct date_string *get_lauguage_string(void)
{
    GX_UBYTE id = root->gx_window_root_canvas->gx_canvas_display->gx_display_active_language;
    return lauguage_string_table + id;
}

static int guix_exit_sleep(struct observer_base *obs, 
    unsigned long action, void *data)
{
    if (action == GUIX_EXIT_SLEEP) {
        gx_system_timer_start(root, ROOT_WIN_TIMER, 
            SCREEN_IDLE_TIMEOUT, 0);
    }
    return NOTIFY_DONE;
}

/*************************************************************************************/
UINT guix_main(UINT disp_id, struct guix_driver *drv)
{
#ifdef CONFIG_GUI_SPLIT_BINRES
    GX_THEME *theme;
    GX_STRING **language;
#endif
    GX_RECTANGLE size;
    UINT ret = 0;

    if (disp_id > 0)
        return GX_INVALID_ID;

    /* Initialize GUIX. */
    gx_system_initialize();

    /* Load gui resource */
#ifdef CONFIG_GUI_SPLIT_BINRES
    ret = guix_binres_load(drv, 0, &theme, &language);
    if (ret == GX_SUCCESS) {
        Main_Screen_theme_default_pixelmap_table = theme->theme_pixelmap_table;
        Main_Screen_theme_table[0] = theme;
        guix_smart_watch_display_table[disp_id].theme_table = Main_Screen_theme_table;
        guix_smart_watch_display_table[disp_id].language_table = 
        (GX_CONST GX_STRING **)language;
    }
#endif
    gx_studio_display_configure(MAIN_SCREEN, drv->setup, 
        LANGUAGE_ENGLISH, MAIN_SCREEN_THEME_DEFAULT, &root);

    /* Create main screen */
    gx_studio_named_widget_create("main_screen", (GX_WIDGET *)root, GX_NULL);
    current_screen = (GX_WIDGET *)&main_screen;

    /* Create menu screens. */
    gx_studio_named_widget_create("menu_window", GX_NULL, GX_NULL);

    /* Create message screen. */
    gx_studio_named_widget_create("msg_screen", GX_NULL, GX_NULL);
    msg_list_children_create(&msg_screen.msg_screen_msg_list);

    /* Create message send screen. */
    gx_studio_named_widget_create("msg_send_screen", GX_NULL, GX_NULL);

    /* Create keyboard screen. */
    gx_studio_named_widget_create("keyboard_screen", GX_NULL, GX_NULL);
    gx_single_line_text_input_style_add(&keyboard_screen.keyboard_screen_input_field, GX_STYLE_CURSOR_ALWAYS | GX_STYLE_CURSOR_BLINK);
    keyboard_children_create();

    /* Create weather screens. */
    gx_studio_named_widget_create("weather_screen", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("weather_LosAngeles", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("weather_SanFrancisco", GX_NULL, GX_NULL);
    future_weather_list_children_create((GX_WIDGET *)&weather_screen.weather_screen_weather_info, CITY_NEW_YORK);
    future_weather_list_children_create((GX_WIDGET *)&weather_SanFrancisco.weather_SanFrancisco_weather_info, CITY_SAN_FRANCISCO);
    future_weather_list_children_create((GX_WIDGET *)&weather_LosAngeles.weather_LosAngeles_weather_info, CITY_LOS_ANGELES);

    /* Create healthy screens.  */
    gx_studio_named_widget_create("healthy_screen", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("healthy_pace", GX_NULL, GX_NULL);
    gx_widget_style_add(&healthy_screen.healthy_screen_heart_rate, GX_STYLE_TEXT_COPY);
    gx_widget_style_add(&healthy_pace.healthy_pace_pace, GX_STYLE_TEXT_COPY);

    /* Create game screen. */
    //gx_studio_named_widget_create("game_screen", GX_NULL, GX_NULL);
    //game_list_children_create(&game_screen.game_screen_game_list);

    /* Create clock screen. */
    gx_studio_named_widget_create("clock_screen", GX_NULL, GX_NULL);
    time_list_children_create(&clock_screen.clock_screen_time_list);

    /* Create clock add screen. */
    gx_studio_named_widget_create("clock_add_screen", GX_NULL, GX_NULL);
    city_list_children_create(&clock_add_screen.clock_add_screen_city_list);

    /* Create alarm screen. */
    gx_studio_named_widget_create("alarm_screen", GX_NULL, GX_NULL);
    alarm_list_children_create(&alarm_screen.alarm_screen_alarm_list);

    /* Create alarm add screen. */
    gx_studio_named_widget_create("alarm_add_screen", GX_NULL, GX_NULL);

    /* Create stopwatch screen. */
    gx_studio_named_widget_create("stopwatch_screen", GX_NULL, GX_NULL);
    gx_widget_style_add(&stopwatch_screen.stopwatch_screen_prompt_micro_second, GX_STYLE_TEXT_COPY);
    gx_widget_style_add(&stopwatch_screen.stopwatch_screen_prompt_minute, GX_STYLE_TEXT_COPY);
    gx_widget_style_add(&stopwatch_screen.stopwatch_screen_prompt_second, GX_STYLE_TEXT_COPY);

    /* Create contact screen. */
    gx_studio_named_widget_create("contact_screen", GX_NULL, GX_NULL);
    contact_list_children_create(&contact_screen.contact_screen_contact_list);

    /* Create contact information screen. */
    gx_studio_named_widget_create("contact_info_screen", GX_NULL, GX_NULL);

    /* Create contact information list.  */
    gx_studio_named_widget_create("contact_info_list", GX_NULL, GX_NULL);

    /* Attach contact information list to contact information screen. */
    gx_widget_back_attach((GX_WIDGET *)&contact_info_screen.contact_info_screen_contact_info_window, (GX_WIDGET *)&contact_info_list);
    size = contact_info_screen.contact_info_screen_contact_info_window.gx_widget_size;
    gx_widget_shift((GX_WIDGET *)&contact_info_list, size.gx_rectangle_left, size.gx_rectangle_top, GX_FALSE);

    /* Create contact information edit screen. */
    gx_studio_named_widget_create("contact_info_edit_screen", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("contact_info_edit_list", GX_NULL, GX_NULL);
    gx_widget_back_attach((GX_WIDGET *)&contact_info_edit_screen.contact_info_edit_screen_edit_window, (GX_WIDGET *)&contact_info_edit_list);
    size = contact_info_edit_screen.contact_info_edit_screen_edit_window.gx_widget_size;
    gx_widget_shift((GX_WIDGET *)&contact_info_edit_list, size.gx_rectangle_left, size.gx_rectangle_top, GX_FALSE);

    /* Set text input cursor height. */
    gx_text_input_cursor_height_set(&contact_info_edit_list.contact_info_edit_list_firstname.gx_single_line_text_input_cursor_instance, 22);
    gx_text_input_cursor_height_set(&contact_info_edit_list.contact_info_edit_list_lastname.gx_single_line_text_input_cursor_instance, 22);
    gx_text_input_cursor_height_set(&contact_info_edit_list.contact_info_edit_list_mobile.gx_single_line_text_input_cursor_instance, 22);
    gx_text_input_cursor_height_set(&contact_info_edit_list.contact_info_edit_list_office.gx_single_line_text_input_cursor_instance, 22);
    gx_text_input_cursor_height_set(&contact_info_edit_list.contact_info_edit_list_email.gx_single_line_text_input_cursor_instance, 22);
    gx_text_input_cursor_height_set(&contact_info_edit_list.contact_info_edit_list_address.gx_single_line_text_input_cursor_instance, 22);

    /* Create setting screen.  */
    gx_studio_named_widget_create("settings_screen", GX_NULL, GX_NULL);
    gx_studio_named_widget_create("settings_language", GX_NULL, GX_NULL);

    /* Create scroll-wheel screen */
    gx_studio_named_widget_create("scroll_wheel", GX_NULL, GX_NULL);

    /* Create slide animation control block.  */
    gx_animation_create(&pulldown_animation);
    gx_animation_create(&slide_animation);
    gx_animation_create(&animation[0]);
    gx_animation_create(&animation[1]);
    
    gx_animation_landing_speed_set(&pulldown_animation, 50);
    gx_animation_landing_speed_set(&slide_animation, 50);
    gx_animation_landing_speed_set(&animation[0], 50);
    gx_animation_landing_speed_set(&animation[1], 50);

    /* Set root window draw function.  */
    gx_widget_draw_set((GX_WIDGET *)root, root_window_draw);
    gx_widget_event_process_set((GX_WIDGET *)root, root_window_event_handler);

    /* Add text copy style to time display prompts. */
    gx_widget_style_add(&main_screen.main_screen_hour, GX_STYLE_TEXT_COPY);
    gx_widget_style_add(&main_screen.main_screen_minute, GX_STYLE_TEXT_COPY);

    /* Show the root window.  */
    gx_widget_show(root);
    gui_observer.update = guix_exit_sleep;
    gui_observer.next = NULL;
    gui_observer.priority = 0;
    guix_suspend_notify_register(&gui_observer);

    /* start GUIX thread */
    gx_system_start();

out:
    return ret;
}



/*************************************************************************************/
VOID root_window_draw(GX_WINDOW *root)
{
GX_PIXELMAP *p_map = GX_NULL;
INT          x_pos;

    /* Call default root window draw. */
    gx_window_draw(root);

#if 0
    gx_context_pixelmap_get(GX_PIXELMAP_ID_SMART_WATCH_FRAME, &p_map);

    if (p_map)
    {
        /* Draw smart watch frame.  */
        x_pos = (MAIN_SCREEN_X_RESOLUTION - p_map->gx_pixelmap_width) >> 1;
        gx_context_fill_color_set(GX_COLOR_ID_GRAY);
        gx_canvas_pixelmap_draw(x_pos, 0, p_map);
    }

    gx_context_pixelmap_get(GX_PIXELMAP_ID_MS_AZURE_LOGO_SMALL, &p_map);

    if (p_map)
    {
        /* Draw logo. */
        gx_canvas_pixelmap_draw(15, 15, p_map);
    }

    gx_context_pixelmap_get(GX_PIXELMAP_ID_MENU_ICON_HOME, &p_map);
#endif
}

/*************************************************************************************/

UINT root_window_event_handler(GX_WINDOW *window, GX_EVENT *event_ptr)
{
    switch (event_ptr->gx_event_type)
    {
    case GX_EVENT_PEN_DOWN:
        gx_system_timer_start(root, ROOT_WIN_TIMER, SCREEN_IDLE_TIMEOUT, 0);
        break;

    case GX_EVENT_SHOW:
        gx_system_timer_start(root, ROOT_WIN_TIMER, SCREEN_IDLE_TIMEOUT, 0);
        return gx_window_root_event_process((GX_WINDOW_ROOT *)window, event_ptr);

    case GX_EVENT_TIMER:
        if (event_ptr->gx_event_payload.gx_event_timer_id == ROOT_WIN_TIMER) {
            GX_EVENT event;
            event.gx_event_type = GX_EVENT_TERMINATE;
            event.gx_event_target = NULL;
            printf("GUIX enter in sleep\n");
            gx_system_event_send(&event);
        }
        break;
  
    case GX_SIGNAL(ID_HOME, GX_EVENT_CLICKED):
        if (current_screen != (GX_WIDGET *)&main_screen)
        {
            /* Switch to main screen. */
            screen_switch((GX_WIDGET *)window, (GX_WIDGET *)&main_screen);
        }
        break;

    default:
        return gx_window_root_event_process((GX_WINDOW_ROOT *)window, event_ptr);
    }

    return 0;
}

/*************************************************************************************/
VOID start_slide_animation(GX_WINDOW *window)
{
GX_ANIMATION_INFO slide_animation_info;

    memset(&slide_animation_info, 0, sizeof(GX_ANIMATION_INFO));
    slide_animation_info.gx_animation_parent = (GX_WIDGET *)window;
    slide_animation_info.gx_animation_style = GX_ANIMATION_SCREEN_DRAG | GX_ANIMATION_HORIZONTAL;
    slide_animation_info.gx_animation_id = ANIMATION_ID_MENU_SLIDE;
    slide_animation_info.gx_animation_frame_interval = 1;
    slide_animation_info.gx_animation_slide_screen_list = menu_screen_list;
    slide_animation_info.gx_animation_steps = 2;
    gx_animation_drag_enable(&slide_animation, (GX_WIDGET *)window, &slide_animation_info);
}


/*************************************************************************************/
VOID clock_update()
{
#if 0
    GX_CHAR hour_string_buffer[10];
    GX_CHAR minute_string_buffer[10];
    GX_CHAR am_pm_buffer[20];
    GX_CHAR date_string_buffer[32];
    const struct date_string *dstring;
    GX_STRING string;

    struct time_tod local_time;
    clock_get_tod(&local_time);

    if (local_time.second & 0x1)
    {
        gx_widget_fill_color_set(&main_screen.main_screen_lower_dot, GX_COLOR_ID_MILK_WHITE, GX_COLOR_ID_MILK_WHITE, GX_COLOR_ID_MILK_WHITE);
        gx_widget_fill_color_set(&main_screen.main_screen_upper_dot, GX_COLOR_ID_MILK_WHITE, GX_COLOR_ID_MILK_WHITE, GX_COLOR_ID_MILK_WHITE);
    }

    else
    {
        gx_widget_fill_color_set(&main_screen.main_screen_lower_dot, GX_COLOR_ID_GRAY, GX_COLOR_ID_GRAY, GX_COLOR_ID_GRAY);
        gx_widget_fill_color_set(&main_screen.main_screen_upper_dot, GX_COLOR_ID_GRAY, GX_COLOR_ID_GRAY, GX_COLOR_ID_GRAY);
    }

    if (local_time.hour < 12)
    {
        sprintf(hour_string_buffer, "%02d", local_time.hour);
        sprintf(minute_string_buffer, "%02d", local_time.minute);
        memcpy(am_pm_buffer, "AM", 3);
    }
    else
    {
        sprintf(hour_string_buffer, "%02d", local_time.hour);
        sprintf(minute_string_buffer, "%02d", local_time.minute);
        memcpy(am_pm_buffer, "PM", 3);
    }

    string.gx_string_ptr = hour_string_buffer;
    string.gx_string_length = string_length_get(hour_string_buffer, MAX_TIME_TEXT_LENGTH);
    gx_prompt_text_set_ext(&main_screen.main_screen_hour, &string);

    string.gx_string_ptr = minute_string_buffer;
    string.gx_string_length = string_length_get(minute_string_buffer, MAX_TIME_TEXT_LENGTH);
    gx_prompt_text_set_ext(&main_screen.main_screen_minute, &string);

    string.gx_string_ptr = am_pm_buffer;
    string.gx_string_length = string_length_get(am_pm_buffer, MAX_TIME_TEXT_LENGTH);
    gx_prompt_text_set_ext(&main_screen.main_screen_am_pm, &string);

    dstring = get_lauguage_string();
    sprintf(date_string_buffer, "%s, %s %02d", 
        dstring->day[0],
        dstring->mon[local_time.month - 1], 
        local_time.day);

    string.gx_string_ptr = date_string_buffer;
    string.gx_string_length = string_length_get(date_string_buffer, sizeof(date_string_buffer) - 1);
    gx_prompt_text_set_ext(&main_screen.main_screen_date, &string);
#endif
}

/*************************************************************************************/
UINT main_screen_event_handler(GX_WINDOW *window, GX_EVENT *event_ptr)
{

    switch (event_ptr->gx_event_type)
    {
    case GX_EVENT_SHOW:
        /* Set current time. */
        clock_update();
        start_slide_animation(window);
        /* Start a timer to update time. */
        gx_system_timer_start(&main_screen, CLOCK_TIMER, GX_TICKS_SECOND, GX_TICKS_SECOND);
        return template_main_event_handler(window, event_ptr);

    case GX_EVENT_HIDE:
        gx_animation_drag_disable(&slide_animation, (GX_WIDGET *)window);
        break;

    case GX_SIGNAL(ID_MESSAGE, GX_EVENT_CLICKED):
        screen_switch(window->gx_widget_parent, (GX_WIDGET *)&msg_screen);
        break;
    case GX_SIGNAL(ID_WEATHER, GX_EVENT_CLICKED):
        screen_switch(window->gx_widget_parent, (GX_WIDGET *)&weather_screen);
        break;
    case GX_SIGNAL(ID_HEALTHY, GX_EVENT_CLICKED):
        screen_switch(window->gx_widget_parent, (GX_WIDGET *)&healthy_screen);
        break;
    //case GX_SIGNAL(ID_GAMES, GX_EVENT_CLICKED):
    //    screen_switch(window->gx_widget_parent, (GX_WIDGET *)&game_screen);
    //    break;
    case GX_SIGNAL(ID_CLOCK, GX_EVENT_CLICKED):
        screen_switch(window->gx_widget_parent, (GX_WIDGET *)&clock_screen);
        break;
    case GX_SIGNAL(ID_CONTACTS, GX_EVENT_CLICKED):
        screen_switch(window->gx_widget_parent, (GX_WIDGET *)&contact_screen);
        break;
    case GX_SIGNAL(ID_SETTINGS, GX_EVENT_CLICKED):
        screen_switch(window->gx_widget_parent, (GX_WIDGET *)&settings_screen);
        break;
    case GX_SIGNAL(ID_SCROLL_WHELL, GX_EVENT_CLICKED):
        screen_switch(window->gx_widget_parent, (GX_WIDGET *)&scroll_wheel);
        break;
    case GX_EVENT_TIMER:
        if (event_ptr->gx_event_payload.gx_event_timer_id == CLOCK_TIMER)
        {
            clock_update();
        }
        break;
    }
    return gx_window_event_process(window, event_ptr);
}


/*************************************************************************************/
VOID root_home_button_draw(GX_PIXELMAP_BUTTON *widget)
{
    if (widget->gx_widget_style & GX_STYLE_BUTTON_PUSHED)
    {
        gx_pixelmap_button_draw(widget);
    }
}

/*************************************************************************************/
VOID screen_switch(GX_WIDGET *parent, GX_WIDGET *new_screen)
{
    gx_widget_detach(current_screen);
    gx_widget_attach(parent, new_screen);
    current_screen = new_screen;
}

/*************************************************************************************/
UINT string_length_get(GX_CONST GX_CHAR* input_string, UINT max_string_length)
{
    UINT length = 0;

    if (input_string)
    {
        /* Traverse the string.  */
        for (length = 0; input_string[length]; length++)
        {
            /* Check if the string length is bigger than the max string length.  */
            if (length >= max_string_length)
            {
                break;
            }
        }
    }

    return length;
}
