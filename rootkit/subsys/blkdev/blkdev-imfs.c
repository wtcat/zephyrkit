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
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <device.h>

struct k_blkdev_context {
  struct k_disk_device dd;
  const struct device *dev;
};

static ssize_t k_blkdev_read(rtems_libio_t *iop, void *buffer,
  size_t count) {
  struct k_blkdev_context *ctx = IMFS_generic_get_context_by_iop(iop);
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

static ssize_t k_blkdev_write(rtems_libio_t *iop, const void *buffer,
  size_t count) {
  struct k_blkdev_context *ctx = IMFS_generic_get_context_by_iop(iop);
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

static int k_blkdev_ioctl(rtems_libio_t *iop, ioctl_command_t request,
  void *buffer) {
  int ret = 0;
  if (request != RTEMS_BLKIO_REQUEST) {
    struct k_blkdev_context *ctx = IMFS_generic_get_context_by_iop(iop);
    struct k_disk_device *dd = &ctx->dd;
    ret = (*dd->ioctl)(dd, request, buffer);
  } else {
    /*
     * It is not allowed to directly access the driver circumventing the cache.
     */
    errno = EINVAL;
    ret = -1;
  }

  return ret;
}

static int rtems_blkdev_imfs_fstat(
  const rtems_filesystem_location_info_t *loc,
  struct stat *buf
)
{
  struct k_blkdev_context *ctx;
  struct k_disk_device *dd;
  IMFS_jnode_t *node;

  ctx = IMFS_generic_get_context_by_location(loc);
  dd = &ctx->dd;

  buf->st_blksize = rtems_disk_get_block_size(dd);
  buf->st_blocks = rtems_disk_get_block_count(dd);

  node = loc->node_access;
  buf->st_rdev = IMFS_generic_get_device_identifier_by_node(node);

  return IMFS_stat(loc, buf);
}

static int rtems_blkdev_imfs_fsync_or_fdatasync(
  rtems_libio_t *iop
)
{
  int rv = 0;
  struct k_blkdev_context *ctx = IMFS_generic_get_context_by_iop(iop);
  struct k_disk_device *dd = &ctx->dd;
  rtems_status_code sc = rtems_bdbuf_syncdev(dd);

  if (sc != RTEMS_SUCCESSFUL) {
    errno = EIO;
    rv = -1;
  }

  return rv;
}


int k_blkdev_create(const char *device, uint32_t media_block_size,
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

int k_blkdev_create_partition(const char *partition,
  const char *parent_block_device, blkdev_bnum_t media_block_begin,
  blkdev_bnum_t media_block_count) {
  struct k_disk_device *phys_dd;
  struct k_blkdev_context *ctx;
  struct device *dev;
  int ret;

  dev = device_get_binding(parent_block_device);
  if (dev == NULL)
    return -ENODEV;
  ret = k_disk_fd_get_disk_device(fd, &phys_dd);
  if (ret)
    return -ESRCH;
  ctx = k_malloc(sizeof(*ctx));
  if (ctx == NULL）
    return -ENOMEM;
  ret = k_disk_init_log(&ctx->dd, phys_dd, media_block_begin, 
    media_block_count);
  if (ret) {
    k_free(ctx);
    goto _exit;
  }
  ctx->dev = dev;
_exit:
  return ret; 
}
