#include <string.h>

//#include <zephyr.h>
#include <device.h>
#include <net/buf.h>
#include <sys/crc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(stp_ota, CONFIG_STP_OTA_LOG_LEVEL);

#include "stp/opc_proto.h"
#include "stp/stp_core.h"
#include "stp/ota/ota_file.h"

#ifdef CONFIG_PROTOBUF
#include "stp/proto/ota.pb-c.h"
#endif


#define OTA_FILE_REQ  0x01
#define OTA_FILE_BKPT 0x02
#define OTA_FILE_DATA 0x03
#define OTA_FILE_CMP  0x04

static uint8_t ota_buffer[50*1024];
static struct ota_file ota_file = {
    .f_ops = &ota_ram_ops,
    .buffer = ota_buffer,
    .bsize = sizeof(ota_buffer)
};

static void ota_request_service(const void *buf, size_t size,
    struct net_buf *obuf)
{
    uint8_t resp[sizeof(struct opc_subhdr) + 10];
    struct opc_subhdr *hdr = (struct opc_subhdr *)resp;
    uint16_t len;

    ARG_UNUSED(size);   
#ifdef CONFIG_PROTOBUF
    Ota__FileRequest req = OTA__FILE_REQUEST__INIT;
    req.fmax_size = 2048;
    req.battery = 100;
    len = (uint16_t)ota__file_request__pack(&req, hdr->data);
#else
    struct ota_reqack *ack = (struct ota_reqack *)hdr->data;
    ack->maxsize = ltons(2048);
    ack->battery = 100;
#endif
    hdr->minor = OTA_FILE_REQ;
    hdr->len = ltons(OPC_DLEN(len)); 
    net_buf_add_mem(obuf, resp, sizeof(resp));
}
STP_SERVICE(ota, OPC_CLASS_OTA, OTA_FILE_REQ, ota_request_service);

static void ota_get_breakpoint_service(const void *buf, size_t size, 
    struct net_buf *obuf)
{
    struct ota_filebkpt context;
    uint8_t resp[sizeof(struct opc_subhdr) + 16];
    struct opc_subhdr *hdr = (struct opc_subhdr *)resp;
    uint16_t len = 0;

    ARG_UNUSED(size);
    if (ota_file_get_context(&ota_file, &context)) {
#ifdef CONFIG_PROTOBUF
        Ota__File file = OTA__FILE__INIT;
        file.maxno = context.maxno;
        file.no = context.no;
        file.field_case = OTA__FILE__FIELD_CRC;
        file.crc = context.crc;
        len = ota__file__pack(&file, hdr->data);
#else
        struct ota_filebkpt *ctx = (struct ota_filebkpt *)hdr->data;
        ctx->maxno = ltons(context.maxno);
        ctx->no = ltons(context.no);
        ctx->crc = ltonl(contextcrc);
        len = sizeof(*ctx);
#endif
        LOG_INF("File transmit context length: %d\n", len);
    }
    hdr->minor = OTA_FILE_BKPT;
    hdr->len = ltons(OPC_DLEN(len)); 
    net_buf_add_mem(obuf, resp, sizeof(*hdr)+len);
}
STP_SERVICE(ota, OPC_CLASS_OTA, OTA_FILE_BKPT, ota_get_breakpoint_service);
    
static void ota_data_receive_service(const void *buf, size_t size, 
    struct net_buf *obuf)
{
    uint8_t resp[sizeof(struct opc_subhdr) + 1];
    struct opc_subhdr *hdr = (struct opc_subhdr *)resp;
    uint8_t ret;

#ifdef CONFIG_PROTOBUF
    Ota__File *file = ota__file__unpack(NULL, OPC_LEN(size), buf);
    if (file && file->field_case == OTA__FILE__FIELD_DATA)
        ret = ota_file_receive(&ota_file, file, 0);
    else
        ret = 0x10;
    ota__file__free_unpacked(file, NULL);
#else
    ret = ota_file_receive(&ota_file, buf, OPC_LEN(size));
#endif
    hdr->minor = OTA_FILE_DATA;
    hdr->len = ltons(OPC_SLEN(1)); 
    hdr->data[0] = ret;
    net_buf_add_mem(obuf, resp, sizeof(resp));
}
STP_SERVICE(ota, OPC_CLASS_OTA, OTA_FILE_DATA, ota_data_receive_service);

static void ota_checksum_service(const void *buf, size_t size, 
    struct net_buf *obuf)
{
    uint8_t resp[sizeof(struct opc_subhdr) + 1];
    struct opc_subhdr *hdr = (struct opc_subhdr *)resp;
    uint32_t file_crc;
    uint32_t crc;

    hdr->minor = OTA_FILE_CMP;
    hdr->len = ltons(OPC_SLEN(1));  
#ifdef CONFIG_PROTOBUF
    Ota__FileCheck *file_chk = ota__file_check__unpack(NULL, OPC_LEN(size), buf);
    if (!file_chk) {
        hdr->data[0] = 0x02;
        goto _exit;
    }

    file_crc = file_chk->crc;
    ota__file_check__free_unpacked(file_chk, NULL);
#else
    if (OPC_LEN(size) != sizeof(uint32_t)) {
        hdr->data[0] = 0x01;
        goto _exit;
    }
    file_crc = btol(buf);
#endif
    crc = ota_file_checksum(&ota_file);
    if (file_crc != crc) {
        LOG_ERR("%s(): File checksum is error(expect: 0x%x, real: 0x%x)\n",
            __func__, file_crc, crc);
        hdr->data[0] = 0x02;
    } else {
        LOG_INF("File checksum is 0x%x\n", crc);
        hdr->data[0] = 0x00;
    }
_exit:
    net_buf_add_mem(obuf, resp, sizeof(resp));
}
STP_SERVICE(ota, OPC_CLASS_OTA, OTA_FILE_CMP, ota_checksum_service);
    
static void ota_fwinfo_upload_action(const void *buf, size_t size, 
    struct net_buf *obuf)
{
    
}
STP_SERVICE(ota, OPC_CLASS_OTA, 0x05, ota_fwinfo_upload_action);

