#ifndef STP_APP_CALL_SERVICE_H_
#define STP_APP_CALL_SERVICE_H_

#include "base/observer_class.h"
#ifdef CONFIG_PROTOBUF
#include "stp/proto/call.pb-c.h"
#endif

#ifdef __cplusplus
extern "C"{
#endif

/*
 * CALL minor number
 */
#define CALLING_NOTICE       0x01
#define CALLING_STATE_UPDATE 0x02


/*
 * OBSERVER_CLASS_DECLARE(xxxxx)
 * method:
 *  int xxxxx_notify(unsigned long action, void *ptr)
 *  int xxxxx_add_observer(struct observer_base *obs)
 *  int xxxxx_remove_observer(struct observer_base *obs)
 */
OBSERVER_CLASS_DECLARE(call)
    
/*
 * Sync calling-state local state to remote
 */ 
int call_sync_request(Call__CallState__State state);

#ifdef __cplusplus
}
#endif
#endif /* STP_APP_CALL_SERVICE_H_ */