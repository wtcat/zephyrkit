#include <string.h>

#include <device.h>
#include <net/buf.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(stp_app, CONFIG_STP_APP_LOG_LEVEL);

#include "stp/opc_proto.h"
#include "stp/stp_core.h"

#define OBSERVER_CLASS_DEFINE
#include "stp/app/camera_service.h"


int remote_camera_request(uint32_t req)
{
    stp_packet_declare(1) p;
    struct stp_bufv v[1];
    opc_bufv_init(&v[0], &p.hdr, req, 0);
    return stp_sendmsgs(opc_chan_get(OPC_CLASS_CAMERA), v, 1);
}

static void camera_service(const void *buf, size_t size,
    uint32_t cmd, const char *caller)
{
    if (size == 1) {
        int result = *(const uint8_t *)buf;
        camera_notify(cmd, &result);
    } else {
        LOG_ERR("%s(): Camera ack packet error\n", caller);
    }
}

static void camera_open_notify(const void *buf, size_t size,
    struct net_buf *obuf)
{
    ARG_UNUSED(obuf);
    camera_service(buf, OPC_LEN(size), CAMERA_REQ_OPEN, __func__);
}

static void camera_shot_notify(const void *buf, size_t size,
    struct net_buf *obuf)
{
    ARG_UNUSED(obuf);
    camera_service(buf, OPC_LEN(size), CAMERA_REQ_SHOT, __func__);
}

static void camera_close_notify(const void *buf, size_t size,
    struct net_buf *obuf)
{
    ARG_UNUSED(obuf);
    camera_service(buf, OPC_LEN(size), CAMERA_REQ_CLOSE, __func__);
}

STP_SERVICE(camera, OPC_CLASS_CAMERA, CAMERA_REQ_OPEN, camera_open_notify);
STP_SERVICE(camera, OPC_CLASS_CAMERA, CAMERA_REQ_SHOT, camera_shot_notify);
STP_SERVICE(camera, OPC_CLASS_CAMERA, CAMERA_REQ_CLOSE, camera_close_notify);
