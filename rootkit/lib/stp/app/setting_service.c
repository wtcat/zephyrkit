#include <string.h>

#include <device.h>
#include <net/buf.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(stp_app, CONFIG_STP_APP_LOG_LEVEL);

#include "stp/opc_proto.h"
#include "stp/stp_core.h"

#define OBSERVER_CLASS_DEFINE
#include "setting_service.h"


static void time_update_service(const void *buf, size_t size,
    struct net_buf *obuf)
{
    uint8_t resp[sizeof(struct opc_subhdr) + 1];
    struct opc_subhdr *hdr = (struct opc_subhdr *)resp;
    Setting__Time *time;

    hdr->minor = SETTING_UPDATE_TIME;
    hdr->len = ltons(OPC_SLEN(1));    
    time = setting__time__unpack(NULL, OPC_LEN(size), buf);
    if (time == NULL) {
        LOG_ERR("%s(): Unpack data failed\n", __func__);
        hdr->data[0] = 0x1;
        goto _resp;
    }

    /* Notify */
    setting_notify(SETTING_UPDATE_TIME, time);

    /* Free buffer */
    setting__time__free_unpacked(time, NULL);
    hdr->data[0] = 0x00;
_resp:
    net_buf_add_mem(obuf, resp, sizeof(resp));
}
STP_SERVICE(setting, OPC_CLASS_SETTING, SETTING_UPDATE_TIME, time_update_service);

static void alarm_set_service(const void *buf, size_t size,
    struct net_buf *obuf)
{
    uint8_t resp[sizeof(struct opc_subhdr) + 1];
    struct opc_subhdr *hdr = (struct opc_subhdr *)resp;
    Setting__Alarm *alarm;

    hdr->minor = SETTING_SET_ALARM;
    hdr->len = ltons(OPC_SLEN(1));
    alarm = setting__alarm__unpack(NULL, OPC_LEN(size), buf);
    if (alarm == NULL) {
        LOG_ERR("%s(): Unpack data failed\n", __func__);
        hdr->data[0] = 0x1;
        goto _resp;
    }

    /* Notify */
    setting_notify(SETTING_SET_ALARM, alarm);

    /* Free buffer */
    setting__alarm__free_unpacked(alarm, NULL);
    hdr->data[0] = 0x00;
_resp:
    net_buf_add_mem(obuf, resp, sizeof(resp));
}
STP_SERVICE(setting, OPC_CLASS_SETTING, SETTING_SET_ALARM, alarm_set_service);
