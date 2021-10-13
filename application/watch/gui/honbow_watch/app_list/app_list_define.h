#ifndef __APP_LIST_DEFINE_H__
#define __APP_LIST_DEFINE_H__
#include "gx_api.h"
#include "zephyr.h"

typedef enum {
	APP_ID_EMPTY = 0,
	APP_ID_MIN,
	APP_ID_01_TODAY_OVERVIEW = APP_ID_MIN,
	APP_ID_02_SPORT,
	APP_ID_03_HEARTRATE,
	APP_ID_04_BLOOD_OXYGEN,
	APP_ID_05_ALARM_BREATHE,
	APP_ID_06_REMINDER,
	APP_ID_07_ALARM,
	APP_ID_08_TIMER,
	APP_ID_09_STOP_WATCH,
	APP_ID_10_MUSIC,
	APP_ID_11_COMPASS,
	APP_ID_12_SETTING,
	APP_ID_MAX = APP_ID_12_SETTING,
} APP_ID_ENUM;

typedef void (*GX_APP_INIT_FUNC)(GX_WINDOW *);

typedef struct {
	GX_WIDGET *app_widget;
	GX_RESOURCE_ID res_id_normal;
	GX_RESOURCE_ID res_id_select;
	APP_ID_ENUM app_id;
	GX_APP_INIT_FUNC init_func;
} APP_LIST_META_DATA;

#define _APP_NAME(n1, n2) __gui_app_var_##n1

#define _GUI_APP_DEFINE(_name, _app_id, _app_widget, _res_id_normal, _res_id_select)                                   \
	__in_section(app_node_, _app_id, 0) __used static const APP_LIST_META_DATA _APP_NAME(_name, _app_id) = {           \
		.app_widget = _app_widget,                                                                                     \
		.res_id_normal = _res_id_normal,                                                                               \
		.res_id_select = _res_id_select,                                                                               \
		.app_id = _app_id,                                                                                             \
		.init_func = NULL,                                                                                             \
	}

#define _GUI_APP_DEFINE_WITH_INIT(_name, _app_id, _app_widget, _res_id_normal, _res_id_select, _init_func)             \
	__in_section(app_node_, _app_id, 0) __used static const APP_LIST_META_DATA _APP_NAME(_name, _app_id) = {           \
		.app_widget = _app_widget,                                                                                     \
		.res_id_normal = _res_id_normal,                                                                               \
		.res_id_select = _res_id_select,                                                                               \
		.app_id = _app_id,                                                                                             \
		.init_func = _init_func,                                                                                       \
	}
#define GUI_APP_DEFINE(_name, _app_id, _app_widget, _res_id_normal, _res_id_select)                                    \
	_GUI_APP_DEFINE(_name, _app_id, _app_widget, _res_id_normal, _res_id_select)

#define GUI_APP_DEFINE_WITH_INIT(_name, _app_id, _app_widget, _res_id_normal, _res_id_select, _init_func)              \
	_GUI_APP_DEFINE_WITH_INIT(_name, _app_id, _app_widget, _res_id_normal, _res_id_select, _init_func)

void app_list_element_pos_init(void);
APP_ID_ENUM app_list_element_sequence_get(uint8_t sequence_id);
const APP_LIST_META_DATA *app_list_element_meta_get(APP_ID_ENUM app_id);
int app_list_element_pos_edit(APP_ID_ENUM *array, uint8_t size);
void app_init_func_process(void);
#endif