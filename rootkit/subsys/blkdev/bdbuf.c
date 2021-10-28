/*
 * Disk I/O buffering
 * Buffer managment
 *
 * Copyright (C) 2001 OKTET Ltd., St.-Peterburg, Russia
 * Author: Andrey G. Ivanov <Andrey.Ivanov@oktet.ru>
 *         Victor V. Vengerov <vvv@oktet.ru>
 *         Alexander Kukuta <kam@oktet.ru>
 *
 * Copyright (C) 2008,2009 Chris Johns <chrisj@rtems.org>
 *    Rewritten to remove score mutex access. Fixes many performance
 *    issues.
 *
 * Copyright (c) 2009, 2017 embedded brains GmbH.
 */

/*
 * Copyright (C) 2021
 * Author: wtcat
 */
#include <errno.h>
#include <string.h>

#include <kernel.h>
#include <cache.h>

#include "blkdev/bdbuf.h"

#define BDBUF_INVALID_DEV NULL


struct k_bdbuf_config {
	uint32_t            max_read_ahead_blocks;   /* Number of blocks to read ahead. */
	uint32_t            max_write_blocks;        /* Number of blocks to write at once. */                                    
	int                 swapout_priority;        /* Priority of the swap out task. */                                           
	uint32_t            swapout_period;          /* Period swap-out checks buf timers. */                                          
	uint32_t            swap_block_hold;         /* Period a buffer is held. */
	size_t              swapout_workers;         /* The number of worker threads for the swap-out task. */        
	int                 swapout_worker_priority; /* Priority of the swap out task. */                                
	size_t              task_stack_size;         /* Task stack size for swap-out task and worker threads. */
	size_t              size;                    /* Size of memory in the cache */                                        
	uint32_t            buffer_min;              /* Minimum buffer size. */
	uint32_t            buffer_max;              /* Maximum buffer size supported. It is also the allocation size. */                            
	int                 read_ahead_priority;     /* Priority of the read-ahead task. */                                               
};

struct k_bdbuf_swapout_transfer {
	sys_dlist_t              bds;         /* The transfer list of BDs. */
	struct k_disk_device     *dd;         /* The device the transfer is for. */
	bool                     syncing;     /* The data is a sync'ing. */
	struct k_blkdev_request  write_req;   /* The write request. */
};

struct k_bdbuf_swapout_worker {
  sys_dnode_t     link;     /* The threads sit on a chain when idle. */
	struct k_thread task;     /* The id of the task so we can wake it. */
  struct k_sem    task_sync;
  struct k_sem    io_sync;
	bool            enabled;  /* The worker is enabled. */
	struct k_bdbuf_swapout_transfer transfer; /* The transfer data for this thread. */
};

struct k_bdbuf_waiters {
	unsigned         count;
	struct k_condvar cond_var;
};

struct k_bdbuf_cache {
	struct k_thread     swapout_task;          /* Swapout task ID */
  struct k_sem        swapout_task_sync;
  struct k_sem        swapout_io_sync;
	bool                swapout_enabled;   /* Swapout is only running if enabled. Set to false to kill the swap out task. 
											    It deletes itself. */
	sys_dlist_t swapout_free_workers; /* The work threads for the swapout task. */
	struct k_bdbuf_buffer* bds;               /* Pointer to table of buffer descriptors. */                             
	void*               buffers;           /* The buffer's memory. */
	size_t              buffer_min_count;  /* Number of minimum size buffers that fit the buffer memory. */                                    
	size_t              max_bds_per_group; /* The number of BDs of minimum buffer size that fit in a group. */                                
	uint32_t            flags;             /* Configuration flags. */
	struct k_mutex      lock;              /* The cache lock. It locks all cache data, BD and lists. */                                   
	struct k_mutex      sync_lock;         /* Sync calls block writes. */
	bool                sync_active;       /* True if a sync is active. */
	//rtems_id            sync_requester;    /* The sync requester. */
  struct k_sem        *sync_requester;
	struct k_disk_device  *sync_device;       /* The device to sync and BDBUF_INVALID_DEV not a device sync. */
	struct k_bdbuf_buffer* tree;              /* Buffer descriptor lookup AVL tree root. There is only one. */               
	sys_dlist_t            lru;               /* Least recently used list */
	sys_dlist_t            modified;          /* Modified buffers list */
	sys_dlist_t            sync;              /* Buffers to sync list */
	struct k_bdbuf_waiters access_waiters;    /* Wait for a buffer in ACCESS_CACHED, ACCESS_MODIFIED or
	                                      	   ACCESS_EMPTY state. */                               
	struct k_bdbuf_waiters transfer_waiters;  /* Wait for a buffer in TRANSFER state. */                            
	struct k_bdbuf_waiters buffer_waiters;    /* Wait for a buffer and no one is available. */
	struct k_bdbuf_swapout_transfer *swapout_transfer;
	struct k_bdbuf_swapout_worker *swapout_workers;
	size_t                 group_count;       /* The number of groups. */
	struct k_bdbuf_group*  groups;            /* The groups. */
#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
	struct k_thread        read_ahead_task;   /* Read-ahead task */
  struct k_sem           read_ahead_sync;
	sys_dlist_t            read_ahead_chain;  /* Read-ahead request chain */
	bool                   read_ahead_enabled; /* Read-ahead enabled */
#endif /* CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0 */
  bool                   once;
} ;

enum k_bdbuf_fatal_code {
	K_BDBUF_FATAL_CACHE_WAIT_2,
	K_BDBUF_FATAL_CACHE_WAIT_TO,
	K_BDBUF_FATAL_CACHE_WAKE,
	K_BDBUF_FATAL_PREEMPT_DIS,
	K_BDBUF_FATAL_PREEMPT_RST,
	K_BDBUF_FATAL_RA_WAKE_UP,
	K_BDBUF_FATAL_RECYCLE,
	K_BDBUF_FATAL_SO_WAKE_1,
	K_BDBUF_FATAL_SO_WAKE_2,
	K_BDBUF_FATAL_STATE_0,
	K_BDBUF_FATAL_STATE_2,
	K_BDBUF_FATAL_STATE_4,
	K_BDBUF_FATAL_STATE_5,
	K_BDBUF_FATAL_STATE_6,
	K_BDBUF_FATAL_STATE_7,
	K_BDBUF_FATAL_STATE_8,
	K_BDBUF_FATAL_STATE_9,
	K_BDBUF_FATAL_STATE_10,
	K_BDBUF_FATAL_STATE_11,
	K_BDBUF_FATAL_SWAPOUT_RE,
	K_BDBUF_FATAL_TREE_RM,
	K_BDBUF_FATAL_WAIT_EVNT,
	K_BDBUF_FATAL_WAIT_TRANS_EVNT,
};

static void
z_bdbuf_transfer_done(struct k_blkdev_request* req, int status);
static void z_bdbuf_swapout_task(void *arg);
#if (CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0)
static void z_bdbuf_swapout_worker_task(void *arg);
#endif /* CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0 */
#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
static void z_bdbuf_read_ahead_task(void *arg);
#endif /* CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0 */

static K_THREAD_STACK_DEFINE(swaptask_stack, 
  CONFIG_BDBUF_TASK_STACK_SIZE);
#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
static K_THREAD_STACK_DEFINE(readahead_task_stack, 
  CONFIG_BDBUF_TASK_STACK_SIZE);
#endif /* CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0 */

static struct k_bdbuf_cache bdbuf_cache = {
  .swapout_task_sync = Z_SEM_INITIALIZER(bdbuf_cache.swapout_task_sync, 0, 1),
  .swapout_io_sync = Z_SEM_INITIALIZER(bdbuf_cache.swapout_io_sync, 0, 1),
  .lock = Z_MUTEX_INITIALIZER(bdbuf_cache.lock),
  .sync_lock = Z_MUTEX_INITIALIZER(bdbuf_cache.sync_lock),
  .access_waiters = { 
    .cond_var = Z_CONDVAR_INITIALIZER(bdbuf_cache.access_waiters.cond_var) 
  },
  .transfer_waiters = {
    .cond_var = Z_CONDVAR_INITIALIZER(bdbuf_cache.transfer_waiters.cond_var)
  },
  .buffer_waiters = { 
    .cond_var = Z_CONDVAR_INITIALIZER(bdbuf_cache.buffer_waiters.cond_var) 
  },
#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
  .read_ahead_sync = Z_SEM_INITIALIZER(bdbuf_cache.read_ahead_sync, 0, 1),
#endif /* CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0 */
};

static const struct k_bdbuf_config bdbuf_config = {
  .max_read_ahead_blocks   = CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS,
  .max_write_blocks        = CONFIG_BDBUF_MAX_WRITE_BLOCKS,                                  
  .swapout_priority        = CONFIG_BDBUF_SWAPOUT_TASK_PRIORITY,                                          
  .swapout_period          = CONFIG_BDBUF_SWAPOUT_TASK_SWAP_PERIOD,                                         
  .swap_block_hold         = CONFIG_BDBUF_SWAPOUT_TASK_BLOCK_HOLD,
  .swapout_workers         = CONFIG_BDBUF_SWAPOUT_WORKER_TASKS,     
  .swapout_worker_priority = CONFIG_BDBUF_SWAPOUT_WORKER_TASK_PRIORITY,                       
  .task_stack_size         = CONFIG_BDBUF_TASK_STACK_SIZE,
  .size                    = CONFIG_BDBUF_CACHE_MEMORY_SIZE,                                      
  .buffer_min              = CONFIG_BDBUF_BUFFER_MIN_SIZE,
  .buffer_max              = CONFIG_BDBUF_BUFFER_MAX_SIZE,                           
  .read_ahead_priority     = CONFIG_BDBUF_READ_AHEAD_TASK_PRIORITY
};


#if defined(CONFIG_BDBUF_TRACE)
#define _bdbuf_tracer 1 //bool _bdbuf_tracer;

uint32_t k_bdbuf_list_count(sys_dlist_t* list) {
	sys_dnode_t *node;
	uint32_t count = 0;
	SYS_DLIST_FOR_EACH_NODE(list, node) {
		count++;
	}
	return count;
}

