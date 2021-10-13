#include "data_service_custom.h"
#include "stdint.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(guix_data_service);

extern struct guix_driver *guix_get_drv_list(void);
#define FOREACH_ITEM(_iter, _head) for (_iter = (_head); _iter != NULL; _iter = _iter->next)

void data_service_event_submit(DATA_SERVICE_EVT_TYPE evt_type, void *data, uint8_t data_size)
{
	//长度超过4的消息。
	// 1） 发送者存储消息体内容，UI收到消息后直接通过api的方式从发送者获取消息内容
	// 2） 使用data存储消息体的指针.
	if (data_size > 4) {
		LOG_ERR("data_size too long!");
	}

	GX_EVENT event = {
		.gx_event_sender = 0,
		.gx_event_target = 0,
		.gx_event_display_handle = 0,
	};

	event.gx_event_type = evt_type;
	struct guix_driver *drv;
	struct guix_driver *guix_driver_list = guix_get_drv_list();
	int8_t *tmp = data;
	if (NULL != guix_driver_list) {
		FOREACH_ITEM(drv, guix_driver_list)
		{
			event.gx_event_display_handle = (ULONG)drv;
			for (uint8_t i = 0; i < data_size; i++) {
				event.gx_event_payload.gx_event_chardata[i] = tmp[i];
			}
			gx_system_event_send(&event);
		}
	}
}