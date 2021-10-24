/*
 * Copyright (C) 2001 OKTET Ltd., St.-Petersburg, Russia
 * Author: Victor V. Vengerov <vvv@oktet.ru>
 */
#include <errno.h>
#include "blkdev/blkdev.h"
#include "blkdev/bdbuf.h"

int k_blkdev_ioctl(struct k_disk_device *dd, uint32_t req, void *argp) {
    int rc = 0;
    switch (req) {
    case K_BLKIO_GETMEDIABLKSIZE:
        *(uint32_t *) argp = dd->media_block_size;
        break;
    case K_BLKIO_GETBLKSIZE:
        *(uint32_t *) argp = dd->block_size;
        break;
    case K_BLKIO_SETBLKSIZE:
        sc = k_bdbuf_set_block_size(dd, *(uint32_t *) argp, true);
        if (sc != RTEMS_SUCCESSFUL) {
            errno = EIO;
            rc = -1;
        }
        break;
    case K_BLKIO_GETSIZE:
        *(blkdev_bnum_t *) argp = dd->size;
        break;
    case K_BLKIO_SYNCDEV:
        sc = k_bdbuf_syncdev(dd);
        if (sc != RTEMS_SUCCESSFUL) {
            errno = EIO;
            rc = -1;
        }
        break;
    case K_BLKIO_GETDISKDEV:
        *(struct k_disk_device **) argp = dd;
        break;
    case K_BLKIO_PURGEDEV:
        k_bdbuf_purge_dev(dd);
        break;
    case K_BLKIO_GETDEVSTATS:
        k_bdbuf_get_device_stats(dd, (rtems_blkdev_stats *) argp);
        break;
    case K_BLKIO_RESETDEVSTATS:
        k_bdbuf_reset_device_stats(dd);
        break;
    default:
        errno = EINVAL;
        rc = -1;
        break;
    }
    return rc;
}
