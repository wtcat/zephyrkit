#ifndef APPLE_AMS_H_
#define APPLE_AMS_H_

#include <bluetooth/services/ams_client.h>

#ifdef __cplusplus
extern "C"{
#endif

int ams_init(void);
int ams_remote_command_send(enum bt_ams_remote_command_id cmd,
	bt_ams_write_cb done);

#ifdef __cplusplus
}
#endif
#endif /* APPLE_AMS_H_ */