void k_bdbuf_show_usage(void) {
  uint32_t total = 0;
  uint32_t group;
  uint32_t val;
  for (group = 0; group < bdbuf_cache.group_count; group++)
  total += bdbuf_cache.groups[group].users;
  printf ("bdbuf:group users=%u", total);
  val = k_bdbuf_list_count(&bdbuf_cache.lru);
  printf(", lru=%u", val);
  total = val;
  val = k_bdbuf_list_count(&bdbuf_cache.modified);
  printf(", mod=%u", val);
  total += val;
  val = k_bdbuf_list_count(&bdbuf_cache.sync);
  printf(", sync=%u", val);
  total += val;
  printf(", total=%u\n", total);
}

void k_bdbuf_show_users(const char* where, struct k_bdbuf_buffer* bd) {
  const char* states[] =
    { "FR", "EM", "CH", "AC", "AM", "AE", "AP", "MD", "SY", "TR", "TP" };
  printf ("bdbuf:users: %15s: [%" PRIu32 " (%s)] %td:%td = %" PRIu32 " %s\n",
          where,
          bd->block, states[bd->state],
          bd->group - bdbuf_cache.groups,
          bd - bdbuf_cache.bds,
          bd->group->users,
          bd->group->users > 8 ? "<<<<<<<" : "");
}
#else
#define _bdbuf_tracer 0
#define k_bdbuf_tracer (0)
#define k_bdbuf_show_usage() ((void) 0)
#define k_bdbuf_show_users(_w, _b) ((void) 0)
#endif /* CONFIG_BDBUF_TRACE */


static void z_bdbuf_fatal (int error) {
  printk("bdbuf error code: %u\n", (unsigned int)error);
  k_oops();
}

static inline void z_bdbuf_wait_for_sync(struct k_sem *sem, 
  k_timeout_t timeout, int status) {
  int ret = k_sem_take(sem, timeout);
  if (ret)
    z_bdbuf_fatal(status);
}

#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0) || \
    (CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0)
static void z_bdbuf_wait_for_event(struct k_sem *sem) {
  z_bdbuf_wait_for_sync(sem, K_FOREVER, 
    K_BDBUF_FATAL_WAIT_EVNT);
}
#endif

static void z_bdbuf_wait_for_transient_event(struct k_sem *sem) {
  z_bdbuf_wait_for_sync(sem, K_FOREVER, 
    K_BDBUF_FATAL_WAIT_TRANS_EVNT);
}

static inline void z_bdbuf_send_event(struct k_sem *sem) {
  k_sem_give(sem);
}

/**
 * The default maximum height of 32 allows for AVL trees having between
 * 5,704,880 and 4,294,967,295 nodes, depending on order of insertion.  You may
 * change this compile-time constant as you wish.
 */
#ifndef K_BDBUF_AVL_MAX_HEIGHT
#define K_BDBUF_AVL_MAX_HEIGHT CONFIG_BDBUF_AVL_MAX_HEIGHT
#endif

static void z_bdbuf_fatal_with_state (enum k_bdbuf_buf_state state,
  enum k_bdbuf_fatal_code error) {
  z_bdbuf_fatal ((((uint32_t) state) << 16) | error);
}

static struct k_bdbuf_buffer *z_bdbuf_avl_search(struct k_bdbuf_buffer** root,
  const struct k_disk_device *dd, blkdev_bnum_t block) {
  struct k_bdbuf_buffer* p = *root;
  while ((p != NULL) && 
    ((p->dd != dd) || (p->block != block))) {
    if (((uintptr_t)p->dd < (uintptr_t)dd) || 
      ((p->dd == dd) && (p->block < block))) {
        p = p->avl.right;
      } else {
        p = p->avl.left;
      }
  }
  return p;
}

static int z_bdbuf_avl_insert(struct k_bdbuf_buffer** root,
  struct k_bdbuf_buffer *node) {
  const struct k_disk_device *dd = node->dd;
  blkdev_bnum_t block = node->block;

  struct k_bdbuf_buffer*  p = *root;
  struct k_bdbuf_buffer*  q;
  struct k_bdbuf_buffer*  p1;
  struct k_bdbuf_buffer*  p2;
  struct k_bdbuf_buffer*  buf_stack[K_BDBUF_AVL_MAX_HEIGHT];
  struct k_bdbuf_buffer** buf_prev = buf_stack;

  bool modified = false;

  if (p == NULL)
  {
    *root = node;
    node->avl.left = NULL;
    node->avl.right = NULL;
    node->avl.bal = 0;
    return 0;
  }

  while (p != NULL)
  {
    *buf_prev++ = p;

    if (((uintptr_t) p->dd < (uintptr_t) dd)
        || ((p->dd == dd) && (p->block < block)))
    {
      p->avl.cache = 1;
      q = p->avl.right;
      if (q == NULL)
      {
        q = node;
        p->avl.right = q = node;
        break;
      }
    }
    else if ((p->dd != dd) || (p->block != block))
    {
      p->avl.cache = -1;
      q = p->avl.left;
      if (q == NULL)
      {
        q = node;
        p->avl.left = q;
        break;
      }
    }
    else
    {
      return -1;
    }

    p = q;
  }

  q->avl.left = q->avl.right = NULL;
  q->avl.bal = 0;
  modified = true;
  buf_prev--;

  while (modified)
  {
    if (p->avl.cache == -1)
    {
      switch (p->avl.bal)
      {
        case 1:
          p->avl.bal = 0;
          modified = false;
          break;

        case 0:
          p->avl.bal = -1;
          break;

        case -1:
          p1 = p->avl.left;
          if (p1->avl.bal == -1) /* simple LL-turn */
          {
            p->avl.left = p1->avl.right;
            p1->avl.right = p;
            p->avl.bal = 0;
            p = p1;
          }
          else /* double LR-turn */
          {
            p2 = p1->avl.right;
            p1->avl.right = p2->avl.left;
            p2->avl.left = p1;
            p->avl.left = p2->avl.right;
            p2->avl.right = p;
            if (p2->avl.bal == -1) p->avl.bal = +1; else p->avl.bal = 0;
            if (p2->avl.bal == +1) p1->avl.bal = -1; else p1->avl.bal = 0;
            p = p2;
          }
          p->avl.bal = 0;
          modified = false;
          break;

        default:
          break;
      }
    }
    else
    {
      switch (p->avl.bal)
      {
        case -1:
          p->avl.bal = 0;
          modified = false;
          break;

        case 0:
          p->avl.bal = 1;
          break;

        case 1:
          p1 = p->avl.right;
          if (p1->avl.bal == 1) /* simple RR-turn */
          {
            p->avl.right = p1->avl.left;
            p1->avl.left = p;
            p->avl.bal = 0;
            p = p1;
          }
          else /* double RL-turn */
          {
            p2 = p1->avl.left;
            p1->avl.left = p2->avl.right;
            p2->avl.right = p1;
            p->avl.right = p2->avl.left;
            p2->avl.left = p;
            if (p2->avl.bal == +1) p->avl.bal = -1; else p->avl.bal = 0;
            if (p2->avl.bal == -1) p1->avl.bal = +1; else p1->avl.bal = 0;
            p = p2;
          }
          p->avl.bal = 0;
          modified = false;
          break;

        default:
          break;
      }
    }
    q = p;
    if (buf_prev > buf_stack)
    {
      p = *--buf_prev;

      if (p->avl.cache == -1)
      {
        p->avl.left = q;
      }
      else
      {
        p->avl.right = q;
      }
    }
    else
    {
      *root = p;
      break;
    }
  };

  return 0;
}

static int z_bdbuf_avl_remove(struct k_bdbuf_buffer** root,
  const struct k_bdbuf_buffer *node) {
  const struct k_disk_device *dd = node->dd;
  blkdev_bnum_t block = node->block;

  struct k_bdbuf_buffer*  p = *root;
  struct k_bdbuf_buffer*  q;
  struct k_bdbuf_buffer*  r;
  struct k_bdbuf_buffer*  s;
  struct k_bdbuf_buffer*  p1;
  struct k_bdbuf_buffer*  p2;
  struct k_bdbuf_buffer*  buf_stack[K_BDBUF_AVL_MAX_HEIGHT];
  struct k_bdbuf_buffer** buf_prev = buf_stack;

  bool modified = false;

  memset (buf_stack, 0, sizeof(buf_stack));

  while (p != NULL)
  {
    *buf_prev++ = p;

    if (((uintptr_t) p->dd < (uintptr_t) dd)
        || ((p->dd == dd) && (p->block < block)))
    {
      p->avl.cache = 1;
      p = p->avl.right;
    }
    else if ((p->dd != dd) || (p->block != block))
    {
      p->avl.cache = -1;
      p = p->avl.left;
    }
    else
    {
      /* node found */
      break;
    }
  }

  if (p == NULL)
  {
    /* there is no such node */
    return -1;
  }

  q = p;

  buf_prev--;
  if (buf_prev > buf_stack)
  {
    p = *(buf_prev - 1);
  }
  else
  {
    p = NULL;
  }

  /* at this moment q - is a node to delete, p is q's parent */
  if (q->avl.right == NULL)
  {
    r = q->avl.left;
    if (r != NULL)
    {
      r->avl.bal = 0;
    }
    q = r;
  }
  else
  {
    struct k_bdbuf_buffer **t;

    r = q->avl.right;

    if (r->avl.left == NULL)
    {
      r->avl.left = q->avl.left;
      r->avl.bal = q->avl.bal;
      r->avl.cache = 1;
      *buf_prev++ = q = r;
    }
    else
    {
      t = buf_prev++;
      s = r;

      while (s->avl.left != NULL)
      {
        *buf_prev++ = r = s;
        s = r->avl.left;
        r->avl.cache = -1;
      }

      s->avl.left = q->avl.left;
      r->avl.left = s->avl.right;
      s->avl.right = q->avl.right;
      s->avl.bal = q->avl.bal;
      s->avl.cache = 1;

      *t = q = s;
    }
  }

  if (p != NULL)
  {
    if (p->avl.cache == -1)
    {
      p->avl.left = q;
    }
    else
    {
      p->avl.right = q;
    }
  }
  else
  {
    *root = q;
  }

  modified = true;

  while (modified)
  {
    if (buf_prev > buf_stack)
    {
      p = *--buf_prev;
    }
    else
    {
      break;
    }

    if (p->avl.cache == -1)
    {
      /* rebalance left branch */
      switch (p->avl.bal)
      {
        case -1:
          p->avl.bal = 0;
          break;
        case  0:
          p->avl.bal = 1;
          modified = false;
          break;

        case +1:
          p1 = p->avl.right;

          if (p1->avl.bal >= 0) /* simple RR-turn */
          {
            p->avl.right = p1->avl.left;
            p1->avl.left = p;

            if (p1->avl.bal == 0)
            {
              p1->avl.bal = -1;
              modified = false;
            }
            else
            {
              p->avl.bal = 0;
              p1->avl.bal = 0;
            }
            p = p1;
          }
          else /* double RL-turn */
          {
            p2 = p1->avl.left;

            p1->avl.left = p2->avl.right;
            p2->avl.right = p1;
            p->avl.right = p2->avl.left;
            p2->avl.left = p;

            if (p2->avl.bal == +1) p->avl.bal = -1; else p->avl.bal = 0;
            if (p2->avl.bal == -1) p1->avl.bal = 1; else p1->avl.bal = 0;

            p = p2;
            p2->avl.bal = 0;
          }
          break;

        default:
          break;
      }
    }
    else
    {
      /* rebalance right branch */
      switch (p->avl.bal)
      {
        case +1:
          p->avl.bal = 0;
          break;

        case  0:
          p->avl.bal = -1;
          modified = false;
          break;

        case -1:
          p1 = p->avl.left;

          if (p1->avl.bal <= 0) /* simple LL-turn */
          {
            p->avl.left = p1->avl.right;
            p1->avl.right = p;
            if (p1->avl.bal == 0)
            {
              p1->avl.bal = 1;
              modified = false;
            }
            else
            {
              p->avl.bal = 0;
              p1->avl.bal = 0;
            }
            p = p1;
          }
          else /* double LR-turn */
          {
            p2 = p1->avl.right;

            p1->avl.right = p2->avl.left;
            p2->avl.left = p1;
            p->avl.left = p2->avl.right;
            p2->avl.right = p;

            if (p2->avl.bal == -1) p->avl.bal = 1; else p->avl.bal = 0;
            if (p2->avl.bal == +1) p1->avl.bal = -1; else p1->avl.bal = 0;

            p = p2;
            p2->avl.bal = 0;
          }
          break;

        default:
          break;
      }
    }

    if (buf_prev > buf_stack)
    {
      q = *(buf_prev - 1);

      if (q->avl.cache == -1)
      {
        q->avl.left = p;
      }
      else
      {
        q->avl.right = p;
      }
    }
    else
    {
      *root = p;
      break;
    }

  }

  return 0;
}

