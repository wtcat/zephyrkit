/*******************************************************************************/
/*  This file is auto-generated by Azure RTOS GUIX Studio. Do not edit this */
/*  file by hand. Modifications to this file should only be made by running */
/*  the Azure RTOS GUIX Studio application and re-generating the application */
/*  specification file(s). For more information please refer to the Azure RTOS
 */
/*  GUIX Studio User Guide, or visit our web site at azure.com/rtos */
/*                                                                             */
/*  GUIX Studio Revision 6.1.0.0 */
/*  Date (dd.mm.yyyy):  3. 2.2021   Time (hh:mm): 13:46 */
/*******************************************************************************/

#ifndef _GUIX_SIMPLE_SPECIFICATIONS_H_
#define _GUIX_SIMPLE_SPECIFICATIONS_H_

#include "gx_api.h"

/* Determine if C++ compiler is being used, if so use standard C. */
#ifdef __cplusplus
extern "C"
{
#endif

	/* Define widget ids */

#define ID_ADD 1

	/* Define animation ids */

#define GX_NEXT_ANIMATION_ID 1

	/* Define user event ids */

#define GX_NEXT_USER_EVENT_ID GX_FIRST_USER_EVENT

	/* Declare properties structures for each utilized widget type */

	typedef struct GX_STUDIO_WIDGET_STRUCT {
		GX_CHAR *widget_name;
		USHORT widget_type;
		USHORT widget_id;
#if defined(GX_WIDGET_USER_DATA)
		INT user_data;
#endif
		ULONG style;
		ULONG status;
		ULONG control_block_size;
		GX_RESOURCE_ID normal_fill_color_id;
		GX_RESOURCE_ID selected_fill_color_id;
		GX_RESOURCE_ID disabled_fill_color_id;
		UINT(*create_function)
		(GX_CONST struct GX_STUDIO_WIDGET_STRUCT *, GX_WIDGET *, GX_WIDGET *);
		void (*draw_function)(GX_WIDGET *);
		UINT (*event_function)(GX_WIDGET *, GX_EVENT *);
		GX_RECTANGLE size;
		GX_CONST struct GX_STUDIO_WIDGET_STRUCT *next_widget;
		GX_CONST struct GX_STUDIO_WIDGET_STRUCT *child_widget;
		ULONG control_block_offset;
		GX_CONST void *properties;
	} GX_STUDIO_WIDGET;

	typedef struct {
		GX_CONST GX_STUDIO_WIDGET *widget_information;
		GX_WIDGET *widget;
	} GX_STUDIO_WIDGET_ENTRY;

	typedef struct {
		GX_RESOURCE_ID normal_pixelmap_id;
		GX_RESOURCE_ID selected_pixelmap_id;
		GX_RESOURCE_ID disabled_pixelmap_id;
	} GX_PIXELMAP_BUTTON_PROPERTIES;

	typedef struct {
		GX_RESOURCE_ID normal_pixelmap_id;
		GX_RESOURCE_ID selected_pixelmap_id;
	} GX_ICON_PROPERTIES;

	typedef struct {
		GX_RESOURCE_ID string_id;
		GX_RESOURCE_ID font_id;
		GX_RESOURCE_ID normal_text_color_id;
		GX_RESOURCE_ID selected_text_color_id;
		GX_RESOURCE_ID disabled_text_color_id;
	} GX_PROMPT_PROPERTIES;

	typedef struct {
		GX_RESOURCE_ID wallpaper_id;
	} GX_WINDOW_PROPERTIES;

	typedef struct {
		GX_RESOURCE_ID wallpaper_id;
		VOID (*callback)(GX_VERTICAL_LIST *, GX_WIDGET *, INT);
		int total_rows;
	} GX_VERTICAL_LIST_PROPERTIES;

	/* Declare top-level control blocks */

	typedef struct ALARM_LIST_CONTROL_BLOCK_STRUCT {
		GX_VERTICAL_LIST_MEMBERS_DECLARE
		GX_SCROLLBAR alarm_list_vertical_scroll_9;
	} ALARM_LIST_CONTROL_BLOCK;

	typedef struct ALARM_WINDOW_CONTROL_BLOCK_STRUCT {
		GX_WINDOW_MEMBERS_DECLARE
		GX_WINDOW alarm_window_window_6;
		GX_PIXELMAP_BUTTON alarm_window_alarm_add;
		GX_VERTICAL_LIST alarm_window_alarm_list;
		GX_SCROLLBAR alarm_window_vertical_scroll_9;
	} ALARM_WINDOW_CONTROL_BLOCK;

	typedef struct WINDOW_2_CONTROL_BLOCK_STRUCT {
		GX_WINDOW_MEMBERS_DECLARE
		GX_VERTICAL_LIST window_2_vertical_list;
		GX_WINDOW window_2_window_4;
		GX_ICON window_2_icon_4;
		GX_PROMPT window_2_prompt_5;
		GX_WINDOW window_2_window;
		GX_ICON window_2_icon;
		GX_PROMPT window_2_prompt;
		GX_WINDOW window_2_window_1;
		GX_ICON window_2_icon_1;
		GX_PROMPT window_2_prompt_1;
		GX_WINDOW window_2_window_2;
		GX_ICON window_2_icon_2;
		GX_PROMPT window_2_prompt_2;
		GX_WINDOW window_2_window_3;
		GX_ICON window_2_icon_3;
		GX_PROMPT window_2_prompt_3;
		GX_WINDOW window_2_window_5;
		GX_ICON window_2_icon_5;
		GX_PROMPT window_2_prompt_4;
	} WINDOW_2_CONTROL_BLOCK;

	typedef struct STATUS_WINDOW_CONTROL_BLOCK_STRUCT {
		GX_WINDOW_MEMBERS_DECLARE
		GX_PIXELMAP_BUTTON status_window_pixelmap_button;
		GX_PIXELMAP_BUTTON status_window_pixelmap_button_1;
		GX_PIXELMAP_BUTTON status_window_pixelmap_button_2;
	} STATUS_WINDOW_CONTROL_BLOCK;

	typedef struct ROOT_WINDOW_CONTROL_BLOCK_STRUCT {
		GX_WINDOW_MEMBERS_DECLARE
		GX_WINDOW root_window_main_window;
		GX_WINDOW root_window_watch_face_window;
	} ROOT_WINDOW_CONTROL_BLOCK;

	typedef struct WINDOW_CONTROL_BLOCK_STRUCT {
		GX_WINDOW_MEMBERS_DECLARE
		GX_WINDOW window_window_7;
	} WINDOW_CONTROL_BLOCK;

	/* extern statically defined control blocks */

#ifndef GUIX_STUDIO_GENERATED_FILE
	extern ALARM_LIST_CONTROL_BLOCK alarm_list;
	extern ALARM_WINDOW_CONTROL_BLOCK alarm_window;
	extern WINDOW_2_CONTROL_BLOCK window_2;
	extern STATUS_WINDOW_CONTROL_BLOCK status_window;
	extern ROOT_WINDOW_CONTROL_BLOCK root_window;
	extern WINDOW_CONTROL_BLOCK window;
#endif

	/* Declare event process functions, draw functions, and callback functions
	 */

	VOID alarm_list_row_create(GX_VERTICAL_LIST *, GX_WIDGET *, INT);
	UINT alarm_screen_event_handler(GX_WINDOW *widget, GX_EVENT *event_ptr);
	UINT alarm_add_screen_event_handler(GX_WINDOW *widget, GX_EVENT *event_ptr);
	UINT alarm_add_event(GX_PIXELMAP_BUTTON *widget, GX_EVENT *event_ptr);
	VOID custom_pixelmap_button_draw(GX_PIXELMAP_BUTTON *widget);
	UINT alarm_list_win_event(GX_VERTICAL_LIST *widget, GX_EVENT *event_ptr);
	VOID callback_name(GX_VERTICAL_LIST *, GX_WIDGET *, INT);
	UINT root_event(GX_WINDOW *widget, GX_EVENT *event_ptr);
	UINT main_disp_handler(GX_WINDOW *widget, GX_EVENT *event_ptr);
	VOID main_disp_draw(GX_WINDOW *widget);

	/* Declare the GX_STUDIO_DISPLAY_INFO structure */

	typedef struct GX_STUDIO_DISPLAY_INFO_STRUCT {
		GX_CONST GX_CHAR *name;
		GX_CONST GX_CHAR *canvas_name;
		GX_CONST GX_THEME **theme_table;
		GX_CONST GX_STRING **language_table;
		USHORT theme_table_size;
		USHORT language_table_size;
		UINT string_table_size;
		UINT x_resolution;
		UINT y_resolution;
		GX_DISPLAY *display;
		GX_CANVAS *canvas;
		GX_WINDOW_ROOT *root_window;
		GX_COLOR *canvas_memory;
		ULONG canvas_memory_size;
	} GX_STUDIO_DISPLAY_INFO;

	/* Declare Studio-generated functions for creating top-level widgets */

	UINT gx_studio_pixelmap_button_create(GX_CONST GX_STUDIO_WIDGET *info,
										  GX_WIDGET *control_block,
										  GX_WIDGET *parent);
	UINT gx_studio_icon_create(GX_CONST GX_STUDIO_WIDGET *info,
							   GX_WIDGET *control_block, GX_WIDGET *parent);
	UINT gx_studio_prompt_create(GX_CONST GX_STUDIO_WIDGET *info,
								 GX_WIDGET *control_block, GX_WIDGET *parent);
	UINT gx_studio_window_create(GX_CONST GX_STUDIO_WIDGET *info,
								 GX_WIDGET *control_block, GX_WIDGET *parent);
	UINT gx_studio_vertical_list_create(GX_CONST GX_STUDIO_WIDGET *info,
										GX_WIDGET *control_block,
										GX_WIDGET *parent);
	UINT gx_studio_vertical_scrollbar_create(GX_CONST GX_STUDIO_WIDGET *info,
											 GX_WIDGET *control_block,
											 GX_WIDGET *parent);
	GX_WIDGET *gx_studio_widget_create(GX_BYTE *storage,
									   GX_CONST GX_STUDIO_WIDGET *definition,
									   GX_WIDGET *parent);
	UINT gx_studio_named_widget_create(char *name, GX_WIDGET *parent,
									   GX_WIDGET **new_widget);
	UINT gx_studio_display_configure(USHORT display,
									 UINT (*driver)(GX_DISPLAY *),
									 GX_UBYTE language, USHORT theme,
									 GX_WINDOW_ROOT **return_root);

/* Determine if a C++ compiler is being used.  If so, complete the standard
  C conditional started above. */
#ifdef __cplusplus
}
#endif

#endif /* sentry                         */
