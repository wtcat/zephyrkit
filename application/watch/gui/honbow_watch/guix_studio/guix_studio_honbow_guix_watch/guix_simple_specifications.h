/*******************************************************************************/
/*  This file is auto-generated by Azure RTOS GUIX Studio. Do not edit this    */
/*  file by hand. Modifications to this file should only be made by running    */
/*  the Azure RTOS GUIX Studio application and re-generating the application   */
/*  specification file(s). For more information please refer to the Azure RTOS */
/*  GUIX Studio User Guide, or visit our web site at azure.com/rtos            */
/*                                                                             */
/*  GUIX Studio Revision 6.1.7.0                                               */
/*  Date (dd.mm.yyyy): 18. 6.2021   Time (hh:mm): 12:05                        */
/*******************************************************************************/


#ifndef _GUIX_SIMPLE_SPECIFICATIONS_H_
#define _GUIX_SIMPLE_SPECIFICATIONS_H_

#include "gx_api.h"

/* Determine if C++ compiler is being used, if so use standard C.              */
#ifdef __cplusplus
extern   "C" {
#endif

/* Define widget ids                                                           */



/* Define animation ids                                                        */

#define GX_NEXT_ANIMATION_ID 1


/* Define user event ids                                                       */

#define GX_NEXT_USER_EVENT_ID GX_FIRST_USER_EVENT


/* Declare properties structures for each utilized widget type                 */

typedef struct GX_STUDIO_WIDGET_STRUCT
{
   GX_CHAR *widget_name;
   USHORT  widget_type;
   USHORT  widget_id;
   #if defined(GX_WIDGET_USER_DATA)
   INT   user_data;
   #endif
   ULONG style;
   ULONG status;
   ULONG control_block_size;
   GX_RESOURCE_ID normal_fill_color_id;
   GX_RESOURCE_ID selected_fill_color_id;
   GX_RESOURCE_ID disabled_fill_color_id;
   UINT (*create_function) (GX_CONST struct GX_STUDIO_WIDGET_STRUCT *, GX_WIDGET *, GX_WIDGET *);
   void (*draw_function) (GX_WIDGET *);
   UINT (*event_function) (GX_WIDGET *, GX_EVENT *);
   GX_RECTANGLE size;
   GX_CONST struct GX_STUDIO_WIDGET_STRUCT *next_widget;
   GX_CONST struct GX_STUDIO_WIDGET_STRUCT *child_widget;
   ULONG control_block_offset;
   GX_CONST void *properties;
} GX_STUDIO_WIDGET;

typedef struct
{
    GX_CONST GX_STUDIO_WIDGET *widget_information;
    GX_WIDGET        *widget;
} GX_STUDIO_WIDGET_ENTRY;

typedef struct
{
    GX_RESOURCE_ID wallpaper_id;
} GX_WINDOW_PROPERTIES;

typedef struct
{
    GX_RESOURCE_ID wallpaper_id;
    VOID (*callback)(GX_HORIZONTAL_LIST *, GX_WIDGET *, INT);
    int total_rows;
} GX_HORIZONTAL_LIST_PROPERTIES;


/* Declare top-level control blocks                                            */

typedef struct APP_STOP_WATCH_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_STOP_WATCH_WINDOW_CONTROL_BLOCK;

typedef struct APP_TIMER_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_TIMER_WINDOW_CONTROL_BLOCK;

typedef struct APP_BREATH_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_BREATH_WINDOW_CONTROL_BLOCK;

typedef struct APP_SPO2_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_SPO2_WINDOW_CONTROL_BLOCK;

typedef struct APP_HEART_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_HEART_WINDOW_CONTROL_BLOCK;

typedef struct LANGUAGE_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} LANGUAGE_WINDOW_CONTROL_BLOCK;

typedef struct MESSAGE_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} MESSAGE_WINDOW_CONTROL_BLOCK;

typedef struct PAIRING_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} PAIRING_WINDOW_CONTROL_BLOCK;

typedef struct ROOT_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
    GX_WINDOW root_window_home_window;
    GX_WINDOW root_window_wf_window;
} ROOT_WINDOW_CONTROL_BLOCK;

typedef struct APP_MUSIC_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_MUSIC_WINDOW_CONTROL_BLOCK;

typedef struct APP_REMINDERS_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_REMINDERS_WINDOW_CONTROL_BLOCK;

typedef struct APP_ALARM_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_ALARM_WINDOW_CONTROL_BLOCK;

typedef struct APP_TODAY_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_TODAY_WINDOW_CONTROL_BLOCK;

typedef struct APP_SPORT_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_SPORT_WINDOW_CONTROL_BLOCK;