static void z_bdbuf_set_state(struct k_bdbuf_buffer *bd, 
  enum k_bdbuf_buf_state state) {
  bd->state = state;
}

static blkdev_bnum_t z_bdbuf_media_block(const struct k_disk_device *dd, 
  blkdev_bnum_t block) {
  if (dd->block_to_media_block_shift >= 0)
    return block << dd->block_to_media_block_shift;
  else
    return (blkdev_bnum_t)
      ((((uint64_t)block) * dd->block_size) / dd->media_block_size);
}

static void z_bdbuf_lock(struct k_mutex *lock) {
  k_mutex_lock(lock, K_FOREVER);
}

static void z_bdbuf_unlock(struct k_mutex *lock) {
  k_mutex_unlock(lock);
}

static void z_bdbuf_lock_cache(void) {
  z_bdbuf_lock(&bdbuf_cache.lock);
}

static void z_bdbuf_unlock_cache(void) {
  z_bdbuf_unlock(&bdbuf_cache.lock);
}

static void z_bdbuf_lock_sync(void) {
  z_bdbuf_lock(&bdbuf_cache.sync_lock);
}

static void z_bdbuf_unlock_sync(void) {
  z_bdbuf_unlock(&bdbuf_cache.sync_lock);
}

static void z_bdbuf_group_obtain(struct k_bdbuf_buffer *bd) {
  ++bd->group->users;
}

static void z_bdbuf_group_release(struct k_bdbuf_buffer *bd) {
  --bd->group->users;
}

static void z_bdbuf_anonymous_wait(struct k_bdbuf_waiters *waiters) {
  ++waiters->count;
  k_condvar_wait(&waiters->cond_var, &bdbuf_cache.lock, K_FOREVER);
  --waiters->count;
}

static void z_bdbuf_wait(struct k_bdbuf_buffer *bd, 
  struct k_bdbuf_waiters *waiters) {
  z_bdbuf_group_obtain(bd);
  ++bd->waiters;
  z_bdbuf_anonymous_wait(waiters);
  --bd->waiters;
  z_bdbuf_group_release(bd);
}

static void z_bdbuf_wake(struct k_bdbuf_waiters *waiters) {
  if (waiters->count > 0)
    k_condvar_broadcast(&waiters->cond_var);
}

static void z_bdbuf_wake_swapper(void) {
  z_bdbuf_send_event(&bdbuf_cache.swapout_task_sync);
}

static bool z_bdbuf_has_buffer_waiters(void) {
  return bdbuf_cache.buffer_waiters.count;
}

static void z_bdbuf_remove_from_tree(struct k_bdbuf_buffer *bd) {
  if (z_bdbuf_avl_remove(&bdbuf_cache.tree, bd) != 0)
    z_bdbuf_fatal_with_state(bd->state, K_BDBUF_FATAL_TREE_RM);
}

static void z_bdbuf_remove_from_tree_and_lru_list(struct k_bdbuf_buffer *bd) {
  switch (bd->state) {
  case K_BDBUF_STATE_FREE:
    break;
  case K_BDBUF_STATE_CACHED:
    z_bdbuf_remove_from_tree(bd);
    break;
  default:
    z_bdbuf_fatal_with_state(bd->state, K_BDBUF_FATAL_STATE_10);
  }
  sys_dlist_remove(&bd->link);
}

static void z_bdbuf_make_free_and_add_to_lru_list(struct k_bdbuf_buffer *bd) {
  z_bdbuf_set_state(bd, K_BDBUF_STATE_FREE);
  sys_dlist_prepend(&bdbuf_cache.lru, &bd->link);
}

static void z_bdbuf_make_empty(struct k_bdbuf_buffer *bd) {
  z_bdbuf_set_state(bd, K_BDBUF_STATE_EMPTY);
}

static void z_bdbuf_make_cached_and_add_to_lru_list(struct k_bdbuf_buffer *bd) {
  z_bdbuf_set_state(bd, K_BDBUF_STATE_CACHED);
  sys_dlist_append(&bdbuf_cache.lru, &bd->link);
}

static void z_bdbuf_discard_buffer(struct k_bdbuf_buffer *bd) {
  z_bdbuf_make_empty (bd);
  if (bd->waiters == 0) {
    z_bdbuf_remove_from_tree(bd);
    z_bdbuf_make_free_and_add_to_lru_list(bd);
  }
}

static void z_bdbuf_add_to_modified_list_after_access(
  struct k_bdbuf_buffer *bd) {
  if (bdbuf_cache.sync_active && bdbuf_cache.sync_device == bd->dd) {
    z_bdbuf_unlock_cache();
    /* Wait for the sync lock. */
    z_bdbuf_lock_sync();
    z_bdbuf_unlock_sync();
    z_bdbuf_lock_cache();
  }

  /*
   * Only the first modified release sets the timer and any further user
   * accesses do not change the timer value which should move down. This
   * assumes the user's hold of the buffer is much less than the time on the
   * modified list. Resetting the timer on each access which could result in a
   * buffer never getting to 0 and never being forced onto disk. This raises a
   * difficult question. Is a snapshot of a block that is changing better than
   * nothing being written? We have tended to think we should hold changes for
   * only a specific period of time even if still changing and get onto disk
   * and letting the file system try and recover this position if it can.
   */
  if (bd->state == K_BDBUF_STATE_ACCESS_CACHED
        || bd->state == K_BDBUF_STATE_ACCESS_EMPTY)
    bd->hold_timer = bdbuf_config.swap_block_hold;
  z_bdbuf_set_state(bd, K_BDBUF_STATE_MODIFIED);
  sys_dlist_append(&bdbuf_cache.modified, &bd->link);
  if (bd->waiters)
    z_bdbuf_wake(&bdbuf_cache.access_waiters);
  else if (z_bdbuf_has_buffer_waiters())
    z_bdbuf_wake_swapper();
}

static void z_bdbuf_add_to_lru_list_after_access(struct k_bdbuf_buffer *bd) {
  z_bdbuf_group_release(bd);
  z_bdbuf_make_cached_and_add_to_lru_list(bd);
  if (bd->waiters)
    z_bdbuf_wake (&bdbuf_cache.access_waiters);
  else
    z_bdbuf_wake (&bdbuf_cache.buffer_waiters);
}

static size_t z_bdbuf_bds_per_group(size_t size) {
  size_t bufs_per_size;
  size_t bds_per_size;
  if (size > bdbuf_config.buffer_max)
    return 0;
  bufs_per_size = ((size - 1) / bdbuf_config.buffer_min) + 1;
  for (bds_per_size = 1;
       bds_per_size < bufs_per_size;
       bds_per_size <<= 1)
    ;
  return bdbuf_cache.max_bds_per_group / bds_per_size;
}

static void z_bdbuf_discard_buffer_after_access(struct k_bdbuf_buffer *bd) {
  z_bdbuf_group_release(bd);
  z_bdbuf_discard_buffer(bd);
  if (bd->waiters)
    z_bdbuf_wake(&bdbuf_cache.access_waiters);
  else
    z_bdbuf_wake(&bdbuf_cache.buffer_waiters);
}

static struct k_bdbuf_buffer *z_bdbuf_group_realloc(
  struct k_bdbuf_group* group, size_t new_bds_per_group) {
  struct k_bdbuf_buffer* bd;
  size_t              b;
  size_t              bufs_per_bd;
  if (_bdbuf_tracer)
    printf ("bdbuf:realloc: %tu: %zd -> %zd\n",
            group - bdbuf_cache.groups, group->bds_per_group,
            new_bds_per_group);

  bufs_per_bd = bdbuf_cache.max_bds_per_group / group->bds_per_group;
  for (b = 0, bd = group->bdbuf;
       b < group->bds_per_group;
       b++, bd += bufs_per_bd)
    z_bdbuf_remove_from_tree_and_lru_list(bd);

  group->bds_per_group = new_bds_per_group;
  bufs_per_bd = bdbuf_cache.max_bds_per_group / new_bds_per_group;
  for (b = 1, bd = group->bdbuf + bufs_per_bd;
       b < group->bds_per_group;
       b++, bd += bufs_per_bd)
    z_bdbuf_make_free_and_add_to_lru_list(bd);
  if (b > 1)
    z_bdbuf_wake(&bdbuf_cache.buffer_waiters);
  return group->bdbuf;
}

