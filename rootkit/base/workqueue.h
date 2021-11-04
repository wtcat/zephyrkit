#ifndef BASE_WORKQUEUE_H_
#define BASE_WORKQUEUE_H_

#include <zephyr.h>
#include <init.h>

#ifdef __cplusplus
extern "C"{
#endif

#define WORKQUEUE_DEFINE(_name, _stksize, _prio) \
    struct k_work_q _name; \
    K_KERNEL_STACK_DEFINE(_name##__q_stack, _stksize);\
    static int _name##__work_q_init(const struct device *dev) \
    { \
        ARG_UNUSED(dev); \
        k_work_q_start(&_name, _name##__q_stack, \
                   K_KERNEL_STACK_SIZEOF(_name##__q_stack), _prio); \
        k_thread_name_set(&_name.thread, #_name); \
        return 0; \
    } \
    SYS_INIT(_name##__work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT)



#ifdef __cplusplus
}
#endif
#endif //BASE_WORKQUEUE_H_