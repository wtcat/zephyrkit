#ifndef STP_APP_MUSIC_SERVICE_H_
#define STP_APP_MUSIC_SERVICE_H_

#include "base/observer_class.h"
#ifdef CONFIG_PROTOBUF
#include "stp/proto/gps.pb-c.h"
#endif

#ifdef __cplusplus
extern "C"{
#endif

/*
 * method:
 *  int gps_notify(unsigned long action, void *ptr)
 *  int gps_add_observer(struct observer_base *obs)
 *  int gps_remove_observer(struct observer_base *obs)
 */
OBSERVER_CLASS_DECLARE(gps)


/*
 * GSP start with oneshot mode when period is 0.
 */ 
int remote_gps_start(uint32_t period);
int remote_gps_stop(void);

#ifdef __cplusplus
}
#endif
#endif /* STP_APP_MUSIC_SERVICE_H_ */