static void rtems_bdbuf_setup_empty_buffer(struct k_bdbuf_buffer *bd,
  struct k_disk_device *dd, blkdev_bnum_t block) {
  bd->dd        = dd ;
  bd->block     = block;
  bd->avl.left  = NULL;
  bd->avl.right = NULL;
  bd->waiters   = 0;
  if (z_bdbuf_avl_insert(&bdbuf_cache.tree, bd) != 0)
    z_bdbuf_fatal(K_BDBUF_FATAL_RECYCLE);
  z_bdbuf_make_empty(bd);
}

static struct k_bdbuf_buffer *z_bdbuf_get_buffer_from_lru_list(
  struct k_disk_device *dd, blkdev_bnum_t block) {
  sys_dnode_t *node;
  SYS_DLIST_FOR_EACH_NODE(&bdbuf_cache.lru, node) {
    struct k_bdbuf_buffer *bd = CONTAINER_OF(node, struct k_bdbuf_buffer, link);
    struct k_bdbuf_buffer *empty_bd = NULL;
    if (_bdbuf_tracer) {
      printf ("bdbuf:next-bd: %tu (%td:%" PRId32 ") %zd -> %zd\n",
              bd - bdbuf_cache.bds,
              bd->group - bdbuf_cache.groups, bd->group->users,
              bd->group->bds_per_group, dd->bds_per_group);
    }
    /* If nobody waits for this BD, we may recycle it. */
    if (bd->waiters == 0) {
      if (bd->group->bds_per_group == dd->bds_per_group) {
        z_bdbuf_remove_from_tree_and_lru_list(bd);
        empty_bd = bd;
      } else if (bd->group->users == 0)
        empty_bd = z_bdbuf_group_realloc(bd->group, dd->bds_per_group);
    }
    if (empty_bd != NULL) {
      rtems_bdbuf_setup_empty_buffer(empty_bd, dd, block);
      return empty_bd;
    }
  }
  return NULL;
}

static int z_bdbuf_create_task(struct k_thread *new_thread, 
  k_thread_stack_t *stack, const char *name, int prio,
  void (*entry)(void *), void *arg) {
  size_t stack_size = CONFIG_BDBUF_TASK_STACK_SIZE;
  if (stack == NULL) {
    stack = k_malloc(sizeof(*stack) * stack_size);
    if (stack == NULL)
      return -ENOMEM;
  }
  k_tid_t thread = k_thread_create(new_thread, stack, stack_size,		  
	  (k_thread_entry_t)entry, arg, NULL, NULL, prio, 0, K_FOREVER);
  if (name)
    k_thread_name_set(thread, name);
  return 0;	  
}

static struct k_bdbuf_swapout_transfer *
z_bdbuf_swapout_transfer_alloc(void) {
  /*
   * @note chrisj The struct k_blkdev_request and the array at the end is a hack.
   * I am disappointment at finding code like this in RTEMS. The request should
   * have been a sys_dlist_t. Simple, fast and less storage as the node
   * is already part of the buffer structure.
   */
  size_t transfer_size = sizeof(struct k_bdbuf_swapout_transfer)
    + (bdbuf_config.max_write_blocks * sizeof (struct k_blkdev_sg_buffer));
  return k_calloc(1, transfer_size);
}

static void
z_bdbuf_swapout_transfer_init(struct k_bdbuf_swapout_transfer* transfer,
  struct k_sem *sync) {
  sys_dlist_init(&transfer->bds);
  transfer->write_req.io_sync = sync;
  transfer->dd = BDBUF_INVALID_DEV;
  transfer->syncing = false;
  transfer->write_req.req = K_BLKDEV_REQ_WRITE;
  transfer->write_req.done = z_bdbuf_transfer_done;
}

#if (CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0)
static size_t z_bdbuf_swapout_worker_size(void) {
  return sizeof(struct k_bdbuf_swapout_worker) +
    (bdbuf_config.max_write_blocks * sizeof(struct k_blkdev_sg_buffer));
}

static int z_bdbuf_swapout_workers_create(void) {
  char *worker_current;
  char name[32];
  size_t worker_size;
  size_t w;
  int ret = 0;

  worker_size = z_bdbuf_swapout_worker_size();
  worker_current = k_calloc(1, bdbuf_config.swapout_workers * worker_size);
  if (worker_current == NULL)
    return -ENOMEM;
  bdbuf_cache.swapout_workers = 
    (struct k_bdbuf_swapout_worker *)worker_current;
  for (w = 0; w < bdbuf_config.swapout_workers;
       w++, worker_current += worker_size) {
    struct k_bdbuf_swapout_worker *worker = 
      (struct k_bdbuf_swapout_worker *)worker_current;

    snprintf(name, sizeof(name), "swapworker@%d", w);
    ret = z_bdbuf_create_task(&worker->task, NULL, name, 
      bdbuf_config.swapout_worker_priority, z_bdbuf_swapout_worker_task, worker);
    if (ret) 
      break;
    k_sem_init(&worker->task_sync, 0, 1);
    k_sem_init(&worker->io_sync, 0, 1);
    z_bdbuf_swapout_transfer_init(&worker->transfer, &worker->io_sync);
    sys_dlist_append(&bdbuf_cache.swapout_free_workers, &worker->link);
    worker->enabled = true;
    k_thread_start(&bdbuf_cache.swapout_task);
  }
  return ret;
}
#endif /* CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0 */

static size_t z_bdbuf_read_request_size(uint32_t transfer_count) {
  return sizeof (struct k_blkdev_request) +
    sizeof(struct k_blkdev_sg_buffer) * transfer_count;
}

static int z_bdbuf_do_init(void) {
  struct k_bdbuf_group* group;
  struct k_bdbuf_buffer* bd;
  uint8_t* buffer;
  size_t b;
  int ret;

  if (_bdbuf_tracer)
    printf ("bdbuf:init\n");
  if (k_is_in_isr())
    return -ESRCH;
  if ((bdbuf_config.buffer_max % bdbuf_config.buffer_min) != 0)
    return -EINVAL;
  if (z_bdbuf_read_request_size (bdbuf_config.max_read_ahead_blocks)
      > CONFIG_BDBUF_TASK_STACK_SIZE / 8U)
    return -EINVAL;
  bdbuf_cache.sync_device = BDBUF_INVALID_DEV;
  sys_dlist_init(&bdbuf_cache.swapout_free_workers);
  sys_dlist_init(&bdbuf_cache.lru);
  sys_dlist_init(&bdbuf_cache.modified);
  sys_dlist_init(&bdbuf_cache.sync);
#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
  sys_dlist_init(&bdbuf_cache.read_ahead_chain);
#endif
  z_bdbuf_lock_cache();

  /* Compute the various number of elements in the cache. */
  bdbuf_cache.buffer_min_count =
    bdbuf_config.size / bdbuf_config.buffer_min;
  bdbuf_cache.max_bds_per_group =
    bdbuf_config.buffer_max / bdbuf_config.buffer_min;
  bdbuf_cache.group_count =
    bdbuf_cache.buffer_min_count / bdbuf_cache.max_bds_per_group;

  /* Allocate the memory for the buffer descriptors. */
  bdbuf_cache.bds = k_calloc(sizeof(struct k_bdbuf_buffer), 
    bdbuf_cache.buffer_min_count);     
  if (!bdbuf_cache.bds)
    goto error;

  /* Allocate the memory for the buffer descriptors. */
  bdbuf_cache.groups = k_calloc(sizeof(struct k_bdbuf_group), 
    bdbuf_cache.group_count);        
  if (!bdbuf_cache.groups)
    goto error;

  /*
   * Allocate memory for buffer memory. The buffer memory will be cache
   * aligned. It is possible to k_free the memory allocated by
   * rtems_cache_aligned_malloc() with k_free().
   */
  bdbuf_cache.buffers = k_aligned_alloc(sys_cache_data_line_size_get(), 
    bdbuf_cache.buffer_min_count * bdbuf_config.buffer_min);
  if (bdbuf_cache.buffers == NULL)
    goto error;

  /*
   * The cache is empty after opening so we need to add all the buffers to it
   * and initialise the groups.
   */
  for (b = 0, group = bdbuf_cache.groups,
         bd = bdbuf_cache.bds, buffer = bdbuf_cache.buffers;
       b < bdbuf_cache.buffer_min_count;
       b++, bd++, buffer += bdbuf_config.buffer_min) {
    bd->dd     = BDBUF_INVALID_DEV;
    bd->group  = group;
    bd->buffer = buffer;
    sys_dlist_append(&bdbuf_cache.lru, &bd->link);
    if ((b % bdbuf_cache.max_bds_per_group) ==
        (bdbuf_cache.max_bds_per_group - 1))
      group++;
  }
  for (b = 0, group = bdbuf_cache.groups,
       bd = bdbuf_cache.bds;
       b < bdbuf_cache.group_count;
       b++, group++, bd += bdbuf_cache.max_bds_per_group) {
    group->bds_per_group = bdbuf_cache.max_bds_per_group;
    group->bdbuf = bd;
  }

  /* Create and start swapout task. */
  bdbuf_cache.swapout_transfer = z_bdbuf_swapout_transfer_alloc();
  if (!bdbuf_cache.swapout_transfer)
    goto error;

  bdbuf_cache.swapout_enabled = true;
  ret = z_bdbuf_create_task(&bdbuf_cache.swapout_task, swaptask_stack, 
    "swapout", 
    bdbuf_config.swapout_worker_priority, z_bdbuf_swapout_task, 
    bdbuf_cache.swapout_transfer);
  if (ret) 
    goto error;
 
  z_bdbuf_swapout_transfer_init(bdbuf_cache.swapout_transfer, 
    &bdbuf_cache.swapout_io_sync);
  k_thread_start(&bdbuf_cache.swapout_task);

#if (CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0)
  if (bdbuf_config.swapout_workers > 0) {
    ret = z_bdbuf_swapout_workers_create();
    if (ret)
      goto del_swapout;
  }
#endif /* CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0 */
#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
  if (bdbuf_config.max_read_ahead_blocks > 0) {
    bdbuf_cache.read_ahead_enabled = true;
    ret = z_bdbuf_create_task(&bdbuf_cache.read_ahead_task, readahead_task_stack,
     "bdbuf@readahead", 
      bdbuf_config.read_ahead_priority, z_bdbuf_read_ahead_task, NULL);
    if (ret)
      goto del_workers;
    k_sem_init(&bdbuf_cache.read_ahead_sync, 0, 1);
    k_thread_start(&bdbuf_cache.read_ahead_task);
  }
#endif /* CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0 */
  z_bdbuf_unlock_cache();
  return 0;


#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
del_workers:
#endif
#if (CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0)
  if (true) {
    char *worker_current = (char *)bdbuf_cache.swapout_workers;
    size_t worker_size = z_bdbuf_swapout_worker_size();
    size_t w;
    for (w = 0; w < bdbuf_config.swapout_workers;
         w++, worker_current += worker_size) {
      struct k_bdbuf_swapout_worker *worker = 
        (struct k_bdbuf_swapout_worker *)worker_current;
      k_thread_abort(&worker->task);
    }
  }
del_swapout:
#endif /* CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0 */
  k_thread_abort(&bdbuf_cache.swapout_task);
error:
  k_free(bdbuf_cache.buffers);
  k_free(bdbuf_cache.groups);
  k_free(bdbuf_cache.bds);
  k_free(bdbuf_cache.swapout_transfer);
  k_free(bdbuf_cache.swapout_workers);
  z_bdbuf_unlock_cache();
  return -ENOMEM;
}

