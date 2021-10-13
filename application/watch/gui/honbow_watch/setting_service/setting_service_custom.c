#include "setting_service_custom.h"
#include <settings/settings.h>
#include <logging/log.h>
#include <errno.h>
#include "storage/flash_map.h"
#include "zephyr.h"
LOG_MODULE_REGISTER(setting_service);

typedef uint8_t LENGTH_OF_DATA;
const LENGTH_OF_DATA setting_meta[SETTING_CUSTOM_DATA_ID_MAX] = {
	[SETTING_CUSTOM_DATA_ID_BRIGHTNESS] = 1,		// length of brightness is 1
	[SETTING_CUSTOM_DATA_ID_BRIGHT_TIME] = 1,		// length of bright time is 1
	[SETTING_CUSTOM_DATA_ID_WRIST_RAISE] = 1,		// length of wrist raise is 1
	[SETTING_CUSTOM_DATA_ID_NIGHT_MODE] = 5,		// length of night mode cfg is 5
	[SETTING_CUSTOM_DATA_ID_NIGHT_MODE_CUSTOM] = 5, // length of night mode custom cfg is 1
	[SETTING_CUSTOM_DATA_ID_APP_VIEW_TYPE] = 1,
};
static void setting_service_default_value(void)
{
	uint8_t data[5];

	//默认亮度值设置
	data[0] = 125;
	setting_service_set(SETTING_CUSTOM_DATA_ID_BRIGHTNESS, &data, setting_meta[SETTING_CUSTOM_DATA_ID_BRIGHTNESS],
						false);

	//默认亮屏时间设置
	data[0] = 10;
	setting_service_set(SETTING_CUSTOM_DATA_ID_BRIGHT_TIME, &data, setting_meta[SETTING_CUSTOM_DATA_ID_BRIGHT_TIME],
						false);

	//抬碗亮屏默认值
	data[0] = 1; //默认开启抬碗亮屏，为0则不开启
	setting_service_set(SETTING_CUSTOM_DATA_ID_WRIST_RAISE, &data, setting_meta[SETTING_CUSTOM_DATA_ID_WRIST_RAISE],
						false);

	//夜晚模式默认参数
	data[0] = 0; //默认不开启
	data[1] = 22;
	data[2] = 0; //开始世界默认22:00
	data[3] = 7;
	data[4] = 0; //结束时间默认07:00
	setting_service_set(SETTING_CUSTOM_DATA_ID_NIGHT_MODE, &data, setting_meta[SETTING_CUSTOM_DATA_ID_NIGHT_MODE],
						false);

	//夜晚模式客制化默认参数
	data[0] = 0; //默认不开启
	data[1] = 22;
	data[2] = 0; //开始世界默认22:00
	data[3] = 7;
	data[4] = 0; //结束时间默认07:00
	setting_service_set(SETTING_CUSTOM_DATA_ID_NIGHT_MODE_CUSTOM, &data,
						setting_meta[SETTING_CUSTOM_DATA_ID_NIGHT_MODE_CUSTOM], false);

	data[0] = 0; //默认网格样式
	setting_service_set(SETTING_CUSTOM_DATA_ID_APP_VIEW_TYPE, &data, setting_meta[SETTING_CUSTOM_DATA_ID_APP_VIEW_TYPE],
						false);

	setting_service_store_sync();
}

/***************************************分割线，以上为定制区**********************************************/
static uint32_t setting_misc_total_size;
static uint8_t setting_service_init_flag = 0;
static char store_buff[256];
#if CONFIG_SETTINGS
static int setting_misc_data_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int rc;
	if (settings_name_steq(name, "params", &next) && !next) {
		if (len != setting_misc_total_size) {
			return -EINVAL;
		}
		rc = read_cb(cb_arg, (void *)&store_buff, setting_misc_total_size);
		if (rc >= 0) {
			return 0;
		}
		return rc;
	}
	return -ENOENT;
}

struct settings_handler setting_misc_conf = {.name = "setting/misc", .h_set = setting_misc_data_set};
#endif
void setting_service_init(void)
{
	setting_misc_total_size = 0;
	for (uint32_t i = 0; i < SETTING_CUSTOM_DATA_ID_MAX; i++) {
		setting_misc_total_size += setting_meta[i];
	}

	if (setting_misc_total_size > sizeof(store_buff)) {
		LOG_ERR("[%s]u should increase buff size.", __FUNCTION__);
		return;
	}
	LOG_INF("[%s]size of setting misc = %d, buff size = %d", __FUNCTION__, setting_misc_total_size, sizeof(store_buff));

#if CONFIG_SETTINGS
	int ret = settings_subsys_init();
	if (ret != 0) {
		int rc;
		const struct flash_area *_fa_p_ext_flash;
		rc = flash_area_open(FLASH_AREA_ID(storage), &_fa_p_ext_flash);
		if (!rc) {
			if (!flash_area_erase(_fa_p_ext_flash, 0, _fa_p_ext_flash->fa_size)) {
				settings_subsys_init();
			}
		}
	}
	settings_register(&setting_misc_conf);
	ret = settings_load_subtree("setting/misc");

	if (store_buff[SETTING_CUSTOM_DATA_ID_BRIGHTNESS] == 0) { //亮度不可能为0.利用这个字节判断flash中是否存储了数据
		LOG_ERR("[%s] misc params set as default", __FUNCTION__);
		setting_service_init_flag = 1;
		setting_service_default_value(); // give default value
	}
#endif
	setting_service_init_flag = 1;
}

void setting_service_set(SETTING_CUSTOM_DATA_ID id, void *data, uint8_t data_size, bool sync_flag)
{
	if (0 == setting_service_init_flag) {
		LOG_ERR("[%s]setting service init fail or not inited!", __FUNCTION__);
		return;
	}

	if (data_size != setting_meta[id]) {
		LOG_ERR("[%s]data_size is wrong!, expect is %d", __FUNCTION__, setting_meta[id]);
		return;
	}

	uint32_t write_index = 0;

	for (uint32_t i = 0; i < id; i++) {
		write_index += setting_meta[i];
	}
	memcpy(&store_buff[write_index], data, data_size);

	if (true == sync_flag) {
		settings_save_one("setting/misc/params", (void *)&store_buff, setting_misc_total_size);
	}
}

void setting_service_get(SETTING_CUSTOM_DATA_ID id, void *data, uint8_t data_size)
{
	if (0 == setting_service_init_flag) {
		LOG_ERR("[%s]setting service init fail or not inited!\n", __FUNCTION__);
		return;
	}

	if (data_size != setting_meta[id]) {
		LOG_ERR("[%s]data_size is wrong!, expect is %d", __FUNCTION__, setting_meta[id]);
		return;
	}

	if (data == NULL) {
		LOG_ERR("[%s]data is a null pointer!", __FUNCTION__);
	}

	uint32_t write_index = 0;

	for (uint32_t i = 0; i < id; i++) {
		write_index += setting_meta[i];
	}

	memcpy(data, &store_buff[write_index], data_size);
}

void setting_service_store_sync(void)
{
	if (0 == setting_service_init_flag) {
		LOG_ERR("[%s]setting service init fail or not inited!", __FUNCTION__);
		return;
	}

	settings_save_one("setting/misc/params", (void *)&store_buff, setting_misc_total_size);
}
