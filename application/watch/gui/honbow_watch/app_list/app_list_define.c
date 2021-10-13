#include "app_list_define.h"
#include "guix_simple_resources.h"
#include "zephyr.h"
static APP_ID_ENUM app_id_pos_array[12];

// GUI_APP_DEFINE(today_overview, APP_ID_01_TODAY_OVERVIEW, GX_NULL, GX_PIXELMAP_ID_APP_LIST_ICON_TODAY_OVERVIEW_90_90,
// 			   GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
// GUI_APP_DEFINE(sport, APP_ID_02_SPORT, GX_NULL, GX_PIXELMAP_ID_APP_LIST_ICON_SPORT_90_90,
// 			   GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
// GUI_APP_DEFINE(heartrate, APP_ID_03_HEARTRATE, GX_NULL, GX_PIXELMAP_ID_APP_LIST_ICON_HEART_90_90,
// 			   GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
// GUI_APP_DEFINE(blood_oxygen, APP_ID_04_BLOOD_OXYGEN, GX_NULL, GX_PIXELMAP_ID_APP_LIST_ICON_BLOOD_OXYGEN_90_90,
// 			   GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
// GUI_APP_DEFINE(alarm_breathe, APP_ID_05_ALARM_BREATHE, GX_NULL, GX_PIXELMAP_ID_APP_LIST_ICON_BREATHE_90_90,
// 			   GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
// GUI_APP_DEFINE(remind, APP_ID_06_REMINDER, GX_NULL, GX_PIXELMAP_ID_APP_LIST_ICON_REMIND_90_90,
//			   GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
// GUI_APP_DEFINE(alarm, APP_ID_07_ALARM, GX_NULL, GX_PIXELMAP_ID_APP_LIST_ICON_ALARM_90_90,
//			   GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
// GUI_APP_DEFINE(timer, APP_ID_08_TIMER, GX_NULL, GX_PIXELMAP_ID_APP_LIST_ICON_TIMER_90_90,
//			   GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
// GUI_APP_DEFINE(stopwatch, APP_ID_09_STOP_WATCH, GX_NULL, GX_PIXELMAP_ID_APP_LIST_ICON_STOPWATCH_90_90,
//			   GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
// GUI_APP_DEFINE(music, APP_ID_10_MUSIC, GX_NULL, GX_PIXELMAP_ID_APP_LIST_ICON_MUSIC_90_90,
//			   GX_PIXELMAP_ID_APP_LIST_ICON_90_OVERLAY);
void app_list_element_pos_init(void)
{
	for (uint8_t i = 0; i < sizeof(app_id_pos_array) / sizeof(APP_ID_ENUM); i++) {
		if (i + 1 <= APP_ID_MAX) {
			app_id_pos_array[i] = i + APP_ID_MIN;
		} else {
			app_id_pos_array[i] = APP_ID_EMPTY;
		}
	}
}

APP_ID_ENUM app_list_element_sequence_get(uint8_t sequence_id)
{
	if (sequence_id < sizeof(app_id_pos_array) / sizeof(APP_ID_ENUM)) {
		return app_id_pos_array[sequence_id];
	} else {
		return APP_ID_EMPTY;
	}
}

int app_list_element_pos_edit(APP_ID_ENUM *array, uint8_t size)
{
	if (size != sizeof(app_id_pos_array) / sizeof(APP_ID_ENUM)) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < sizeof(app_id_pos_array) / sizeof(APP_ID_ENUM); i++) {
		if (array[i] > APP_ID_MAX) {
			return -EINVAL;
		}
	}

	memcpy(app_id_pos_array, array, size);

	return 0;
}

struct static_btree {
	void *start;
	size_t count;
	size_t size;
};

static struct static_btree root;
extern APP_LIST_META_DATA __app_node_start[];
extern APP_LIST_META_DATA __app_node_end[];
void app_list_tree_init(struct static_btree *tree)
{
	static uint8_t inited = 0;
	if (0 == inited) {
		uintptr_t size = (uintptr_t)__app_node_end - (uintptr_t)__app_node_start;
		tree->start = __app_node_start;
		tree->size = sizeof(APP_LIST_META_DATA);
		tree->count = size / tree->size;
		inited = 1;
	}
}

static int compare(const void *a, const void *b)
{
	const APP_LIST_META_DATA *left = (const APP_LIST_META_DATA *)a;
	const APP_LIST_META_DATA *right = (const APP_LIST_META_DATA *)b;

	return left->app_id - right->app_id;
}

static const APP_LIST_META_DATA *app_find_handle(struct static_btree *tree, APP_ID_ENUM id)
{
	APP_LIST_META_DATA key;
	key.app_id = id;
	return bsearch(&key, tree->start, tree->count, tree->size, compare);
}

const APP_LIST_META_DATA *app_list_element_meta_get(APP_ID_ENUM app_id)
{
	app_list_tree_init(&root);
	const APP_LIST_META_DATA *meta = app_find_handle(&root, app_id);
	return meta;
}

void app_init_func_process(void)
{
	app_list_tree_init(&root);
	uint8_t i = 0;
	for (i = 0; i < root.count; i++) {
		const APP_LIST_META_DATA *meta = (const APP_LIST_META_DATA *)((const char *)__app_node_start + (i * root.size));
		if (meta->init_func) {
			meta->init_func((GX_WINDOW *)meta->app_widget);
		}
	}
}