//TODO: not multi-thread safe
int k_bdbuf_init(void) {
  if (!bdbuf_cache.once) {
    bdbuf_cache.once = true;
    return z_bdbuf_do_init();
  }
  return 0;
}

static void z_bdbuf_wait_for_access(struct k_bdbuf_buffer *bd) {
  while (true) {
    switch (bd->state) {
    case K_BDBUF_STATE_MODIFIED:
      z_bdbuf_group_release (bd);
      /* Fall through */
    case K_BDBUF_STATE_CACHED:
      sys_dlist_remove (&bd->link);
      /* Fall through */
    case K_BDBUF_STATE_EMPTY:
      return;
    case K_BDBUF_STATE_ACCESS_CACHED:
    case K_BDBUF_STATE_ACCESS_EMPTY:
    case K_BDBUF_STATE_ACCESS_MODIFIED:
    case K_BDBUF_STATE_ACCESS_PURGED:
      z_bdbuf_wait (bd, &bdbuf_cache.access_waiters);
      break;
    case K_BDBUF_STATE_SYNC:
    case K_BDBUF_STATE_TRANSFER:
    case K_BDBUF_STATE_TRANSFER_PURGED:
      z_bdbuf_wait (bd, &bdbuf_cache.transfer_waiters);
      break;
    default:
      z_bdbuf_fatal_with_state (bd->state, K_BDBUF_FATAL_STATE_7);
    }
  }
}

static void z_bdbuf_request_sync_for_modified_buffer(
  struct k_bdbuf_buffer *bd) {
  z_bdbuf_set_state (bd, K_BDBUF_STATE_SYNC);
  sys_dlist_remove(&bd->link);
  sys_dlist_append(&bdbuf_cache.sync, &bd->link);
  z_bdbuf_wake_swapper();
}

static bool z_bdbuf_wait_for_recycle(struct k_bdbuf_buffer *bd) {
  while (true) {
    switch (bd->state) {
    case K_BDBUF_STATE_FREE:
      return true;
    case K_BDBUF_STATE_MODIFIED:
      z_bdbuf_request_sync_for_modified_buffer(bd);
      break;
    case K_BDBUF_STATE_CACHED:
    case K_BDBUF_STATE_EMPTY:
      if (bd->waiters == 0)
        return true;
      else
      {
        /*
          * It is essential that we wait here without a special wait count and
          * without the group in use.  Otherwise we could trigger a wait ping
          * pong with another recycle waiter.  The state of the buffer is
          * arbitrary afterwards.
          */
        z_bdbuf_anonymous_wait(&bdbuf_cache.buffer_waiters);
        return false;
      }
    case K_BDBUF_STATE_ACCESS_CACHED:
    case K_BDBUF_STATE_ACCESS_EMPTY:
    case K_BDBUF_STATE_ACCESS_MODIFIED:
    case K_BDBUF_STATE_ACCESS_PURGED:
      z_bdbuf_wait(bd, &bdbuf_cache.access_waiters);
      break;
    case K_BDBUF_STATE_SYNC:
    case K_BDBUF_STATE_TRANSFER:
    case K_BDBUF_STATE_TRANSFER_PURGED:
      z_bdbuf_wait(bd, &bdbuf_cache.transfer_waiters);
      break;
    default:
      z_bdbuf_fatal_with_state(bd->state, K_BDBUF_FATAL_STATE_8);
    }
  }
}

static void z_bdbuf_wait_for_sync_done(struct k_bdbuf_buffer *bd) {
  while (true) {
    switch (bd->state) {
    case K_BDBUF_STATE_CACHED:
    case K_BDBUF_STATE_EMPTY:
    case K_BDBUF_STATE_MODIFIED:
    case K_BDBUF_STATE_ACCESS_CACHED:
    case K_BDBUF_STATE_ACCESS_EMPTY:
    case K_BDBUF_STATE_ACCESS_MODIFIED:
    case K_BDBUF_STATE_ACCESS_PURGED:
      return;
    case K_BDBUF_STATE_SYNC:
    case K_BDBUF_STATE_TRANSFER:
    case K_BDBUF_STATE_TRANSFER_PURGED:
      z_bdbuf_wait(bd, &bdbuf_cache.transfer_waiters);
      break;
    default:
      z_bdbuf_fatal_with_state(bd->state, K_BDBUF_FATAL_STATE_9);
    }
  }
}

static void z_bdbuf_wait_for_buffer(void) {
  if (!sys_dlist_is_empty(&bdbuf_cache.modified))
    z_bdbuf_wake_swapper();
  z_bdbuf_anonymous_wait(&bdbuf_cache.buffer_waiters);
}

static void z_bdbuf_sync_after_access(struct k_bdbuf_buffer *bd) {
  z_bdbuf_set_state(bd, K_BDBUF_STATE_SYNC);
  sys_dlist_append(&bdbuf_cache.sync, &bd->link);
  if (bd->waiters)
    z_bdbuf_wake (&bdbuf_cache.access_waiters);
  z_bdbuf_wake_swapper();
  z_bdbuf_wait_for_sync_done(bd);

  /* We may have created a cached or empty buffer which may be recycled. */
  if (bd->waiters == 0 && 
     (bd->state == K_BDBUF_STATE_CACHED || 
      bd->state == K_BDBUF_STATE_EMPTY)) {
    if (bd->state == K_BDBUF_STATE_EMPTY) {
      z_bdbuf_remove_from_tree (bd);
      z_bdbuf_make_free_and_add_to_lru_list (bd);
    }
    z_bdbuf_wake(&bdbuf_cache.buffer_waiters);
  }
}

static struct k_bdbuf_buffer *z_bdbuf_get_buffer_for_read_ahead(
  struct k_disk_device *dd, blkdev_bnum_t block) {
  struct k_bdbuf_buffer *bd = NULL;
  bd = z_bdbuf_avl_search(&bdbuf_cache.tree, dd, block);
  if (bd == NULL) {
    bd = z_bdbuf_get_buffer_from_lru_list(dd, block);
    if (bd != NULL)
      z_bdbuf_group_obtain (bd);
  } else {
    /*
     * The buffer is in the cache.  So it is already available or in use, and
     * thus no need for a read ahead.
     */
    bd = NULL;
  }
  return bd;
}

static struct k_bdbuf_buffer *z_bdbuf_get_buffer_for_access(
  struct k_disk_device *dd, blkdev_bnum_t block) {
  struct k_bdbuf_buffer *bd = NULL;
  do {
    bd = z_bdbuf_avl_search(&bdbuf_cache.tree, dd, block);
    if (bd != NULL) {
      if (bd->group->bds_per_group != dd->bds_per_group) {
        if (z_bdbuf_wait_for_recycle(bd)) {
          z_bdbuf_remove_from_tree_and_lru_list (bd);
          z_bdbuf_make_free_and_add_to_lru_list (bd);
          z_bdbuf_wake(&bdbuf_cache.buffer_waiters);
        }
        bd = NULL;
      }
    } else {
      bd = z_bdbuf_get_buffer_from_lru_list(dd, block);
      if (bd == NULL)
        z_bdbuf_wait_for_buffer();
    }
  } while (bd == NULL);
  z_bdbuf_wait_for_access(bd);
  z_bdbuf_group_obtain(bd);
  return bd;
}

static int z_bdbuf_get_media_block(const struct k_disk_device *dd,
  blkdev_bnum_t block, blkdev_bnum_t *media_block_ptr) {               
  if (block < dd->block_count) {
    /*
     * Compute the media block number. Drivers work with media block number not
     * the block number a BD may have as this depends on the block size set by
     * the user.
     */
    *media_block_ptr = z_bdbuf_media_block(dd, block) + dd->start;
    return 0;
  }
  return -EINVAL;
}

int k_bdbuf_get(struct k_disk_device *dd, blkdev_bnum_t block,
  struct k_bdbuf_buffer **bd_ptr) {
  struct k_bdbuf_buffer *bd = NULL;
  blkdev_bnum_t media_block;
  z_bdbuf_lock_cache();
  int ret = z_bdbuf_get_media_block(dd, block, &media_block);
  if (ret == 0) {
    /* Print the block index relative to the physical disk. */
    if (_bdbuf_tracer) {
      printf ("bdbuf:get: %" PRIu32 " (%" PRIu32 ") (dev = %08x)\n",
              media_block, block, (unsigned) dd->dev);
    }
    bd = z_bdbuf_get_buffer_for_access(dd, media_block);
    switch (bd->state) {
    case K_BDBUF_STATE_CACHED:
      z_bdbuf_set_state(bd, K_BDBUF_STATE_ACCESS_CACHED);
      break;
    case K_BDBUF_STATE_EMPTY:
      z_bdbuf_set_state(bd, K_BDBUF_STATE_ACCESS_EMPTY);
      break;
    case K_BDBUF_STATE_MODIFIED:
      /*
        * To get a modified buffer could be considered a bug in the caller
        * because you should not be getting an already modified buffer but
        * user may have modified a byte in a block then decided to seek the
        * start and write the whole block and the file system will have no
        * record of this so just gets the block to fill.
        */
      z_bdbuf_set_state (bd, K_BDBUF_STATE_ACCESS_MODIFIED);
      break;
    default:
      z_bdbuf_fatal_with_state (bd->state, K_BDBUF_FATAL_STATE_2);
      break;
    }
    if (_bdbuf_tracer) {
      k_bdbuf_show_users("get", bd);
      k_bdbuf_show_usage();
    }
  }
  z_bdbuf_unlock_cache();
  *bd_ptr = bd;
  return ret;
}

