/* This is a demo of the high-performance GUIX graphics framework. */

#include <stdio.h>
#include "gx_api.h"
#include "guix_smart_watch_resources.h"
#include "guix_smart_watch_specifications.h"


extern SCROLL_WHEEL_CONTROL_BLOCK scroll_wheel;


static UINT string_length_get(GX_CONST GX_CHAR *s, 
    UINT max_len)
{
    int i = 0;
    while (s[i] != '\0' && i < max_len)
        i++;
    return i;
}

static VOID day_count_update(VOID)
{
    int year;
    int month;
    int day_count;
    int new_day_count;

    GX_NUMERIC_SCROLL_WHEEL *day_wheel = &scroll_wheel.scroll_wheel_scroll_wheel_day;
    GX_NUMERIC_SCROLL_WHEEL *year_wheel = &scroll_wheel.scroll_wheel_scroll_wheel_year_1;
    GX_STRING_SCROLL_WHEEL *month_wheel = &scroll_wheel.scroll_wheel_scroll_wheel_month;

    year = year_wheel->gx_numeric_scroll_wheel_start_val;
    year += year_wheel->gx_scroll_wheel_selected_row;

    month = month_wheel->gx_scroll_wheel_selected_row + 1;
    day_count = GX_ABS(day_wheel->gx_numeric_scroll_wheel_end_val - 
        day_wheel->gx_numeric_scroll_wheel_start_val) + 1;

    switch (month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        new_day_count = 31;
        break;
    case 2:
        if (!(year % 400) || (!(year % 4) && (year % 400))) {
            new_day_count = 29;
        } else {
            new_day_count = 28;
        }
        break;

    default:
        new_day_count = 30;
        break;
    }

    if (new_day_count != day_count)
        gx_numeric_scroll_wheel_range_set(day_wheel, 1, new_day_count);
}

static VOID animation_speed_set(VOID)
{
    GX_NUMERIC_SCROLL_WHEEL *day_wheel = &scroll_wheel.scroll_wheel_scroll_wheel_day;
    GX_NUMERIC_SCROLL_WHEEL *year_wheel = &scroll_wheel.scroll_wheel_scroll_wheel_year_1;
    GX_STRING_SCROLL_WHEEL *month_wheel = &scroll_wheel.scroll_wheel_scroll_wheel_month;

    gx_scroll_wheel_speed_set(day_wheel, GX_FIXED_VAL_MAKE(1), 200, 10, 2);
    gx_scroll_wheel_speed_set(month_wheel, 512, 200, 10, 2);
    gx_scroll_wheel_speed_set(year_wheel, GX_FIXED_VAL_MAKE(2), 200, 10, 2);
}

UINT scroll_wheel_screen_event_process(GX_WINDOW *window, 
    GX_EVENT *event_ptr)
{
    switch (event_ptr->gx_event_type)
    {
    case GX_EVENT_SHOW:
        gx_window_event_process(window, event_ptr);
        animation_speed_set();
        break;
    case GX_SIGNAL(ID_SCROLL_WHEEL_MONTH, GX_EVENT_LIST_SELECT):
    case GX_SIGNAL(ID_SCROLL_WHEEL_YEAR, GX_EVENT_LIST_SELECT):
        day_count_update();
        break;

    default:
        return gx_window_event_process(window, event_ptr);
    }

    return 0;
}

UINT day_wheel_value_format(GX_NUMERIC_SCROLL_WHEEL *wheel, INT row, 
    GX_STRING *string)
{
    sprintf(wheel->gx_numeric_scroll_wheel_string_buffer, "%02d", row + 1);
    string->gx_string_ptr = wheel->gx_numeric_scroll_wheel_string_buffer;
    string->gx_string_length = string_length_get(string->gx_string_ptr, 
        GX_NUMERIC_SCROLL_WHEEL_STRING_BUFFER_SIZE - 1);
    return GX_SUCCESS;
}



