/*
 * Copyright (C) 2001 OKTET Ltd., St.-Petersburg, Russia
 * Author: Victor V. Vengerov <vvv@oktet.ru>
 */

#ifndef _RTEMS_BLKDEV_H
#define _RTEMS_BLKDEV_H

#include <sys/ioccom.h>
#include <stdio.h>

#include "blkdev/blkdev.h"

#ifdef __cplusplus
extern "C" {
#endif

enum k_blkdev_request_op {
  RTEMS_BLKDEV_REQ_READ,       /* Read the requested blocks of data. */
  RTEMS_BLKDEV_REQ_WRITE,      /* Write the requested blocks of data. */
  RTEMS_BLKDEV_REQ_SYNC        /* Sync any data with the media. */
};

struct k_blkdev_request;

/*
 * @brief Block device request done callback function type.
 */
typedef void (*blkdev_request_cb_t)(struct k_blkdev_request *req, 
  int status);
  

struct k_blkdev_sg_buffer {
  blkdev_bnum_t block;
  uint32_t length;
  void *buffer;
  void *user; /* User pointer. */
};

struct k_blkdev_request {
  enum k_blkdev_request_op req;
  blkdev_request_cb_t done; /* Request done callback function. */
  void *done_arg; /* Argument to be passed to callback function. */
  int status; /* Last IO operation completion status. */
  uint32_t bufnum; /* Number of blocks for this request. */
  rtems_id io_task;

  /*
   * TODO: The use of these req blocks is not a great design. The req is a
   *       struct with a single 'bufs' declared in the req struct and the
   *       others are added in the outer level struct. This relies on the
   *       structs joining as a single array and that assumes the compiler
   *       packs the structs. Why not just place on a list ? The BD has a
   *       node that can be used.
   */
  struct k_blkdev_sg_buffer bufs[0];
};


static inline void k_blkdev_request_done(struct k_blkdev_request *req,
   int status) {
  (*req->done)(req, status);
}

/**
 * @brief The start block in a request.
 *
 * Only valid if the driver has returned the
 * @ref RTEMS_BLKDEV_CAP_MULTISECTOR_CONT capability.
 */
#define RTEMS_BLKDEV_START_BLOCK(req) (req->bufs[0].block)

/*
 * IO Control Request Codes
 */
#define RTEMS_BLKIO_REQUEST         _IOWR('B', 1, struct k_blkdev_request)
#define RTEMS_BLKIO_GETMEDIABLKSIZE _IOR('B', 2, uint32_t)
#define RTEMS_BLKIO_GETBLKSIZE      _IOR('B', 3, uint32_t)
#define RTEMS_BLKIO_SETBLKSIZE      _IOW('B', 4, uint32_t)
#define RTEMS_BLKIO_GETSIZE         _IOR('B', 5, blkdev_bnum_t)
#define RTEMS_BLKIO_SYNCDEV         _IO('B', 6)
#define RTEMS_BLKIO_DELETED         _IO('B', 7)
#define RTEMS_BLKIO_CAPABILITIES    _IO('B', 8)
#define RTEMS_BLKIO_GETDISKDEV      _IOR('B', 9, struct k_disk_device *)
#define RTEMS_BLKIO_PURGEDEV        _IO('B', 10)
#define RTEMS_BLKIO_GETDEVSTATS     _IOR('B', 11, struct k_blkdev_stats *)
#define RTEMS_BLKIO_RESETDEVSTATS   _IO('B', 12)


static inline int rtems_disk_fd_get_media_block_size(int fd,
  uint32_t *media_block_size) {
  return ioctl(fd, RTEMS_BLKIO_GETMEDIABLKSIZE, media_block_size);
}

static inline int rtems_disk_fd_get_block_size(int fd, uint32_t *block_size)
{
  return ioctl(fd, RTEMS_BLKIO_GETBLKSIZE, block_size);
}

static inline int rtems_disk_fd_set_block_size(int fd, uint32_t block_size)
{
  return ioctl(fd, RTEMS_BLKIO_SETBLKSIZE, &block_size);
}

static inline int rtems_disk_fd_get_block_count(
  int fd,
  blkdev_bnum_t *block_count
)
{
  return ioctl(fd, RTEMS_BLKIO_GETSIZE, block_count);
}

static inline int rtems_disk_fd_get_disk_device(
  int fd,
  struct k_disk_device **dd_ptr
)
{
  return ioctl(fd, RTEMS_BLKIO_GETDISKDEV, dd_ptr);
}

static inline int rtems_disk_fd_sync(int fd)
{
  return ioctl(fd, RTEMS_BLKIO_SYNCDEV);
}

static inline int rtems_disk_fd_purge(int fd)
{
  return ioctl(fd, RTEMS_BLKIO_PURGEDEV);
}

static inline int rtems_disk_fd_get_device_stats(
  int fd,
  rtems_blkdev_stats *stats
)
{
  return ioctl(fd, RTEMS_BLKIO_GETDEVSTATS, stats);
}

static inline int rtems_disk_fd_reset_device_stats(int fd)
{
  return ioctl(fd, RTEMS_BLKIO_RESETDEVSTATS);
}

/**
 * @brief Only consecutive multi-sector buffer requests are supported.
 *
 * This option means the cache will only supply multiple buffers that are
 * inorder so the ATA multi-sector command for example can be used. This is a
 * hack to work around the current ATA driver.
 */
#define RTEMS_BLKDEV_CAP_MULTISECTOR_CONT (1 << 0)

/**
 * @brief The driver will accept a sync call.
 *
 * A sync call is made to a driver after a bdbuf cache sync has finished.
 */
#define RTEMS_BLKDEV_CAP_SYNC (1 << 1)

/** @} */

/**
 * @brief Common IO control primitive.
 *
 * Use this in all block devices to handle the common set of IO control
 * requests.
 */
int
rtems_blkdev_ioctl(struct k_disk_device *dd, uint32_t req, void *argp);

/**
 * @brief Creates a block device.
 *
 * The block size is set to the media block size.
 *
 * @param[in] device The path for the new block device.
 * @param[in] media_block_size The media block size in bytes.  Must be positive.
 * @param[in] media_block_count The media block count.  Must be positive.
 * @param[in] handler The block device IO control handler.  Must not be @c NULL.
 * @param[in] driver_data The block device driver data.
 *
 * @retval RTEMS_SUCCESSFUL Successful operation.
 * @retval RTEMS_INVALID_NUMBER Media block size or count is not positive.
 * @retval RTEMS_NO_MEMORY Not enough memory.
 * @retval RTEMS_UNSATISFIED Cannot create generic device node.
 * @retval RTEMS_INCORRECT_STATE Cannot initialize bdbuf.
 *
 * @see rtems_blkdev_create_partition(), rtems_bdbuf_set_block_size(), and
 * k_blkdev_request.
 */
rtems_status_code rtems_blkdev_create(
  const char *device,
  uint32_t media_block_size,
  blkdev_bnum_t media_block_count,
  rtems_block_device_ioctl handler,
  void *driver_data
);

/**
 * @brief Creates a partition within a parent block device.
 *
 * A partition manages a subset of consecutive blocks contained in a parent block
 * device.  The blocks must be within the range of blocks managed by the
 * associated parent block device.  The media block size and IO control
 * handler are inherited by the parent block device.  The block size is set to
 * the media block size.
 *
 * @param[in] partition The path for the new partition device.
 * @param[in] parent_block_device The parent block device path.
 * @param[in] media_block_begin The media block begin of the partition within
 * the parent block device.
 * @param[in] media_block_count The media block count of the partition.
 *
 * @retval RTEMS_SUCCESSFUL Successful operation.
 * @retval RTEMS_INVALID_ID Block device node does not exist.
 * @retval RTEMS_INVALID_NODE File system node is not a block device.
 * @retval RTEMS_NOT_IMPLEMENTED Block device implementation is incomplete.
 * @retval RTEMS_INVALID_NUMBER Block begin or block count is invalid.
 * @retval RTEMS_NO_MEMORY Not enough memory.
 * @retval RTEMS_UNSATISFIED Cannot create generic device node.
 *
 * @see rtems_blkdev_create() and rtems_bdbuf_set_block_size().
 */
rtems_status_code rtems_blkdev_create_partition(
  const char *partition,
  const char *parent_block_device,
  blkdev_bnum_t media_block_begin,
  blkdev_bnum_t media_block_count
);

/**
 * @brief Prints the block device statistics.
 */
void rtems_blkdev_print_stats(
  const rtems_blkdev_stats *stats,
  uint32_t media_block_size,
  uint32_t media_block_count,
  uint32_t block_size,
  const rtems_printer* printer
);

/**
 * @brief Block device statistics command.
 */
void rtems_blkstats(
  const rtems_printer *printer,
  const char *device,
  bool reset
);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
