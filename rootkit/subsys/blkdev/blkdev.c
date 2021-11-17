/*
 * Copyright (c) 2012, 2018 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Dornierstr. 4
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */
/*
 * Copyright (C) 2021
 * Author: wtcat
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <init.h>

#include "blkdev/blkdev.h"

#include <storage/flash_map.h>

static K_MUTEX_DEFINE(lock)
static struct k_blkdev *blkdev_list;

struct k_blkdev *k_blkdev_get(int partition) {
  struct k_blkdev *ctx;
  k_mutex_lock(&lock, K_FOREVER);
  for (ctx = blkdev_list; ctx != NULL; ctx = ctx->next) {
    const struct k_blkdev_partition *pt = ctx->drv.p;
    if (pt->fa_id == partition) {
      atomic_add(&ctx->refcnt, 1);
      break;
    }
  }
  k_mutex_unlock(&lock);
  return ctx;
}

int k_blkdev_put(struct k_blkdev *ctx) {
  if (ctx == NULL)
    return -EINVAL;
  
  atomic_sub(&ctx->refcnt, 1);
  return 0;
}

ssize_t k_blkdev_read(struct k_blkdev *ctx, void *buffer,
  size_t count, off_t offset) {
  struct k_disk_device *dd = &ctx->dd;
  ssize_t remaining = (ssize_t)count;
  ssize_t block_size = (ssize_t)k_disk_get_block_size(dd);
  blkdev_bnum_t block = (blkdev_bnum_t)(offset / block_size);
  ssize_t block_offset = (ssize_t)(offset % block_size);
  char *dst = buffer;
  int rv;

  while (remaining > 0) {
    struct k_bdbuf_buffer *bd;
    int ret = k_bdbuf_read(dd, block, &bd);
    if (ret == 0) {
      ssize_t copy = block_size - block_offset;
      if (copy > remaining)
        copy = remaining;
      memcpy(dst, (char *)bd->buffer + block_offset, (size_t)copy);
      ret = k_bdbuf_release(bd);
      if (ret == 0) {
        block_offset = 0;
        remaining -= copy;
        dst += copy;
        ++block;
      } else {
        remaining = -1;
      }
    } else {
      remaining = -1;
    }
  }
  if (remaining >= 0) {
    rv = (ssize_t)count;
  } else {
    errno = EIO;
    rv = -1;
  }
  return rv;
}

ssize_t k_blkdev_write(struct k_blkdev *ctx, const void *buffer,
  size_t count, off_t offset) {
  struct k_disk_device *dd = &ctx->dd;
  ssize_t remaining = (ssize_t)count;
  ssize_t block_size = (ssize_t)k_disk_get_block_size(dd);
  blkdev_bnum_t block = (blkdev_bnum_t)(offset / block_size);
  ssize_t block_offset = (ssize_t)(offset % block_size);
  const char *src = buffer;
  int rv;

  while (remaining > 0) {
    struct k_bdbuf_buffer *bd;
    int ret;
    if (block_offset == 0 && remaining >= block_size) 
       ret = k_bdbuf_get(dd, block, &bd);
    else
       ret = k_bdbuf_read(dd, block, &bd);
    if (ret == 0) {
      ssize_t copy = block_size - block_offset;
      if (copy > remaining)
        copy = remaining;
      memcpy((char *)bd->buffer + block_offset, src, (size_t)copy);
      ret = k_bdbuf_release_modified(bd);
      if (ret == 0) {
        block_offset = 0;
        remaining -= copy;
        src += copy;
        ++block;
      } else {
        remaining = -1;
      }
    } else {
      remaining = -1;
    }
  }
  if (remaining >= 0) {
    rv = (ssize_t)count;
  } else {
    errno = EIO;
    rv = -1;
  }
  return rv;
}

int k_blkdev_ioctl(struct k_blkdev *ctx, ioctl_command_t request,
   void *buffer) {
  int ret = 0;
  if (request != K_BLKIO_REQUEST) {
    struct k_disk_device *dd = &ctx->dd;
    ret = (*dd->ioctl)(dd, request, buffer);
  } else {
    errno = EINVAL;
    ret = -1;
  }
  return ret;
}

int k_blkdev_default_ioctl(struct k_disk_device *dd, uint32_t req, 
  void *argp) {
    int rc = 0;
    switch (req) {
    case K_BLKIO_GETMEDIABLKSIZE:
        *(uint32_t *) argp = dd->media_block_size;
        break;
    case K_BLKIO_GETBLKSIZE:
        *(uint32_t *) argp = dd->block_size;
        break;
    case K_BLKIO_SETBLKSIZE:
        rc = k_bdbuf_set_block_size(dd, *(uint32_t *) argp, true);
        if (rc != 0) {
            errno = EIO;
            rc = -1;
        }
        break;
    case K_BLKIO_GETSIZE:
        *(blkdev_bnum_t *)argp = dd->size;
        break;
    case K_BLKIO_SYNCDEV:
        rc = k_bdbuf_syncdev(dd);
        if (rc != 0) {
            errno = EIO;
            rc = -1;
        }
        break;
    case K_BLKIO_GETDISKDEV:
        *(struct k_disk_device **)argp = dd;
        break;
    case K_BLKIO_PURGEDEV:
        k_bdbuf_purge_dev(dd);
        break;
    case K_BLKIO_GETDEVSTATS:
        k_bdbuf_get_device_stats(dd, (struct k_blkdev_stats *)argp);
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

static int blkdev_partition_create(const struct flash_area *fa, 
  blkdev_ioctrl_fn handler) {
    const struct flash_pages_layout *layout;
  const struct flash_driver_api *api;
  struct k_blkdev *ctx;
  const struct device *dev;
  size_t layout_size;
  int ret;

  if (fa == NULL)
    return -EINVAL;
  ret = k_bdbuf_init();
  if (ret) 
    return ret;
  ctx = k_malloc(sizeof(*ctx));
  if (ctx == NULL)
    return -ENOMEM;
  dev = device_get_binding(fa->fa_dev_name);
  if (dev == NULL) {
    ret = -ENODEV;
    goto _freem;
  }
  api = dev->api;
  api->page_layout(dev, &layout, &layout_size);
  ret = k_disk_init_phys(&ctx->dd, layout->pages_size, 
    fa->fa_size/layout->pages_size, handler, &ctx->drv);
  if (ret)
    goto _freem;
  ctx->drv.dev = dev;
  ctx->drv.p = fa;
  k_mutex_lock(&lock, K_FOREVER);
  ctx->next = blkdev_list;
  blkdev_list = ctx;
  k_mutex_unlock(&lock, K_FOREVER);
  return 0;
_freem:
  k_free(ctx);
  return ret;
}

static void blkdev_partition_iterator(const struct flash_area *fa,
    void *user_data) {
    blkdev_partition_create(fa, flash_disk_ioctl);
}

static int blkdev_partition_register(const struct device *dev __unused) {
  flash_area_foreach(blkdev_partition_iterator, NULL);
  return 0;
}

SYS_INIT(blkdev_partition_register,
  POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
