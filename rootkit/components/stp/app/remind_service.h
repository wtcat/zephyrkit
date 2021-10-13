#ifndef STP_APP_REMIND_SERVICE_H_
#define STP_APP_REMIND_SERVICE_H_

#include "base/observer_class.h"
#ifdef CONFIG_PROTOBUF
#include "stp/proto/remind.pb-c.h"
#endif

#ifdef __cplusplus
extern "C"{
#endif

/*
 * Remind minor number
 */
#define MESSAGE_REMIND 0x01



/*
 * method:
 *  int remind_notify(unsigned long action, void *ptr)
 *  int remind_add_observer(struct observer_base *obs)
 *  int remind_remove_observer(struct observer_base *obs)
 */
OBSERVER_CLASS_DECLARE(remind)
    

#ifdef __cplusplus
}
#endif
#endif /* STP_APP_REMIND_SERVICE_H_ */
