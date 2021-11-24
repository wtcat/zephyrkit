/*
 * Copyright (C) 2001 OKTET Ltd., St.-Petersburg, Russia
 * Author: Victor V. Vengerov <vvv@oktet.ru>
 */

#ifndef SUBSYS_BLKDEV_BLKDEV_H_
#define SUBSYS_BLKDEV_BLKDEV_H_

#include <stdio.h>
#include <kernel.h>

#include "sys/iocom.h"
#include "blkdev/diskdevs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct k_blkdev_request;
typedef void (*blkdev_request_cb_t)(struct k_blkdev_request *req, 
  int status);

struct blkdev_partition {
  const char *devname;
  const char *partition;
	off_t offset;
	size_t size;
};

struct k_blkdev_driver {
  const struct device *dev;
  const struct blkdev_partition *p;
  size_t blksize;
};

struct k_blkdev {
  struct k_disk_device dd;
  struct k_blkdev_driver drv;
  struct k_blkdev *next;
  //atomic_t refcnt;
};

enum k_blkdev_request_op {
  K_BLKDEV_REQ_READ,       /* Read the requested blocks of data. */
  K_BLKDEV_REQ_WRITE,      /* Write the requested blocks of data. */
  K_BLKDEV_REQ_SYNC        /* Sync any data with the media. */
};

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
  struct k_sem *io_sync; //rtems_id io_task;

  /*
   * TODO: The use of these req blocks is not a great design. The req is a
   *       struct with a single 'bufs' declared in the req struct and the
   *       others are added in the outer level struct. This relies on the
   *       structs joining as a single array and that assumes the compiler
   *       packs the structs. Why not just place on a list ? The BD has a
   *       node that can be used.
   */
  struct k_blkdev_sg_buffer bufs[];
};


static inline void k_blkdev_request_done(struct k_blkdev_request *req,
   int status) {
  (*req->done)(req, status);
}

/**
 * @brief The start block in a request.
 *
 * Only valid if the driver has returned the
 * @ref K_BLKDEV_CAP_MULTISECTOR_CONT capability.
 */
#define K_BLKDEV_START_BLOCK(req) (req->bufs[0].block)

/*
 * IO Control Request Codes
 */
#define K_BLKIO_REQUEST         _IOWR('B', 1, struct k_blkdev_request)
#define K_BLKIO_GETMEDIABLKSIZE _IOR('B', 2, uint32_t)
#define K_BLKIO_GETBLKSIZE      _IOR('B', 3, uint32_t)
#define K_BLKIO_SETBLKSIZE      _IOW('B', 4, uint32_t)
#define K_BLKIO_GETSIZE         _IOR('B', 5, blkdev_bnum_t)
#define K_BLKIO_SYNCDEV         _IO('B', 6)
#define K_BLKIO_DELETED         _IO('B', 7)
#define K_BLKIO_CAPABILITIES    _IO('B', 8)
#define K_BLKIO_GETDISKDEV      _IOR('B', 9, struct k_disk_device *)
#define K_BLKIO_PURGEDEV        _IO('B', 10)
#define K_BLKIO_GETDEVSTATS     _IOR('B', 11, struct k_blkdev_stats *)
#define K_BLKIO_RESETDEVSTATS   _IO('B', 12)


/**
 * @brief Only consecutive multi-sector buffer requests are supported.
 *
 * This option means the cache will only supply multiple buffers that are
 * inorder so the ATA multi-sector command for example can be used. This is a
 * hack to work around the current ATA driver.
 */
#define K_BLKDEV_CAP_MULTISECTOR_CONT (1 << 0)

/**
 * @brief The driver will accept a sync call.
 *
 * A sync call is made to a driver after a bdbuf cache sync has finished.
 */
#define K_BLKDEV_CAP_SYNC (1 << 1)

void blkdev_partition_foreach(
    void (*user_cb)(const struct blkdev_partition *, void *), 
    void *user_data);

struct k_blkdev *k_blkdev_get(const char *partition);
int k_blkdev_put(struct k_blkdev *ctx);
ssize_t k_blkdev_read(struct k_blkdev *ctx, void *buffer,
  size_t count, off_t offset);
ssize_t k_blkdev_write(struct k_blkdev *ctx, const void *buffer,
  size_t count, off_t offset);

int k_blkdev_ioctl(struct k_blkdev *dd, uint32_t req, void *argp);
int k_disk_default_ioctl(struct k_disk_device *dd, uint32_t req, 
  void *argp);

int flash_disk_ioctl(struct k_disk_device *dd, uint32_t req, void *arg);

#ifdef __cplusplus
}
#endif
#endif
