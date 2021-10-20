#ifndef IPC_RPMSG_ENDPOINT_H_
#define IPC_RPMSG_ENDPOINT_H_

#include <init.h>
#include <ipc/rpmsg_service.h>
#include <logging/log.h>

#ifdef __cplusplus
extern "C"{
#endif

#define RPMSG_ENDPOINT_DEFINE(name, callback, idptr) \
    static int name##_##register_endpoint(const struct device *arg) {          \
        int ret;                                                               \
        ret = rpmsg_service_register_endpoint(#name, callback);                \
        if (ret < 0) {                                                         \
            LOG_ERR("[%s]Registering endpoint failed with %d", __func__, ret); \
            return ret;                                                        \
        }                                                                      \
        *(idptr) = ret;                                                        \
        return ret;                                                            \
    }                                                                          \
    SYS_INIT(name##_##register_endpoint, POST_KERNEL,  \
        CONFIG_RPMSG_SERVICE_EP_REG_PRIORITY)

#ifdef __cplusplus
}
#endif
#endif /* IPC_RPMSG_ENDPOINT_H_ */