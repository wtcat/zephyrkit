/*
 * Copyright (C) 2001 OKTET Ltd., St.-Petersburg, Russia
 * Author: Victor V. Vengerov <vvv@oktet.ru>
 *
 * Copyright (C) 2008,2009 Chris Johns <chrisj@rtems.org>
 *    Rewritten to remove score mutex access. Fixes many performance
 *    issues.
 *    Change to support demand driven variable buffer sizes.
 *
 * Copyright (c) 2009-2012 embedded brains GmbH.
 */

#ifndef SUBSYS_BLKDEV_BDBUF_H_
#define SUBSYS_BLKDEV_BDBUF_H_

#include <sys/dlist.h>
#include "blkdev/blkdev.h"

#ifdef __cplusplus
extern "C" {
#endif

struct k_bdbuf_group;

enum k_bdbuf_buf_state {
	K_BDBUF_STATE_FREE = 0,
	K_BDBUF_STATE_EMPTY,
	K_BDBUF_STATE_CACHED,
	K_BDBUF_STATE_ACCESS_CACHED,
	K_BDBUF_STATE_ACCESS_MODIFIED,
	K_BDBUF_STATE_ACCESS_EMPTY,
	K_BDBUF_STATE_ACCESS_PURGED,
	K_BDBUF_STATE_MODIFIED,
	K_BDBUF_STATE_SYNC,
	K_BDBUF_STATE_TRANSFER,
	K_BDBUF_STATE_TRANSFER_PURGED
};

struct k_bdbuf_buffer {
	sys_dnode_t link;       /* Link the BD onto a number of lists. */
	struct k_bdbuf_avl_node {
		struct k_bdbuf_buffer* left;   /* Left Child */
		struct k_bdbuf_buffer* right;  /* Right Child */
		signed char            cache;     /* Cache */
		signed char            bal;       /* The balance of the sub-tree */
	} avl;
	struct k_disk_device *dd;               /* disk device */
	blkdev_bnum_t     block;                /* block number on the device */
	unsigned char*    buffer;               /* Pointer to the buffer memory area */
	enum k_bdbuf_buf_state state;               /* State of the buffer. */
	uint32_t waiters;                       /* The number of threads waiting on this buffer. */                       
	struct k_bdbuf_group* group;            /* Pointer to the group of BDs this BD is part of. */                  
	uint32_t hold_timer;                    /* Timer to indicate how long a buffer has been held in the cache modified. */
	int   references;                       /* Allow reference counting by owner. */
	void* user;                             /* User data. */
};

/**
 * A group is a continuous block of buffer descriptors. A group covers the
 * maximum configured buffer size and is the allocation size for the buffers to
 * a specific buffer size. If you allocate a buffer to be a specific size, all
 * buffers in the group, if there are more than 1 will also be that size. The
 * number of buffers in a group is a multiple of 2, ie 1, 2, 4, 8, etc.
 */
struct k_bdbuf_group {
	sys_dnode_t         link;          /* Link the groups on a LRU list if they have no buffers in use. */            
	size_t              bds_per_group; /* The number of BD allocated to this group. This value must be a multiple of 2. */          
	uint32_t            users;         /* How many users the block has. */
	struct k_bdbuf_buffer* bdbuf;      /* First BD this block covers. */
};

int k_bdbuf_init(void);
int k_bdbuf_get(struct k_disk_device *dd, blkdev_bnum_t block, 
	struct k_bdbuf_buffer** bd);
int k_bdbuf_read(struct k_disk_device *dd, blkdev_bnum_t block,
	struct k_bdbuf_buffer** bd);
void k_bdbuf_peek(struct k_disk_device *dd, blkdev_bnum_t block,
	uint32_t nr_blocks);
int k_bdbuf_release(struct k_bdbuf_buffer* bd);
int k_bdbuf_release_modified(struct k_bdbuf_buffer* bd);
int k_bdbuf_sync(struct k_bdbuf_buffer* bd);
int k_bdbuf_syncdev(struct k_disk_device *dd);
void k_bdbuf_purge_dev(struct k_disk_device *dd);
int k_bdbuf_set_block_size(struct k_disk_device *dd,
	uint32_t block_size, bool sync);           
void k_bdbuf_get_device_stats(const struct k_disk_device *dd,
	struct k_blkdev_stats *stats);
void k_bdbuf_reset_device_stats(struct k_disk_device *dd);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_BLKDEV_BDBUF_H_ */
