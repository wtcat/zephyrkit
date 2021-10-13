#ifndef __SETTING_SERVICE_CUSTOM_H__
#define __SETTING_SERVICE_CUSTOM_H__
#include "stdint.h"
#include "stdbool.h"

typedef enum {
	SETTING_CUSTOM_DATA_ID_BRIGHTNESS = 0,
	SETTING_CUSTOM_DATA_ID_BRIGHT_TIME,
	SETTING_CUSTOM_DATA_ID_WRIST_RAISE,
	SETTING_CUSTOM_DATA_ID_NIGHT_MODE,
	SETTING_CUSTOM_DATA_ID_NIGHT_MODE_CUSTOM,
	SETTING_CUSTOM_DATA_ID_APP_VIEW_TYPE,
	SETTING_CUSTOM_DATA_ID_MAX,
} SETTING_CUSTOM_DATA_ID;

#define SETTING_BRIGHTNESS_DEFAULT_VALUE 125

void setting_service_init(void);
void setting_service_set(SETTING_CUSTOM_DATA_ID id, void *data, uint8_t data_size, bool sync_flag);
void setting_service_get(SETTING_CUSTOM_DATA_ID id, void *data, uint8_t data_size);
void setting_service_store_sync(void);

#endif