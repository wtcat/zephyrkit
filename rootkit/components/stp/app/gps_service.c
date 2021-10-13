#include <string.h>

#include <device.h>
#include <net/buf.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(stp_app, CONFIG_STP_APP_LOG_LEVEL);

#include "stp/opc_proto.h"
#include "stp/stp_core.h"

#define OBSERVER_CLASS_DEFINE
#include "stp/app/gps_service.h"

#define GPS_START  0x01
#define GPS_NOTIFY 0x02
#define GPS_STOP   0x03


int remote_gps_start(uint32_t period)
{
    Gps__Request gps = GPS__REQUEST__INIT;
    stp_packet_declare(16) m;
    struct stp_bufv v[1];

    if (period > 0) {
        gps.mode = GPS__REQUEST__START_MODE__PERIOD;
        gps.has_period = true;
        gps.period = period;
    } else {
        gps.mode = GPS__REQUEST__START_MODE__ONESHOT;
    }
    size_t size = gps__request__pack(&gps, m.hdr.data);
    __ASSERT(size < sizeof(m) - sizeof(*hdr), "");
    opc_bufv_init(&v[0], &m.hdr, GPS_START, size);  
    return stp_sendmsgs(opc_chan_get(OPC_CLASS_GPS), v, 1); 
}

int remote_gps_stop(void)
{
    stp_packet_declare(1) p;
    struct stp_bufv v[1];
    opc_bufv_init(&v[0], &p.hdr, GPS_STOP, 0);
    return stp_sendmsgs(opc_chan_get(OPC_CLASS_CAMERA), v, 1);
}

static void gps_common_service(const void *buf, size_t size,
    uint32_t cmd, const char *caller)
{
    if (size == 1) {
        int result = *(const uint8_t *)buf;
        gps_notify(cmd, &result);
    } else {
        LOG_ERR("%s(): Camera ack packet error\n", caller);
    }
}

static void gps_start_notify(const void *buf, size_t size,
    struct net_buf *obuf)
{
    ARG_UNUSED(obuf);
    gps_common_service(buf, OPC_LEN(size), GPS_START, __func__);
}

static void gps_stop_notify(const void *buf, size_t size,
    struct net_buf *obuf)
{
    ARG_UNUSED(obuf);
    gps_common_service(buf, OPC_LEN(size), GPS_STOP, __func__);
}

static void gps_receive_service(const void *buf, size_t size,
    struct net_buf *obuf)
{
    Gps__Position *gps;
    gps = gps__position__unpack(NULL, OPC_LEN(size), buf);
    if (gps == NULL) {
        LOG_ERR("%s(): GPS data receive failed\n", __func__);
        return;
    }
    gps_notify(GPS_NOTIFY, gps);
    gps__position__free_unpacked(gps, NULL);
    //Don't ack sender
}

STP_SERVICE(gps, OPC_CLASS_GPS, GPS_START, gps_start_notify);
STP_SERVICE(gps, OPC_CLASS_GPS, GPS_NOTIFY, gps_receive_service);
STP_SERVICE(gps, OPC_CLASS_GPS, GPS_STOP, gps_stop_notify);