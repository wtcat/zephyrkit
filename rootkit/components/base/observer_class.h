#ifndef BASE_OBSERVER_CLASS_H_
#define BASE_OBSERVER_CLASS_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "base/observer.h"

#ifdef OBSERVER_CLASS_DEFINE
#define _OBSERVER_CLASS
#else
#define _OBSERVER_CLASS extern
#endif

/*
 * method:
 *  int _class_name##_notify(unsigned long action, void *ptr)
 *  int _class_name##_add_observer(struct observer_base *obs)
 *  int _class_name##_remove_observer(struct observer_base *obs)
 */
#define _OBSERVER_NAME(_class_name) _##_class_name##__observer_list_head
#define OBSERVER_CLASS_DECLARE(_class_name) \
    _OBSERVER_CLASS struct observer_base *_OBSERVER_NAME(_class_name); \
    static inline int _class_name##_notify(unsigned long value, void *ptr) { \
        return observer_notify(&_OBSERVER_NAME(_class_name), value, ptr); \
    } \
    static inline int _class_name##_add_observer(struct observer_base *obs) { \
        return observer_cond_register(&_OBSERVER_NAME(_class_name), obs); \
    } \
    static inline int _class_name##_remove_observer(struct observer_base *obs) { \
        return observer_unregister(&_OBSERVER_NAME(_class_name), obs); \
    }

#ifdef __cplusplus
}
#endif
#endif /* STP_PROTO_OBS_H_ */