typedef struct WF_LIST_CONTROL_BLOCK_STRUCT
{
    GX_HORIZONTAL_LIST_MEMBERS_DECLARE
} WF_LIST_CONTROL_BLOCK;

typedef struct CONTROL_CENTER_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} CONTROL_CENTER_WINDOW_CONTROL_BLOCK;

typedef struct APP_SETTING_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_SETTING_WINDOW_CONTROL_BLOCK;

typedef struct APP_LIST_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_LIST_WINDOW_CONTROL_BLOCK;

typedef struct APP_COMPASS_WINDOW_CONTROL_BLOCK_STRUCT
{
    GX_WINDOW_MEMBERS_DECLARE
} APP_COMPASS_WINDOW_CONTROL_BLOCK;


/* extern statically defined control blocks                                    */

#ifndef GUIX_STUDIO_GENERATED_FILE
extern APP_STOP_WATCH_WINDOW_CONTROL_BLOCK app_stop_watch_window;
extern APP_TIMER_WINDOW_CONTROL_BLOCK app_timer_window;
extern APP_BREATH_WINDOW_CONTROL_BLOCK app_breath_window;
extern APP_SPO2_WINDOW_CONTROL_BLOCK app_spo2_window;
extern APP_HEART_WINDOW_CONTROL_BLOCK app_heart_window;
extern LANGUAGE_WINDOW_CONTROL_BLOCK language_window;
extern MESSAGE_WINDOW_CONTROL_BLOCK message_window;
extern PAIRING_WINDOW_CONTROL_BLOCK pairing_window;
extern ROOT_WINDOW_CONTROL_BLOCK root_window;
extern APP_MUSIC_WINDOW_CONTROL_BLOCK app_music_window;
extern APP_REMINDERS_WINDOW_CONTROL_BLOCK app_reminders_window;
extern APP_ALARM_WINDOW_CONTROL_BLOCK app_alarm_window;
extern APP_TODAY_WINDOW_CONTROL_BLOCK app_today_window;
extern APP_SPORT_WINDOW_CONTROL_BLOCK app_sport_window;
extern WF_LIST_CONTROL_BLOCK wf_list;
extern CONTROL_CENTER_WINDOW_CONTROL_BLOCK control_center_window;
extern APP_SETTING_WINDOW_CONTROL_BLOCK app_setting_window;
extern APP_LIST_WINDOW_CONTROL_BLOCK app_list_window;
extern APP_COMPASS_WINDOW_CONTROL_BLOCK app_compass_window;
#endif

/* Declare event process functions, draw functions, and callback functions     */

VOID wf_list_callback(GX_HORIZONTAL_LIST *, GX_WIDGET *, INT);
UINT wf_list_event(GX_HORIZONTAL_LIST *widget, GX_EVENT *event_ptr);

/* Declare the GX_STUDIO_DISPLAY_INFO structure                                */


typedef struct GX_STUDIO_DISPLAY_INFO_STRUCT 
{
    GX_CONST GX_CHAR *name;
    GX_CONST GX_CHAR *canvas_name;
    GX_CONST GX_THEME **theme_table;
    GX_CONST GX_STRING **language_table;
    USHORT   theme_table_size;
    USHORT   language_table_size;
    UINT     string_table_size;
    UINT     x_resolution;
    UINT     y_resolution;
    GX_DISPLAY *display;
    GX_CANVAS  *canvas;
    GX_WINDOW_ROOT *root_window;
    GX_COLOR   *canvas_memory;
    ULONG      canvas_memory_size;
    USHORT     rotation_angle;
} GX_STUDIO_DISPLAY_INFO;


/* Declare Studio-generated functions for creating top-level widgets           */

UINT gx_studio_window_create(GX_CONST GX_STUDIO_WIDGET *info, GX_WIDGET *control_block, GX_WIDGET *parent);
UINT gx_studio_horizontal_list_create(GX_CONST GX_STUDIO_WIDGET *info, GX_WIDGET *control_block, GX_WIDGET *parent);
GX_WIDGET *gx_studio_widget_create(GX_BYTE *storage, GX_CONST GX_STUDIO_WIDGET *definition, GX_WIDGET *parent);
UINT gx_studio_named_widget_create(char *name, GX_WIDGET *parent, GX_WIDGET **new_widget);
UINT gx_studio_display_configure(USHORT display, UINT (*driver)(GX_DISPLAY *), GX_UBYTE language, USHORT theme, GX_WINDOW_ROOT **return_root);

/* Determine if a C++ compiler is being used.  If so, complete the standard
  C conditional started above.                                                 */
#ifdef __cplusplus
}
#endif

#endif                                       /* sentry                         */