static void
z_bdbuf_transfer_done(struct k_blkdev_request* req, int status) {
  req->status = status;
  if (likely(req->io_sync))
    z_bdbuf_send_event(req->io_sync);
}

static int
z_bdbuf_execute_transfer_request(struct k_disk_device *dd,
  struct k_blkdev_request *req, bool cache_locked) {
  uint32_t transfer_index = 0;
  bool wake_transfer_waiters = false;
  bool wake_buffer_waiters = false;
  int ret;

  if (cache_locked)
    z_bdbuf_unlock_cache();
  k_sem_init(req->io_sync, 0, 1);
  dd->ioctl(dd->phys_dev, K_BLKIO_REQUEST, req);
  z_bdbuf_wait_for_transient_event(req->io_sync);
  ret = req->status;
  z_bdbuf_lock_cache ();
  if (req->req == K_BLKDEV_REQ_READ) {
    dd->stats.read_blocks += req->bufnum;
    if (ret)
      ++dd->stats.read_errors;
  } else {
    dd->stats.write_blocks += req->bufnum;
    ++dd->stats.write_transfers;
    if (ret)
      ++dd->stats.write_errors;
  }

  for (transfer_index = 0; transfer_index < req->bufnum; 
    ++transfer_index) {
    struct k_bdbuf_buffer *bd = req->bufs[transfer_index].user;
    bool waiters = bd->waiters;
    if (waiters)
      wake_transfer_waiters = true;
    else
      wake_buffer_waiters = true;
    z_bdbuf_group_release(bd);
    if (ret == 0 && bd->state == K_BDBUF_STATE_TRANSFER)
      z_bdbuf_make_cached_and_add_to_lru_list(bd);
    else
      z_bdbuf_discard_buffer(bd);
    if (_bdbuf_tracer)
      k_bdbuf_show_users ("transfer", bd);
  }
  if (wake_transfer_waiters)
    z_bdbuf_wake(&bdbuf_cache.transfer_waiters);
  if (wake_buffer_waiters)
    z_bdbuf_wake(&bdbuf_cache.buffer_waiters);
  if (!cache_locked)
    z_bdbuf_unlock_cache();
  return ret;
}

static int z_bdbuf_execute_read_request(struct k_disk_device *dd,
  struct k_bdbuf_buffer *bd, uint32_t transfer_count) {        
  struct k_blkdev_request *req = NULL;
  struct k_sem io_sem;
  blkdev_bnum_t media_block = bd->block;
  uint32_t media_blocks_per_block = dd->media_blocks_per_block;
  uint32_t block_size = dd->block_size;
  uint32_t transfer_index = 1;
  int ret;

  //TODO: This type of request structure is wrong and should be removed.
#define bdbuf_alloc(size) __builtin_alloca(size)
  req = bdbuf_alloc(z_bdbuf_read_request_size(transfer_count));
  req->io_sync = &io_sem; //req->io_task = rtems_task_self ();
  req->req = K_BLKDEV_REQ_READ;
  req->done = z_bdbuf_transfer_done;
  req->bufnum = 0;

  z_bdbuf_set_state(bd, K_BDBUF_STATE_TRANSFER);
  req->bufs [0].user   = bd;
  req->bufs [0].block  = media_block;
  req->bufs [0].length = block_size;
  req->bufs [0].buffer = bd->buffer;
  if (_bdbuf_tracer)
    k_bdbuf_show_users("read", bd);

  while (transfer_index < transfer_count) {
    media_block += media_blocks_per_block;
    bd = z_bdbuf_get_buffer_for_read_ahead(dd, media_block);
    if (bd == NULL)
      break;
    z_bdbuf_set_state(bd, K_BDBUF_STATE_TRANSFER);
    req->bufs [transfer_index].user   = bd;
    req->bufs [transfer_index].block  = media_block;
    req->bufs [transfer_index].length = block_size;
    req->bufs [transfer_index].buffer = bd->buffer;
    if (_bdbuf_tracer)
      k_bdbuf_show_users("read", bd);
    ++transfer_index;
  }
  req->bufnum = transfer_index;
  ret = z_bdbuf_execute_transfer_request(dd, req, true);
  req->io_sync = NULL;
  return ret;
}

#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
static bool z_bdbuf_is_read_ahead_active(const struct k_disk_device *dd) {
  return sys_dnode_is_linked(&dd->read_ahead.node);
}

static void z_bdbuf_read_ahead_cancel(struct k_disk_device *dd) {
  if (z_bdbuf_is_read_ahead_active(dd))
    sys_dlist_remove(&dd->read_ahead.node);
}

static void z_bdbuf_read_ahead_reset(struct k_disk_device *dd) {
  z_bdbuf_read_ahead_cancel(dd);
  dd->read_ahead.trigger = K_DISK_READ_AHEAD_NO_TRIGGER;
}

static void z_bdbuf_read_ahead_add_to_chain(struct k_disk_device *dd) {
  sys_dlist_t *chain = &bdbuf_cache.read_ahead_chain;
  if (sys_dlist_is_empty(chain)) 
    z_bdbuf_send_event(&bdbuf_cache.read_ahead_sync);
  sys_dlist_append(chain, &dd->read_ahead.node);
}

static void
z_bdbuf_check_read_ahead_trigger(struct k_disk_device *dd,
  blkdev_bnum_t block) {
  if (dd->read_ahead.trigger == block &&
      !z_bdbuf_is_read_ahead_active (dd)) {
    dd->read_ahead.nr_blocks = K_DISK_READ_AHEAD_SIZE_AUTO;
    z_bdbuf_read_ahead_add_to_chain(dd);
  }
}

static void z_bdbuf_set_read_ahead_trigger(struct k_disk_device *dd,
  blkdev_bnum_t block) {
  if (dd->read_ahead.trigger != block) {
    z_bdbuf_read_ahead_cancel(dd);
    dd->read_ahead.trigger = block + 1;
    dd->read_ahead.next = block + 2;
  }
}
#endif /* CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0 */

int k_bdbuf_read(struct k_disk_device *dd, blkdev_bnum_t block,
  struct k_bdbuf_buffer **bd_ptr) {
  struct k_bdbuf_buffer *bd = NULL;
  blkdev_bnum_t media_block;
  int ret;

  z_bdbuf_lock_cache();
  ret = z_bdbuf_get_media_block(dd, block, &media_block);
  if (ret == 0) {
    if (_bdbuf_tracer) {
      printf("bdbuf:read: %" PRIu32 " (%" PRIu32 ") (dev = %08x)\n",
              media_block, block, (unsigned) dd->dev);
    }
    bd = z_bdbuf_get_buffer_for_access(dd, media_block);
    switch (bd->state) {
    case K_BDBUF_STATE_CACHED:
      ++dd->stats.read_hits;
      z_bdbuf_set_state(bd, K_BDBUF_STATE_ACCESS_CACHED);
      break;
    case K_BDBUF_STATE_MODIFIED:
      ++dd->stats.read_hits;
      z_bdbuf_set_state(bd, K_BDBUF_STATE_ACCESS_MODIFIED);
      break;
    case K_BDBUF_STATE_EMPTY:
      ++dd->stats.read_misses;
#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
      z_bdbuf_set_read_ahead_trigger(dd, block);
#endif /* CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0 */
      ret = z_bdbuf_execute_read_request(dd, bd, 1);
      if (ret == 0) {
        z_bdbuf_set_state(bd, K_BDBUF_STATE_ACCESS_CACHED);
        sys_dlist_remove(&bd->link);
        z_bdbuf_group_obtain(bd);
      } else {
        bd = NULL;
      }
      break;
    default:
      z_bdbuf_fatal_with_state (bd->state, K_BDBUF_FATAL_STATE_4);
      break;
    }
#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
    z_bdbuf_check_read_ahead_trigger(dd, block);
#endif
  }
  z_bdbuf_unlock_cache();
  *bd_ptr = bd;
  return ret;
}

#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
void k_bdbuf_peek(struct k_disk_device *dd, blkdev_bnum_t block,
  uint32_t nr_blocks) {
  z_bdbuf_lock_cache ();
  if (bdbuf_cache.read_ahead_enabled && nr_blocks > 0) {
    z_bdbuf_read_ahead_reset(dd);
    dd->read_ahead.next = block;
    dd->read_ahead.nr_blocks = nr_blocks;
    z_bdbuf_read_ahead_add_to_chain(dd);
  }
  z_bdbuf_unlock_cache();
}
#endif /* CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0 */

static int z_bdbuf_check_bd_and_lock_cache(struct k_bdbuf_buffer *bd,
  const char *kind) {
  if (bd == NULL)
    return -EINVAL;
  if (_bdbuf_tracer) {
    printf("bdbuf:%s: %" PRIu32 "\n", kind, bd->block);
    k_bdbuf_show_users(kind, bd);
  }
  z_bdbuf_lock_cache();
  return 0;
}

int k_bdbuf_release(struct k_bdbuf_buffer *bd) {
  int ret = z_bdbuf_check_bd_and_lock_cache(bd, "release");
  if (ret)
    return ret;
  switch (bd->state) {
  case K_BDBUF_STATE_ACCESS_CACHED:
    z_bdbuf_add_to_lru_list_after_access(bd);
    break;
  case K_BDBUF_STATE_ACCESS_EMPTY:
  case K_BDBUF_STATE_ACCESS_PURGED:
    z_bdbuf_discard_buffer_after_access(bd);
    break;
  case K_BDBUF_STATE_ACCESS_MODIFIED:
    z_bdbuf_add_to_modified_list_after_access(bd);
    break;
  default:
    z_bdbuf_fatal_with_state(bd->state, K_BDBUF_FATAL_STATE_0);
    break;
  }
  if (_bdbuf_tracer)
    k_bdbuf_show_usage();
  z_bdbuf_unlock_cache();
  return ret;
}

