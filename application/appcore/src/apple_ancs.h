#ifndef APPLE_ANCS_H_
#define APPLE_ANCS_H_

#include <bluetooth/services/ancs_client.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*ancs_notification_handler_t)(struct bt_ancs_client *ancs, 
    const struct bt_ancs_evt_notif *notif);
typedef void (*ancs_data_handler_t)(struct bt_ancs_client *ancs,
	const struct bt_ancs_attr_response *rsp);


int apple_ancs_init(ancs_notification_handler_t notif_cb,
	ancs_data_handler_t data_cb);

#ifdef __cplusplus
}
#endif
#endif /* APPLE_ANCS_H_ */
