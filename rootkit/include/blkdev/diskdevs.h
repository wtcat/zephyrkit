/*
 * Copyright (C) 2001 OKTET Ltd., St.-Petersburg, Russia
 * Author: Victor V. Vengerov <vvv@oktet.ru>
 */
#ifndef SUBSYS_BLKDEV_DISKDEVS_H_
#define SUBSYS_BLKDEV_DISKDEVS_H_

#include <sys/dlist.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct k_disk_device;
typedef uint32_t blkdev_bnum_t;
typedef uint32_t dev_t;

typedef int (*blkdev_ioctrl_fn)(
  struct k_disk_device *dd,
  uint32_t req,
  void *argp
);

/* Trigger value to disable further read-ahead requests. */
#define K_DISK_READ_AHEAD_NO_TRIGGER ((blkdev_bnum_t) -1)

/* Size value to set number of blocks based on config and disk size. */
#define K_DISK_READ_AHEAD_SIZE_AUTO (0)


struct k_blkdev_read_ahead {
  sys_dnode_t node;
  blkdev_bnum_t trigger;
  blkdev_bnum_t next;
  uint32_t nr_blocks;
};

struct k_blkdev_stats {
  uint32_t read_hits;
  uint32_t read_misses;
  uint32_t read_ahead_transfers;
  uint32_t read_ahead_peeks;
  uint32_t read_blocks;
  uint32_t read_errors;
  uint32_t write_transfers;
  uint32_t write_blocks;
  uint32_t write_errors;
};

struct k_disk_device {
  dev_t dev;
  struct k_disk_device *phys_dev;
  uint32_t capabilities;             /* Driver capabilities. */
  char *name;                        /* Disk device name */
  unsigned uses;                     /* Usage counter. */
  blkdev_bnum_t start;               /* Start media block number. */
  blkdev_bnum_t size;                /* Size of the physical or logical disk in media blocks. */
  uint32_t media_block_size;         /* Media block size in bytes. */
  uint32_t block_size;               /* Block size in bytes. */
  blkdev_bnum_t block_count;         /* Block count. */
  uint32_t media_blocks_per_block;   /* Media blocks per device blocks. */
  int block_to_media_block_shift;    /* media block = block << block_to_media_block_shift */
  size_t bds_per_group;              /* Buffer descriptors per group count. */
  blkdev_ioctrl_fn ioctl;
  void *driver_data;                 /* Private data for the disk driver. */
  bool deleted;
  struct k_blkdev_stats stats;
  struct k_blkdev_read_ahead read_ahead;
};


static inline void *rtems_disk_get_driver_data(
  const struct k_disk_device *dd) {
  return dd->driver_data;
}

static inline uint32_t rtems_disk_get_media_block_size(
  const struct k_disk_device *dd) {
  return dd->media_block_size;
}

static inline uint32_t rtems_disk_get_block_size(
  const struct k_disk_device *dd) {
  return dd->block_size;
}

static inline blkdev_bnum_t rtems_disk_get_block_begin(
  const struct k_disk_device *dd) {
  return dd->start;
}

static inline blkdev_bnum_t rtems_disk_get_block_count(
  const struct k_disk_device *dd) {
  return dd->size;
}

/* Internal function, do not use */
int _k_disk_init_phys(struct k_disk_device *dd, uint32_t block_size, 
  blkdev_bnum_t block_count, blkdev_ioctrl_fn handler, void *driver_data);

/* Internal function, do not use */
int _k_disk_init_log(struct k_disk_device *dd, struct k_disk_device *phys_dd,
  blkdev_bnum_t block_begin, blkdev_bnum_t block_count);
#ifdef __cplusplus
}
#endif
#endif /* SUBSYS_BLKDEV_DISKDEVS_H_ */