int k_bdbuf_release_modified (struct k_bdbuf_buffer *bd) {
  int ret = z_bdbuf_check_bd_and_lock_cache(bd, "release modified");
  if (ret)
    return ret;
  switch (bd->state) {
  case K_BDBUF_STATE_ACCESS_CACHED:
  case K_BDBUF_STATE_ACCESS_EMPTY:
  case K_BDBUF_STATE_ACCESS_MODIFIED:
    z_bdbuf_add_to_modified_list_after_access(bd);
    break;
  case K_BDBUF_STATE_ACCESS_PURGED:
    z_bdbuf_discard_buffer_after_access(bd);
    break;
  default:
    z_bdbuf_fatal_with_state(bd->state, K_BDBUF_FATAL_STATE_6);
    break;
  }
  if (_bdbuf_tracer)
    k_bdbuf_show_usage();
  z_bdbuf_unlock_cache();
  return ret;
}

int k_bdbuf_sync(struct k_bdbuf_buffer *bd) {
  int ret = z_bdbuf_check_bd_and_lock_cache(bd, "sync");
  if (ret)
    return ret;
  switch (bd->state) {
  case K_BDBUF_STATE_ACCESS_CACHED:
  case K_BDBUF_STATE_ACCESS_EMPTY:
  case K_BDBUF_STATE_ACCESS_MODIFIED:
    z_bdbuf_sync_after_access(bd);
    break;
  case K_BDBUF_STATE_ACCESS_PURGED:
    z_bdbuf_discard_buffer_after_access(bd);
    break;
  default:
    z_bdbuf_fatal_with_state(bd->state, K_BDBUF_FATAL_STATE_5);
    break;
  }
  if (_bdbuf_tracer)
    k_bdbuf_show_usage();
  z_bdbuf_unlock_cache();
  return ret;
}

int k_bdbuf_syncdev(struct k_disk_device *dd) {
  struct k_sem syncsem;
  if (_bdbuf_tracer)
    printf("bdbuf:syncdev: %08x\n", (unsigned) dd->dev);
  /*
   * Take the sync lock before locking the cache. Once we have the sync lock we
   * can lock the cache. If another thread has the sync lock it will cause this
   * thread to block until it owns the sync lock then it can own the cache. The
   * sync lock can only be obtained with the cache unlocked.
   */
  z_bdbuf_lock_sync ();
  z_bdbuf_lock_cache ();

  /*
   * Set the cache to have a sync active for a specific device and let the swap
   * out task know the id of the requester to wake when done.
   *
   * The swap out task will negate the sync active flag when no more buffers
   * for the device are held on the "modified for sync" queues.
   */
  k_sem_init(&syncsem, 0, 1);
  bdbuf_cache.sync_active    = true;
  bdbuf_cache.sync_requester = &syncsem;
  bdbuf_cache.sync_device    = dd;
  z_bdbuf_wake_swapper();
  z_bdbuf_unlock_cache();
  z_bdbuf_wait_for_transient_event(&syncsem);
  z_bdbuf_unlock_sync();
  return 0;
}

static void
z_bdbuf_swapout_write(struct k_bdbuf_swapout_transfer* transfer) {
  sys_dnode_t *node;
  if (_bdbuf_tracer)
    printf("bdbuf:swapout transfer: %08x\n", (unsigned)transfer->dd->dev);
  if (!sys_dlist_is_empty(&transfer->bds)) {
    uint32_t last_block = 0;
    struct k_disk_device *dd = transfer->dd;
    uint32_t media_blocks_per_block = dd->media_blocks_per_block;
    bool need_continuous_blocks =
      (dd->phys_dev->capabilities & K_BLKDEV_CAP_MULTISECTOR_CONT) != 0;

    transfer->write_req.status = EBUSY;
    transfer->write_req.bufnum = 0;
    while ((node = sys_dlist_get(&transfer->bds)) != NULL) {
      struct k_bdbuf_buffer* bd = CONTAINER_OF(node, 
        struct k_bdbuf_buffer, link);
      bool write = false;

      /*
       * If the device only accepts sequential buffers and this is not the
       * first buffer (the first is always sequential, and the buffer is not
       * sequential then put the buffer back on the transfer chain and write
       * the committed buffers.
       */
      if (_bdbuf_tracer) {
        printf("bdbuf:swapout write: bd:%" PRIu32 ", bufnum:%" PRIu32 " mode:%s\n",
                bd->block, transfer->write_req.bufnum,
                need_continuous_blocks ? "MULTI" : "SCAT");
      }
      if (need_continuous_blocks && 
          transfer->write_req.bufnum &&
          bd->block != last_block + media_blocks_per_block) {
        sys_dlist_prepend(&transfer->bds, &bd->link);
        write = true;
      } else {
        struct k_blkdev_sg_buffer* buf;
        buf = &transfer->write_req.bufs[transfer->write_req.bufnum];
        transfer->write_req.bufnum++;
        buf->user   = bd;
        buf->block  = bd->block;
        buf->length = dd->block_size;
        buf->buffer = bd->buffer;
        last_block  = bd->block;
      }
      if (sys_dlist_is_empty(&transfer->bds) ||
          (transfer->write_req.bufnum >= bdbuf_config.max_write_blocks))
        write = true;
      if (write) {
        z_bdbuf_execute_transfer_request(dd, &transfer->write_req, false);
        transfer->write_req.status = EBUSY;
        transfer->write_req.bufnum = 0;
      }
    }
    if (transfer->syncing &&
        (dd->phys_dev->capabilities & K_BLKDEV_CAP_SYNC)) {
      //TODO: How should the error be handled ? */
       dd->ioctl(dd->phys_dev, K_BLKDEV_REQ_SYNC, NULL);
    }
  }
}

static void
z_bdbuf_swapout_modified_processing(struct k_disk_device **dd_ptr,
  sys_dlist_t* chain, sys_dlist_t* transfer, bool sync_active,
  bool update_timers, uint32_t timer_delta) {

  if (!sys_dlist_is_empty(chain)) {
    sys_dnode_t* node = sys_dlist_peek_head_not_empty(chain);
    bool sync_all;

    if (sync_active && (*dd_ptr == BDBUF_INVALID_DEV))
      sync_all = true;
    else
      sync_all = false;

    while (node != (sys_dnode_t *)chain) {
      struct k_bdbuf_buffer* bd = CONTAINER_OF(node, 
        struct k_bdbuf_buffer, link);
      if (sync_all || (sync_active && (*dd_ptr == bd->dd))
          || z_bdbuf_has_buffer_waiters())
        bd->hold_timer = 0;

      if (bd->hold_timer) {
        if (update_timers) {
          if (bd->hold_timer > timer_delta)
            bd->hold_timer -= timer_delta;
          else
            bd->hold_timer = 0;
        }
        if (bd->hold_timer) {
          node = node->next;
          continue;
        }
      }

      /*
       * This assumes we can set it to BDBUF_INVALID_DEV which is just an
       * assumption. Cannot use the transfer list being empty the sync dev
       * calls sets the dev to use.
       */
      if (*dd_ptr == BDBUF_INVALID_DEV)
        *dd_ptr = bd->dd;
      if (bd->dd == *dd_ptr) {
        sys_dnode_t* next_node = node->next;
        sys_dnode_t* tnode = transfer->tail;

        /*
         * The blocks on the transfer list are sorted in block order. This
         * means multi-block transfers for drivers that require consecutive
         * blocks perform better with sorted blocks and for real disks it may
         * help lower head movement.
         */
        z_bdbuf_set_state(bd, K_BDBUF_STATE_TRANSFER);
        sys_dlist_remove(node);
        while (node && !sys_dlist_is_head(transfer, tnode)) {
          struct k_bdbuf_buffer* tbd = CONTAINER_OF(tnode, 
            struct k_bdbuf_buffer, link);
          if (bd->block > tbd->block) {
            sys_dlist_insert(tnode->next, node); 
            node = NULL;
          } else
            tnode = tnode->prev;
        }
        if (node)
          sys_dlist_prepend(transfer, node);
        node = next_node;
      } else {
        node = node->next;
      }
    }
  }
}

static bool
z_bdbuf_swapout_processing(unsigned long timer_delta,
  bool update_timers, struct k_bdbuf_swapout_transfer* transfer) {
  struct k_bdbuf_swapout_worker* worker;
  bool transfered_buffers = false;
  bool sync_active;

  z_bdbuf_lock_cache ();
  sync_active = bdbuf_cache.sync_active;

  /*
   * If a sync is active do not use a worker because the current code does not
   * cleaning up after. We need to know the buffers have been written when
   * syncing to release sync lock and currently worker threads do not return to
   * here. We do not know the worker is the last in a sequence of sync writes
   * until after we have it running so we do not know to tell it to release the
   * lock. The simplest solution is to get the main swap out task perform all
   * sync operations.
   */
  if (sync_active) {
    worker = NULL;
  } else {
    worker = (struct k_bdbuf_swapout_worker*)sys_dlist_get(
      &bdbuf_cache.swapout_free_workers);
    if (worker)
      transfer = &worker->transfer;
  }
  sys_dlist_init(&transfer->bds);
  transfer->dd = BDBUF_INVALID_DEV;
  transfer->syncing = sync_active;

  /*
   * When the sync is for a device limit the sync to that device. If the sync
   * is for a buffer handle process the devices in the order on the sync
   * list. This means the dev is BDBUF_INVALID_DEV.
   */
  if (sync_active)
    transfer->dd = bdbuf_cache.sync_device;

  /*
   * If we have any buffers in the sync queue move them to the modified
   * list. The first sync buffer will select the device we use.
   */
  z_bdbuf_swapout_modified_processing(&transfer->dd, &bdbuf_cache.sync,
    &transfer->bds, true, false, timer_delta);

  /* Process the cache's modified list. */
  z_bdbuf_swapout_modified_processing(&transfer->dd, &bdbuf_cache.modified,
    &transfer->bds, sync_active, update_timers, timer_delta);

  /*
   * We have all the buffers that have been modified for this device so the
   * cache can be unlocked because the state of each buffer has been set to
   * TRANSFER.
   */
  z_bdbuf_unlock_cache ();
  if (!sys_dlist_is_empty(&transfer->bds)) {
    if (worker)
      z_bdbuf_send_event(&worker->task_sync);
    else
      z_bdbuf_swapout_write(transfer);
    transfered_buffers = true;
  }
  if (sync_active && !transfered_buffers) {
    struct k_sem *sync_requester;
    z_bdbuf_lock_cache();
    sync_requester = bdbuf_cache.sync_requester;
    bdbuf_cache.sync_active = false;
    bdbuf_cache.sync_requester = NULL;
    z_bdbuf_unlock_cache();
    if (sync_requester)
      z_bdbuf_send_event(sync_requester);
  }
  return transfered_buffers;
}

