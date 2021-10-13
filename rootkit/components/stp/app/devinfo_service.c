#include <string.h>

//#include <zephyr.h>
#include <device.h>
#include <net/buf.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(stp_app, CONFIG_STP_APP_LOG_LEVEL);

#include "stp/opc_proto.h"
#include "stp/stp_core.h"
#ifdef CONFIG_PROTOBUF
#include "stp/proto/devinfo.pb-c.h"
#endif

#include "base/version.h"
#include "base/mac_addr.h"


#define DEV_INFO_REQ  0x01
#define DEV_INFO_BAT  0x02


static void devinfo_request_service(const void *buf, size_t size,
    struct net_buf *obuf)
{
    uint8_t resp[sizeof(struct opc_subhdr) + 64];
    Info__Device dinfo = INFO__DEVICE__INIT;
    struct opc_subhdr *hdr ;
    uint8_t mac[6];
    uint16_t len;

    ARG_UNUSED(size);
    dinfo.has_type = true;
    dinfo.type = INFO__DEVICE__TYPE__WATCH;
    dinfo.has_model = true;
    dinfo.model = INFO__DEVICE__MODEL__DM_APOLLO3P;
    dinfo.has_firmware_ver = true;
    dinfo.firmware_ver = get_version_number(FIRMWARE_VER);
    dinfo.has_hardware_ver = true;
    dinfo.hardware_ver = get_version_number(HARDWARE_VER);
    if (get_mac_address(mac) > 0) {
        dinfo.has_mac = true;
        dinfo.mac.data = mac;
        dinfo.mac.len = 6;
    }
    
    hdr = (struct opc_subhdr *)resp;
    len = info__device__pack(&dinfo, hdr->data);
    hdr->minor = DEV_INFO_REQ;
    hdr->len = ltons(OPC_DLEN(len));
    net_buf_add_mem(obuf, resp, sizeof(struct opc_subhdr) + len);
}
STP_SERVICE(info, OPC_CLASS_INFO, DEV_INFO_REQ, devinfo_request_service);

static void devinfo_battery_service(const void *buf, size_t size,
    struct net_buf *obuf)
{
    uint8_t resp[sizeof(struct opc_subhdr) + 10];
    Info__Battery batinfo = INFO__BATTERY__INIT;
    struct opc_subhdr *hdr ;
    uint16_t len;

    ARG_UNUSED(size);
    batinfo.level = 50; //TODO: Read battery power level
    hdr = (struct opc_subhdr *)resp;
    len = info__battery__pack(&batinfo, hdr->data);
    hdr->minor = DEV_INFO_BAT;
    hdr->len = ltons(OPC_DLEN(len));
    net_buf_add_mem(obuf, resp, sizeof(struct opc_subhdr) + len);
}
STP_SERVICE(info, OPC_CLASS_INFO, DEV_INFO_BAT, devinfo_battery_service);


