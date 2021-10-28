/*
 * Copyright (C) 2021
 * Author: wtcat
 */
#include <string.h>
#include <device.h>

#include "blkdev/blkdev.h"

static int ram_disk_read(struct k_blkdev_driver *drv, 
    struct k_blkdev_request *req) {
    uint32_t start = drv->p->start;
    uint32_t blksize = drv->p->blksize;
    struct k_blkdev_sg_buffer *sg;
    for (int i = 0, sg = req->bufs; i < req->bufnum; i++, sg++) {
        uintptr_t offset = start + sg->block * blksize;
        memcpy(sg->buffer, (void *)offset, sg->length);
    }
    k_blkdev_request_done(req, 0);
    return 0;
}

static int ram_disk_write(struct k_blkdev_driver *drv, 
    struct k_blkdev_request *req) {
    uint32_t start = drv->p->start;
    uint32_t blksize = drv->p->blksize;
    struct k_blkdev_sg_buffer *sg;
    int ret;

    for (int i = 0, sg = req->bufs; i < req->bufnum; i++, sg++) {
        uintptr_t offset = start + sg->block * blksize;
        memcpy((void *)offset, sg->buffer, sg->length);
    }
    k_blkdev_request_done(req, 0);
    return 0;
}

int ram_disk_ioctl(struct k_disk_device *dd, uint32_t req, void *arg) {
    struct k_blkdev_driver *d = k_disk_get_driver_data(dd);
    switch (req) {
    case K_BLKIO_REQUEST: {
        struct k_blkdev_request *r = arg;
        switch (r->req) {
        case K_BLKDEV_REQ_READ:
            return ram_disk_read(d, r);
        case K_BLKDEV_REQ_WRITE:
            return ram_disk_write(d, r);
        default:
            return -EINVAL;
        }
    }
    default:
        return k_blkdev_default_ioctl(dd, r, arg);
    }
}

static int ram_device_init(const struct device *dev __unused) {
    return 0;
}
DEVICE_DEFINE(ramdisk, "ram", ram_device_init, NULL,
		      NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);