#if (CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0)
static void z_bdbuf_swapout_worker_task(void *arg) {
  struct k_bdbuf_swapout_worker* worker = arg;
  while (worker->enabled) {
    z_bdbuf_wait_for_event(&worker->task_sync);
    z_bdbuf_swapout_write(&worker->transfer);
    z_bdbuf_lock_cache();
    sys_dlist_init(&worker->transfer.bds);
    worker->transfer.dd = BDBUF_INVALID_DEV;
    sys_dlist_append(&bdbuf_cache.swapout_free_workers, &worker->link);
    z_bdbuf_unlock_cache();
  }
  k_free(worker);
  k_thread_abort(k_current_get());
}

static void z_bdbuf_swapout_workers_close(void) {
  sys_dnode_t* node;
  z_bdbuf_lock_cache();
  SYS_DLIST_FOR_EACH_NODE(&bdbuf_cache.swapout_free_workers, node) {
    struct k_bdbuf_swapout_worker* worker = CONTAINER_OF(node,
      struct k_bdbuf_swapout_worker, link);
    worker->enabled = false;
    z_bdbuf_send_event(&worker->sync);
  }
  z_bdbuf_unlock_cache();
}
#endif /* CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0 */

static void z_bdbuf_swapout_task(void *arg) {
  const uint32_t period_in_msecs = bdbuf_config.swapout_period;
  struct k_bdbuf_swapout_transfer* transfer = arg;
  k_timeout_t period_in_ticks;
  uint32_t timer_delta;

  period_in_ticks = K_USEC(period_in_msecs * 1000);
  //TODO: This is temporary. Needs to be changed to use the real time clock.
  timer_delta = period_in_msecs;
  while (bdbuf_cache.swapout_enabled) {
    bool update_timers = true;
    bool transfered_buffers;
    do {
      transfered_buffers = false;
      if (z_bdbuf_swapout_processing(timer_delta,
        update_timers, transfer)) {
        transfered_buffers = true;
      }
      update_timers = false;
    } while (transfered_buffers);
    
    z_bdbuf_wait_for_sync(&bdbuf_cache.swapout_task_sync, period_in_ticks, 
      K_BDBUF_FATAL_SWAPOUT_RE);
  }
#if (CONFIG_BDBUF_SWAPOUT_WORKER_TASKS > 0)
  z_bdbuf_swapout_workers_close();
#endif
  k_free(transfer);
  k_thread_abort(k_current_get());
}

static void z_bdbuf_purge_list(sys_dlist_t *purge_list) {
  bool wake_buffer_waiters = false;
  sys_dnode_t *node = NULL;

  while ((node = sys_dlist_get(purge_list)) != NULL) {
    struct k_bdbuf_buffer *bd = CONTAINER_OF(node, 
      struct k_bdbuf_buffer, link);
    if (bd->waiters == 0)
      wake_buffer_waiters = true;
    z_bdbuf_discard_buffer(bd);
  }
  if (wake_buffer_waiters)
    z_bdbuf_wake(&bdbuf_cache.buffer_waiters);
}

static void
z_bdbuf_gather_for_purge(sys_dlist_t *purge_list,
  const struct k_disk_device *dd) {
  struct k_bdbuf_buffer *stack [K_BDBUF_AVL_MAX_HEIGHT];
  struct k_bdbuf_buffer **prev = stack;
  struct k_bdbuf_buffer *cur = bdbuf_cache.tree;

  *prev = NULL;
  while (cur != NULL) {
    if (cur->dd == dd) {
      switch (cur->state) {
      case K_BDBUF_STATE_FREE:
      case K_BDBUF_STATE_EMPTY:
      case K_BDBUF_STATE_ACCESS_PURGED:
      case K_BDBUF_STATE_TRANSFER_PURGED:
        break;
      case K_BDBUF_STATE_SYNC:
        z_bdbuf_wake (&bdbuf_cache.transfer_waiters);
        /* Fall through */
      case K_BDBUF_STATE_MODIFIED:
        z_bdbuf_group_release (cur);
        /* Fall through */
      case K_BDBUF_STATE_CACHED:
        sys_dlist_remove (&cur->link);
        sys_dlist_append (purge_list, &cur->link);
        break;
      case K_BDBUF_STATE_TRANSFER:
        z_bdbuf_set_state (cur, K_BDBUF_STATE_TRANSFER_PURGED);
        break;
      case K_BDBUF_STATE_ACCESS_CACHED:
      case K_BDBUF_STATE_ACCESS_EMPTY:
      case K_BDBUF_STATE_ACCESS_MODIFIED:
        z_bdbuf_set_state (cur, K_BDBUF_STATE_ACCESS_PURGED);
        break;
      default:
        z_bdbuf_fatal (K_BDBUF_FATAL_STATE_11);
      }
    }
    if (cur->avl.left != NULL) { /* Left */
      ++prev;
      *prev = cur;
      cur = cur->avl.left;
    } else if (cur->avl.right != NULL) { /* Right */
      ++prev;
      *prev = cur;
      cur = cur->avl.right;
    } else {
      while (*prev != NULL && 
             (cur == (*prev)->avl.right || (*prev)->avl.right == NULL)) {
        /* Up */
        cur = *prev;
        --prev;
      }
      if (*prev != NULL) 
        cur = (*prev)->avl.right; /* Right */
      else
        cur = NULL; /* Finished */
    }
  }
}

static void z_bdbuf_do_purge_dev(struct k_disk_device *dd) {
  sys_dlist_t purge_list;
  sys_dlist_init(&purge_list);
#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
  z_bdbuf_read_ahead_reset(dd);
#endif /* CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0 */
  z_bdbuf_gather_for_purge(&purge_list, dd);
  z_bdbuf_purge_list(&purge_list);
}

void k_bdbuf_purge_dev (struct k_disk_device *dd) {
  z_bdbuf_lock_cache();
  z_bdbuf_do_purge_dev(dd);
  z_bdbuf_unlock_cache();
}

int k_bdbuf_set_block_size(struct k_disk_device *dd,
  uint32_t block_size, bool sync) {
  int ret = 0;
  if (sync)
    k_bdbuf_syncdev(dd);
  z_bdbuf_lock_cache();
  if (block_size > 0) {
    size_t bds_per_group = z_bdbuf_bds_per_group(block_size);
    if (bds_per_group != 0) {
      int block_to_media_block_shift = 0;
      uint32_t media_blocks_per_block = block_size / dd->media_block_size;
      uint32_t one = 1;

      while ((one << block_to_media_block_shift) < media_blocks_per_block)
        ++block_to_media_block_shift;

      if ((dd->media_block_size << block_to_media_block_shift) != block_size)
        block_to_media_block_shift = -1;

      dd->block_size = block_size;
      dd->block_count = dd->size / media_blocks_per_block;
      dd->media_blocks_per_block = media_blocks_per_block;
      dd->block_to_media_block_shift = block_to_media_block_shift;
      dd->bds_per_group = bds_per_group;
      z_bdbuf_do_purge_dev(dd);
    } else {
      ret = -EINVAL;
    }
  } else {
    ret = -EINVAL;
  }
  z_bdbuf_unlock_cache ();
  return ret;
}

#if (CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0)
static void z_bdbuf_read_ahead_task(void *arg) {
  sys_dlist_t *chain = &bdbuf_cache.read_ahead_chain;

  while (bdbuf_cache.read_ahead_enabled) {
    sys_dnode_t *node;
    z_bdbuf_wait_for_event(&read_ahead_sync.read_ahead_sem);
    z_bdbuf_lock_cache ();

    while ((node = sys_dlist_get(chain)) != NULL) {
      struct k_disk_device *dd = CONTAINER_OF(node, struct k_disk_device, 
        read_ahead.node);
      struct k_bdbuf_buffer *bd；
      blkdev_bnum_t block = dd->read_ahead.next;
      blkdev_bnum_t media_block = 0;

      int ret = z_bdbuf_get_media_block(dd, block, &media_block);
      if (unlikely(ret != 0)) {
        dd->read_ahead.trigger = K_DISK_READ_AHEAD_NO_TRIGGER;
        continue;
      }
      bd = z_bdbuf_get_buffer_for_read_ahead(dd, media_block);
      if (bd != NULL) {
        uint32_t transfer_count = dd->read_ahead.nr_blocks;
        uint32_t blocks_until_end_of_disk = dd->block_count - block;
        uint32_t max_transfer_count = bdbuf_config.max_read_ahead_blocks;

        if (transfer_count == K_DISK_READ_AHEAD_SIZE_AUTO) {
          transfer_count = blocks_until_end_of_disk;
          if (transfer_count >= max_transfer_count) {
            transfer_count = max_transfer_count;
            dd->read_ahead.trigger = block + transfer_count / 2;
            dd->read_ahead.next = block + transfer_count;
          } else {
            dd->read_ahead.trigger = K_DISK_READ_AHEAD_NO_TRIGGER;
          }
        } else {
          if (transfer_count > blocks_until_end_of_disk)
            transfer_count = blocks_until_end_of_disk;
          if (transfer_count > max_transfer_count)
            transfer_count = max_transfer_count;
          ++dd->stats.read_ahead_peeks;
        }
        ++dd->stats.read_ahead_transfers;
        z_bdbuf_execute_read_request(dd, bd, transfer_count);
      }
    }
    z_bdbuf_unlock_cache ();
  }
  k_thread_abort(k_current_get());
}
#endif /* CONFIG_BDBUF_MAX_READ_AHEAD_BLOCKS > 0 */

void k_bdbuf_get_device_stats(const struct k_disk_device *dd,
  struct k_blkdev_stats *stats) {
  z_bdbuf_lock_cache();
  *stats = dd->stats;
  z_bdbuf_unlock_cache();
}

void k_bdbuf_reset_device_stats (struct k_disk_device *dd) {
  z_bdbuf_lock_cache();
  memset(&dd->stats, 0, sizeof(dd->stats));
  z_bdbuf_unlock_cache();
}