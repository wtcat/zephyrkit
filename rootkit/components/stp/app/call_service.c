#include <string.h>

#include <device.h>
#include <net/buf.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(stp_app, CONFIG_STP_APP_LOG_LEVEL);

#include "stp/opc_proto.h"
#include "stp/stp_core.h"

#define OBSERVER_CLASS_DEFINE
#include "call_service.h"


int call_sync_request(Call__CallState__State state)
{
    Call__CallState cs = CALL__CALL_STATE__INIT;
    stp_packet_declare(10) m;
    struct stp_bufv v[1];
    size_t size;

    cs.state = state;
    size = call__call_state__pack(&cs, m.hdr.data);
    opc_bufv_init(&v[0], &m.hdr, CALLING_STATE_UPDATE, size);
    return stp_sendmsgs(opc_chan_get(OPC_CLASS_CALL), v, 1); 
}

static void call_notice_service(const void *buf, size_t size,
    struct net_buf *obuf)
{
    uint8_t resp[sizeof(struct opc_subhdr) + 1];
    struct opc_subhdr *hdr = (struct opc_subhdr *)resp;
    Call__Notice *call;

    hdr->minor = CALLING_NOTICE;
    hdr->len = ltons(OPC_SLEN(1));
    call = call__notice__unpack(NULL, OPC_LEN(size), buf);
    if (call == NULL) {
        LOG_ERR("%s(): Receive calling message failed\n", __func__);
        hdr->data[0] = 0x2;
        goto _resp;
    }

    /* Dump information */
    LOG_INF("Time:%d\n", call->timestamp->time);
    LOG_INF("Name:%s\n", call->people);
    LOG_INF("Phone:%s\n", call->phone);
    
    /* Notify */
    call_notify(CALLING_NOTICE, call);

    /* Free buffer */
    call__notice__free_unpacked(call, NULL);
    hdr->data[0] = 0x00;
_resp:
    net_buf_add_mem(obuf, resp, sizeof(resp));
}
STP_SERVICE(call, OPC_CLASS_CALL, CALLING_NOTICE, call_notice_service);

static void call_update_service(const void *buf, size_t size,
    struct net_buf *obuf)
{
    uint8_t resp[sizeof(struct opc_subhdr) + 1];
    struct opc_subhdr *hdr = (struct opc_subhdr *)resp;
    Call__CallState *state;

    hdr->minor = CALLING_STATE_UPDATE;
    hdr->len = ltons(OPC_SLEN(1));
    state = call__call_state__unpack(NULL, OPC_LEN(size), buf);
    if (state == NULL) {
        LOG_ERR("%s(): Receive calling message failed\n", __func__);
        hdr->data[0] = 0x2;
        goto _resp;
    }

    /* Notify */
    call_notify(CALLING_STATE_UPDATE, state);

    /* Free buffer */
    call__call_state__free_unpacked(state, NULL);
    hdr->data[0] = 0x00;
_resp:
    net_buf_add_mem(obuf, resp, sizeof(resp));
}
STP_SERVICE(call, OPC_CLASS_CALL, CALLING_STATE_UPDATE, call_update_service);