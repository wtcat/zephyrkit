#ifndef STP_APP_CAMERA_SERVICE_H_
#define STP_APP_CAMERA_SERVICE_H_

#include "base/observer_class.h"

#ifdef __cplusplus
extern "C"{
#endif

/*
 * method:
 *  int camera_notify(unsigned long action, void *ptr)
 *  int camera_add_observer(struct observer_base *obs)
 *  int camera_remove_observer(struct observer_base *obs)
 */
OBSERVER_CLASS_DECLARE(camera)

/*
 * Minor command list
 */
#define CAMERA_REQ_OPEN  0x01
#define CAMERA_REQ_SHOT  0x02
#define CAMERA_REQ_CLOSE 0x03

int remote_camera_request(uint32_t req);

#ifdef __cplusplus
}
#endif
#endif /* STP_APP_CAMERA_SERVICE_H_ */
