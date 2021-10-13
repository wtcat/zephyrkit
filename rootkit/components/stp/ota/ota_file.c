#include <zephyr.h>
#include <device.h>
#include <sys/crc.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(stp_ota);

#include "stp/stp_core.h"
#include "stp/opc_proto.h"
#include "stp/ota/ota_file.h"

#ifdef CONFIG_PROTOBUF
#include "stp/proto/ota.pb-c.h"
#endif

struct ota_file_header {
    const char fname[32];
    uint32_t magic;
    size_t size;
    uint32_t offset;
    uint32_t crc;
};


static int __ota_file_write(struct ota_file *ota, const void *buffer,
    size_t size)
{
    size_t remain_room = ota->bsize - ota->blen;
    size_t copy_size = MIN(remain_room, size);
    size_t remain;
    int ret;

    if (size > ota->bsize) {
        LOG_ERR("%s(): write buffer size is too large\n", __func__);
        ret = -EINVAL;
        goto _exit;
    }
    
    if (copy_size == 0) {
        LOG_ERR("%s(): write buffer size is too small\n", __func__);
        ret = 0;
        goto _exit;
    }

    memcpy(ota->buffer + ota->blen, buffer, copy_size);
    ota->blen += copy_size;
    if (ota->blen < ota->bsize) {
        ret = 0;
        goto _add_size;
    }

    LOG_INF("%s(): Flush data(%d bytes) to disk\n", __func__, ota->blen);
    ret = ota_file_write(ota, ota->buffer, ota->blen);
    if (ret < 0) {
        LOG_ERR("%s(): write file error: %d\n", __func__, ret);
        ota->blen -= copy_size;
        goto _exit;
    }
    
    remain = size - copy_size;
    if (remain > 0)
        memcpy(ota->buffer, (const char *)buffer + copy_size, remain);
    ota->blen = remain;  
    ret = 0;
_add_size:
    ota->offset += size;
_exit:
    return ret;
}

static int ota_file_flush(struct ota_file *ota)
{
    int ret;
    if (!ota->blen)
        return 0;
  
    LOG_INF("Flush buffer size is: %d\n", ota->blen);
    ret = ota_file_write(ota, ota->buffer, ota->blen);
    if (ret <= 0) {
        LOG_ERR("%s(): OTA write file error %d\n", __func__, ret);
        return ret;
    }
    
    ota->blen = 0;
    return 0;
}

static int ota_file_reinit(struct ota_file *ota, 
    size_t packet_size, uint16_t packets)
{
    int ret = ota_file_open(ota);
    if (!ret) {
        ota->packet_size = packet_size;
        ota->packets = packets;
        ota->packet_no = 0;
        ota->offset = 0;
        ota->checksum = 0;
        ota->blen = 0;
    }
    return ret;
}

bool ota_file_get_context(struct ota_file *ota, 
    struct ota_filebkpt *ctx)
{
    if (ota->packet_no == 0 || 
        ota->packet_no >= ota->packets)
        return false;
    
    if (ota->packet_size == 0)
        return false;
        
    ctx->maxno = ota->packets;
    ctx->no = ota->packet_no;
    ctx->crc = ota->checksum;
    return true;
}

uint32_t ota_file_checksum(struct ota_file *ota)
{
    size_t fsize = ota->offset;
    uint32_t crc = 0;
    int ret;

    ret = ota_file_open(ota);
    if (ret) {
        LOG_ERR("%s(): OTA open file failed: %d\n", __func__, ret);
        goto _exit;
    }

    ret = ota_file_seek(ota, 0, OTA_SEEK_SET);
    if (ret) {
        LOG_ERR("%s(): File seek failed\n", __func__);
        ota_file_close(ota);
        goto _exit;
    }
    
    LOG_INF("File size is %d\n", fsize);
    while (fsize > 0) {
        size_t bytes = MIN(ota->bsize, fsize);
        ret = ota_file_read(ota, ota->buffer, bytes);
        if (ret < 0) {
            LOG_ERR("%s(): Read file failed\n", __func__);
            goto _exit;
        }
        crc = crc32_ieee_update(crc, ota->buffer, ret);
        fsize -= ret;
    }
    
    ota_file_close(ota);
    return crc;
_exit:
    return 0;
}

uint8_t ota_file_receive(struct ota_file *ota, const void *buf, 
    size_t size)
{
    const uint8_t *data;
    uint16_t total;
    uint16_t seqno;
    int ret;
    
#ifdef CONFIG_PROTOBUF
    Ota__File *file = (Ota__File *)buf;
    total = file->maxno;
    seqno = file->no;
    data = file->data.data;
    size = file->data.len;
#else
    const struct opc_filehdr *hdr;
    if (size <= sizeof(*hdr)) {
        LOG_ERR("%s(): Packet size is to small\n", __func__);
        return 0x01;
    }
    size -= sizeof(*hdr);
    hdr = buf;
    total = ntols(hdr->maxno);
    seqno = ntols(hdr->seqno);
    data = hdr->data;
#endif

    if (seqno == 1) {
        /* First data packet */
        ret = ota_file_reinit(ota, size, total);
        if (ret) {
            LOG_ERR("%s(): OTA initialize failed\n", __func__);
            return 0xa;
        }
    } else if (size != ota->packet_size &&
                seqno != total) {
        LOG_ERR("%s(): Packet size is error(get: %d, expect: %d)\n", 
            __func__, size, ota->packet_size);
        return 0x02;
    }
    
    if (seqno != ota->packet_no + 1 ||
        total != ota->packets ||
        seqno > ota->packets) {
        LOG_ERR("%s(): Packet sequence number(%d) error\n", 
            __func__, seqno);
        return 0x03;
    }

    //For test
    if (ota->blen != ota->packet_no * ota->packet_size) {
        LOG_ERR("File length error(blen: %d, psize: %d)\n", 
            ota->blen,
            ota->packet_no * ota->packet_size);
    }
    
    ret = __ota_file_write(ota, data, size);
    if (ret) {
        LOG_ERR("%s(): Write file failed\n", __func__);
        return 0x04;
    }
   
    ota->packet_no = seqno;
    ota->checksum = crc32_ieee_update(ota->checksum, data, size);
    //LOG_INF("Packet(maxno: %d, seqno:%d, div-size: %d, size: %d, sum-size: %d)\n", 
    //    total, seqno, ota->packet_size, size, ota->offset);

    /* Transmit completed */
    if (total == seqno) {
        LOG_INF("%s(): Sync data to disk\n", __func__);
        if (ota_file_flush(ota)) {
            LOG_ERR("%s(): Flush data failed\n", __func__);
            return 0x05;
        }

        ota_file_close(ota);
        LOG_INF("File information:\n" "Size: %d\n" "CRC: 0x%x\n",
                ota->offset, ota->checksum);
    }
    
    return 0;
}

