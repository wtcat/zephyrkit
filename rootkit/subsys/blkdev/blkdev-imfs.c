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

#include <stdlib.h>
#include <string.h>

#include <kernel.h>
#include <device.h>

struct k_blkdev_partition {
  const char *partition;
  const char *devname;
  size_t start;
  size_t size;
};

struct k_blkdev_context {
  struct k_disk_device dd;
  const struct device *dev;
  atomic_t refcnt;
  struct k_blkdev_context *next;
  const struct k_blkdev_partition *p;
};

extern char __blkdev_partition_start[];
extern char __blkdev_partition_end[];

static K_MUTEX_DEFINE(blkdev_lock);
static struct k_blkdev_context *blkdev_list;


int k_blkdev_open(const char *partition, struct k_blkdev_context **ctx) {

}

int k_blkdev_close(struct k_blkdev_context *ctx) {

}

ssize_t k_blkdev_read(struct k_blkdev_context *ctx, void *buffer,
  size_t count) {
  struct k_disk_device *dd = &ctx->dd;
  ssize_t remaining = (ssize_t)count;
  off_t offset = iop->offset;
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
    iop->offset += count;
    rv = (ssize_t)count;
  } else {
    errno = EIO;
    rv = -1;
  }
  return rv;
}

ssize_t k_blkdev_write(struct k_blkdev_context *ctx, const void *buffer,
  size_t count) {
  struct k_disk_device *dd = &ctx->dd;
  ssize_t remaining = (ssize_t)count;
  off_t offset = iop->offset;
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
    iop->offset += count;
    rv = (ssize_t)count;
  } else {
    errno = EIO;
    rv = -1;
  }
  return rv;
}

int k_blkdev_ioctl(struct k_blkdev_context *ctx, 
  ioctl_command_t request, void *buffer) {
  int ret = 0;
  if (request != RTEMS_BLKIO_REQUEST) {
    struct k_disk_device *dd = &ctx->dd;
    ret = (*dd->ioctl)(dd, request, buffer);
  } else {
    errno = EINVAL;
    ret = -1;
  }
  return ret;
}


static int k_blkdev_create(const char *device, uint32_t media_block_size,
  blkdev_bnum_t media_block_count, blkdev_ioctrl_fn handler,
  void *driver_data) {
  struct k_blkdev_context *ctx;
  int ret;

  ret = k_bdbuf_init();
  if (ret) 
    return ret;
  ctx = k_malloc(sizeof(*ctx));
  if (ctx == NULL)
    return -ENOMEM;
  ctx->dev = device_get_binding(device);
  if (ctx->dev == NULL) {
    ret = -ENODEV;
    goto _freem;
  }
  ret = k_disk_init_phys(&ctx->dd, media_block_size, media_block_count, 
    handler, driver_data);
  if (ret)
    goto _freem;
  return 0;

_freem:
  k_free(ctx);
  return ret;
}


