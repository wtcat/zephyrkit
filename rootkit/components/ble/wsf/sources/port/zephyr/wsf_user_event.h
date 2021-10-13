#include <init.h>

#include "wsf_os.h"


#define WSF_EVENT_DEFINE(_init, _callback)\
    static int __wsf_event_handler_##_init(const struct device *dev) \
    { \
        ARG_UNUSED(dev); \
        wsfHandlerId_t hid; \
        hid = WsfOsSetNextHandler(_callback); \
        (*_init)(hid); \
        return 0; \
    } \
        \
    SYS_INIT(__wsf_event_handler_##_init, APPLICATION, \
        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT+11)
 