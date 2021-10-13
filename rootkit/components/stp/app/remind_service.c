#include <string.h>

#include <device.h>
#include <net/buf.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(stp_app, CONFIG_STP_APP_LOG_LEVEL);

#include "stp/opc_proto.h"
#include "stp/stp_core.h"

#define OBSERVER_CLASS_DEFINE
#include "remind_service.h"


static void message_remind_service(const void *buf, size_t size,
    struct net_buf *obuf)
{
    uint8_t resp[sizeof(struct opc_subhdr) + 1];
    struct opc_subhdr *hdr = (struct opc_subhdr *)resp;
    Remind__Message *msg;

    hdr->minor = MESSAGE_REMIND;
    hdr->len = ltons(OPC_SLEN(1));
    msg = remind__message__unpack(NULL, OPC_LEN(size), buf);
    if (msg == NULL) {
        LOG_ERR("%s(): Receive text message failed\n", __func__);
        hdr->data[0] = 0x1;
        goto _resp;
    }
    
    /* Dump information */
    LOG_INF("Time:%d\n", msg->timestamp->time);
    LOG_INF("Type:%d\n", msg->type);
    if (msg->type == REMIND__MESSAGE__TYPE__TEXT) {
        LOG_INF("Name:%s\n", msg->user);
        LOG_INF("Phone:%s\n", msg->phone);
        LOG_INF("Text:%s\n", msg->chat);
    } else {
        LOG_INF("User:%s\n", msg->user);
        LOG_INF("Text:%s\n", msg->chat);
    }

    /* Notify */
    remind_notify(MESSAGE_REMIND, msg);

    /* Free buffer */
    remind__message__free_unpacked(msg, NULL);
    hdr->data[0] = 0x00;
_resp:
    net_buf_add_mem(obuf, resp, sizeof(resp));
}
STP_SERVICE(remind, OPC_CLASS_REMIND, MESSAGE_REMIND, message_remind_service);



