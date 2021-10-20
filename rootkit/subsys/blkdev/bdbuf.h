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

#ifndef SUBSYS_K_BDBUF_H_
#define SUBSYS_K_BDBUF_H_


#ifdef __cplusplus
extern "C" {
#endif

struct k_bdbuf_group;

enum k_bdbuf_state {
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
	rtems_chain_node link;       /* Link the BD onto a number of lists. */
	struct k_bdbuf_avl_node {
		struct struct k_bdbuf_buffer* left;   /* Left Child */
		struct struct k_bdbuf_buffer* right;  /* Right Child */
		signed char                cache;     /* Cache */
		signed char                bal;       /* The balance of the sub-tree */
	} avl;
	struct k_disk_device *dd;               /* disk device */
	rtems_blkdev_bnum block;                /* block number on the device */
	unsigned char*    buffer;               /* Pointer to the buffer memory area */
	enum k_bdbuf_state state;               /* State of the buffer. */
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
	rtems_chain_node    link;          /* Link the groups on a LRU list if they have no buffers in use. */            
	size_t              bds_per_group; /* The number of BD allocated to this group. This value must be a multiple of 2. */          
	uint32_t            users;         /* How many users the block has. */
	struct k_bdbuf_buffer* bdbuf;      /* First BD this block covers. */
};

struct k_bdbuf_config {
	uint32_t            max_read_ahead_blocks;   /* Number of blocks to read ahead. */
	uint32_t            max_write_blocks;        /* Number of blocks to write at once. */                                    
	rtems_task_priority swapout_priority;        /* Priority of the swap out task. */                                           
	uint32_t            swapout_period;          /* Period swap-out checks buf timers. */                                          
	uint32_t            swap_block_hold;         /* Period a buffer is held. */
	size_t              swapout_workers;         /* The number of worker threads for the swap-out task. */        
	rtems_task_priority swapout_worker_priority; /* Priority of the swap out task. */                                
	size_t              task_stack_size;         /* Task stack size for swap-out task and worker threads. */
	size_t              size;                    /* Size of memory in the cache */                                        
	uint32_t            buffer_min;              /* Minimum buffer size. */
	uint32_t            buffer_max;              /* Maximum buffer size supported. It is also the allocation size. */                            
	rtems_task_priority read_ahead_priority;     /* Priority of the read-ahead task. */                                               
};


extern const struct k_bdbuf_config k_bdbuf_configuration;

/**
 * The default value for the maximum read-ahead blocks disables the read-ahead
 * feature.
 */
#define K_BDBUF_MAX_READ_AHEAD_BLOCKS_DEFAULT    0

/**
 * Default maximum number of blocks to write at once.
 */
#define K_BDBUF_MAX_WRITE_BLOCKS_DEFAULT         16

/**
 * Default swap-out task priority.
 */
#define K_BDBUF_SWAPOUT_TASK_PRIORITY_DEFAULT    15

/**
 * Default swap-out task swap period in milli seconds.
 */
#define K_BDBUF_SWAPOUT_TASK_SWAP_PERIOD_DEFAULT 250

/**
 * Default swap-out task block hold time in milli seconds.
 */
#define K_BDBUF_SWAPOUT_TASK_BLOCK_HOLD_DEFAULT  1000

/**
 * Default swap-out worker tasks. Currently disabled.
 */
#define K_BDBUF_SWAPOUT_WORKER_TASKS_DEFAULT     0

/**
 * Default swap-out worker task priority. The same as the swap-out task.
 */
#define K_BDBUF_SWAPOUT_WORKER_TASK_PRIORITY_DEFAULT \
                             K_BDBUF_SWAPOUT_TASK_PRIORITY_DEFAULT

/**
 * Default read-ahead task priority.  The same as the swap-out task.
 */
#define K_BDBUF_READ_AHEAD_TASK_PRIORITY_DEFAULT \
  K_BDBUF_SWAPOUT_TASK_PRIORITY_DEFAULT

/**
 * Default task stack size for swap-out and worker tasks.
 */
#define K_BDBUF_TASK_STACK_SIZE_DEFAULT RTEMS_MINIMUM_STACK_SIZE

/**
 * Default size of memory allocated to the cache.
 */
#define K_BDBUF_CACHE_MEMORY_SIZE_DEFAULT (64 * 512)

/**
 * Default minimum size of buffers.
 */
#define K_BDBUF_BUFFER_MIN_SIZE_DEFAULT (512)

/**
 * Default maximum size of buffers.
 */
#define K_BDBUF_BUFFER_MAX_SIZE_DEFAULT (4096)


int k_bdbuf_init(void);
int k_bdbuf_get(struct k_disk_device *dd, rtems_blkdev_bnum block, 
	struct k_bdbuf_buffer** bd);
int k_bdbuf_read(struct k_disk_device *dd, rtems_blkdev_bnum block,
	struct k_bdbuf_buffer** bd);
void k_bdbuf_peek(struct k_disk_device *dd, rtems_blkdev_bnum block,
	uint32_t nr_blocks);
int k_bdbuf_release(struct k_bdbuf_buffer* bd);
int k_bdbuf_release_modified(struct k_bdbuf_buffer* bd);
int k_bdbuf_sync(struct k_bdbuf_buffer* bd);
int k_bdbuf_syncdev(struct k_disk_device *dd);
void k_bdbuf_purge_dev(struct k_disk_device *dd);
int k_bdbuf_set_block_size(struct k_disk_device *dd,
	uint32_t block_size, bool sync);           
void k_bdbuf_get_device_stats(const struct k_disk_device *dd,
	rtems_blkdev_stats *stats);
void k_bdbuf_reset_device_stats(struct k_disk_device *dd);


#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_K_BDBUF_H_ */
