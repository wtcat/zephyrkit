#include <drivers/flash.h>
#include "blkdev/blkdev.h"

static int flash_disk_read(struct k_blkdev_driver *drv, 
    struct k_blkdev_request *req) {
    uint32_t start = drv->p->start;
    uint32_t blksize = drv->p->blksize;
    struct k_blkdev_sg_buffer *sg;
    int ret;
    for (int i = 0, sg = req->bufs; i < req->bufnum; i++, sg++) {
        off_t offset = start + sg->block * blksize;
        ret = flash_read(drv->dev, offset, sg->buffer, sg->length);
        if (ret)
            break;
    }
    return ret;
}

static int flash_disk_write(struct k_blkdev_driver *drv, 
    struct k_blkdev_request *req) {
    uint32_t start = drv->p->start;
    uint32_t blksize = drv->p->blksize;
    struct k_blkdev_sg_buffer *sg;
    int ret;
    flash_write_protection_set(drv->dev, false);
    for (int i = 0, sg = req->bufs; i < req->bufnum; i++, sg++) {
        off_t offset = start + sg->block * blksize;
        ret = flash_erase(drv->dev, offset, blksize);
        if (ret)
            break;
        ret = flash_write(drv->dev, offset, sg->buffer, sg->length);
        if (ret)
            break;
    }
_exit:
    flash_write_protection_set(drv->dev, true);
    k_blkdev_request_done(req, ret);
    return ret;
}

int flash_disk_ioctl(struct k_disk_device *dd, uint32_t req, void *arg) {
    struct k_blkdev_driver *d = k_disk_get_driver_data(dd);
    switch (req) {
    case K_BLKIO_REQUEST: {
        struct k_blkdev_request *r = arg;
        switch (r->req) {
        case K_BLKDEV_REQ_READ:
            return flash_disk_read(d, r);
        case K_BLKDEV_REQ_WRITE:
            return flash_disk_write(d, r);
        default:
            return -EINVAL;
        }
    }
    default:
        break;
    }
    return 
}