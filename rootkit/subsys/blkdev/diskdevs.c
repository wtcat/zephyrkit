/**
 * @file
 *
 * @ingroup rtems_disk Block Device Disk Management
 *
 * @brief Block Device Disk Management Initialize
 */

/*
 * Copyright (c) 2012 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Obere Lagerstr. 30
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */
#include <string.h>

#include "blkdev/blkdev.h"
#include "blkdev/bdbuf.h"

int _k_disk_init(struct k_disk_device *dd, uint32_t block_size, 
  blkdev_bnum_t block_count, blkdev_ioctrl_fn handler, void *driver_data) {
  dd = memset(dd, 0, sizeof(*dd));
  dd->phys_dev = dd;
  dd->start = 0;
  dd->size = block_count;
  dd->media_block_size = block_size;
  dd->ioctl = handler;
  dd->driver_data = driver_data;
  dd->read_ahead.trigger = K_DISK_READ_AHEAD_NO_TRIGGER;
  if (block_count > 0) {
    if ((*handler)(dd, K_BLKIO_CAPABILITIES, &dd->capabilities))
      dd->capabilities = 0;
    return k_bdbuf_set_block_size(dd, block_size, false);
  }
  return -EINVAL;
}

// int k_disk_init_log(struct k_disk_device *dd, struct k_disk_device *phys_dd,
//   blkdev_bnum_t block_begin, blkdev_bnum_t block_count) {
//   dd = memset(dd, 0, sizeof(*dd));
//   dd->phys_dev = phys_dd;
//   dd->start = block_begin;
//   dd->size = block_count;
//   dd->media_block_size = phys_dd->media_block_size;
//   dd->ioctl = phys_dd->ioctl;
//   dd->driver_data = phys_dd->driver_data;
//   dd->read_ahead.trigger = K_DISK_READ_AHEAD_NO_TRIGGER;
//   if (phys_dd->phys_dev == phys_dd) {
//     blkdev_bnum_t phys_block_count = phys_dd->size;
//     if (block_begin < phys_block_count &&
//         block_count > 0 &&
//         block_count <= phys_block_count - block_begin) {
//       return k_bdbuf_set_block_size(dd, phys_dd->media_block_size, false);
//     }
//     return -EINVAL;
//   }
//   return -EINVAL;
// }
