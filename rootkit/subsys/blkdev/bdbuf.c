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

/**
 * Set to 1 to enable debug tracing.
 */
#define CONFIG_BDBUF_TRACE 0

#include <kernel.h>
#include <sys/dlist.h>

#include "blkdev/bdbuf.h"

#define BDBUF_INVALID_DEV NULL


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

struct k_bdbuf_swapout_transfer {
	sys_dlist_t           bds;         /* The transfer list of BDs. */
	struct k_disk_device  *dd;         /* The device the transfer is for. */
	bool                  syncing;     /* The data is a sync'ing. */
	rtems_blkdev_request  write_req;   /* The write request. */
};

struct k_bdbuf_swapout_worker {
  struct k_sem            sync;
	struct k_thread         id;       /* The id of the task so we can wake it. */
	sys_dnode_t             link;     /* The threads sit on a chain when idle. */
	bool                    enabled;  /* The worker is enabled. */
	struct k_bdbuf_swapout_transfer transfer; /* The transfer data for this thread. */
};

struct k_bdbuf_waiters {
	unsigned         count;
	struct k_condvar cond_var;
};

struct k_bdbuf_cache {
	struct k_thread     *swapout;          /* Swapout task ID */
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
	rtems_id            sync_requester;    /* The sync requester. */
	struct k_disk_device  *sync_device;       /* The device to sync and BDBUF_INVALID_DEV not a device sync. */
	struct k_bdbuf_buffer* tree;              /* Buffer descriptor lookup AVL tree root. There is only one. */               
	sys_dlist_t lru;               /* Least recently used list */
	sys_dlist_t modified;          /* Modified buffers list */
	sys_dlist_t sync;              /* Buffers to sync list */
	struct k_bdbuf_waiters access_waiters;    /* Wait for a buffer in ACCESS_CACHED, ACCESS_MODIFIED or
	                                      	   ACCESS_EMPTY state. */                               
	struct k_bdbuf_waiters transfer_waiters;  /* Wait for a buffer in TRANSFER state. */                            
	struct k_bdbuf_waiters buffer_waiters;    /* Wait for a buffer and no one is available. */
	struct k_bdbuf_swapout_transfer *swapout_transfer;
	struct k_bdbuf_swapout_worker *swapout_workers;
	size_t              group_count;       /* The number of groups. */
	struct k_bdbuf_group*  groups;            /* The groups. */
	struct k_thread        read_ahead_task;   /* Read-ahead task */
	sys_dlist_t read_ahead_chain;  /* Read-ahead request chain */
	bool                read_ahead_enabled; /* Read-ahead enabled */
	rtems_status_code   init_status;       /* The initialization status */
	pthread_once_t      once;
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
	K_BDBUF_FATAL_WAIT_TRANS_EVNT
};

/**
 * The events used in this code. These should be system events rather than
 * application events.
 */
#define RTEMS_BDBUF_SWAPOUT_SYNC   RTEMS_EVENT_2
#define RTEMS_BDBUF_READ_AHEAD_WAKE_UP RTEMS_EVENT_1

static void bdbuf_swapout_task(void *arg);
static void bdbuf_read_ahead_task(void *arg);


static K_SEM_DEFINE(sync_swapout_sem, 0, 1);
static K_SEM_DEFINE(read_ahead_sem, 0, 1);
static const struct k_bdbuf_config bdbuf_config = {
  .max_read_ahead_blocks   = ,
  .max_write_blocks        = ,                                  
  .swapout_priority        = ,                                          
  .swapout_period          = ,                                         
  .swap_block_hold         = ,
  .swapout_workers         = ,     
  .swapout_worker_priority = ,                       
  .task_stack_size         = ,
  .size                    = ,                                      
  .buffer_min              = ,
  .buffer_max              = ,                           
  .read_ahead_priority     = 
};

/*
 * The Buffer Descriptor cache.
 */
static struct k_bdbuf_cache bdbuf_cache = {
  .lock = Z_MUTEX_INITIALIZER(bdbuf_cache),
  .sync_lock = Z_MUTEX_INITIALIZER(bdbuf_cache),
  .access_waiters = { 
    .cond_var = Z_CONDVAR_INITIALIZER(bdbuf_cache) 
  },
  .transfer_waiters = {
    .cond_var = Z_CONDVAR_INITIALIZER(bdbuf_cache)
  },
  .buffer_waiters = { 
    .cond_var = Z_CONDVAR_INITIALIZER(bdbuf_cache) 
  },
  .once = PTHREAD_ONCE_INIT
};



#if defined(CONFIG_BDBUF_TRACE)
bool rtems_bdbuf_tracer;

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
  printf ("bdbuf:group users=%lu", total);
  val = k_bdbuf_list_count(&bdbuf_cache.lru);
  printf(", lru=%lu", val);
  total = val;
  val = k_bdbuf_list_count(&bdbuf_cache.modified);
  printf(", mod=%lu", val);
  total += val;
  val = k_bdbuf_list_count(&bdbuf_cache.sync);
  printf(", sync=%lu", val);
  total += val;
  printf(", total=%lu\n", total);
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
#define k_bdbuf_tracer (0)
#define k_bdbuf_show_usage() ((void) 0)
#define k_bdbuf_show_users(_w, _b) ((void) 0)
#endif /* CONFIG_BDBUF_TRACE */

/**
 * The default maximum height of 32 allows for AVL trees having between
 * 5,704,880 and 4,294,967,295 nodes, depending on order of insertion.  You may
 * change this compile-time constant as you wish.
 */
#ifndef RTEMS_BDBUF_AVL_MAX_HEIGHT
#define RTEMS_BDBUF_AVL_MAX_HEIGHT (32)
#endif

static void z_bdbuf_fatal (rtems_fatal_code error) {
  z_except_reason(error);
}

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
  struct k_bdbuf_buffer*  buf_stack[RTEMS_BDBUF_AVL_MAX_HEIGHT];
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
  struct k_bdbuf_buffer*  buf_stack[RTEMS_BDBUF_AVL_MAX_HEIGHT];
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
  z_bdbuf_unlock (&bdbuf_cache.lock);
}

static void z_bdbuf_lock_sync(void) {
  z_bdbuf_lock (&bdbuf_cache.sync_lock);
}

static void z_bdbuf_unlock_sync(void) {
  z_bdbuf_unlock (&bdbuf_cache.sync_lock);
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

//TODO:
static void z_bdbuf_wake_swapper(void) {
  // rtems_status_code sc = rtems_event_send (bdbuf_cache.swapout,
  //                                          RTEMS_BDBUF_SWAPOUT_SYNC);
  // if (sc != RTEMS_SUCCESSFUL)
  //   z_bdbuf_fatal (K_BDBUF_FATAL_SO_WAKE_1);
  //k_sem_give(bdbuf_cache.swapout);
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
  case RTEMS_BDBUF_STATE_FREE:
    break;
  case RTEMS_BDBUF_STATE_CACHED:
    z_bdbuf_remove_from_tree(bd);
    break;
  default:
    z_bdbuf_fatal_with_state(bd->state, K_BDBUF_FATAL_STATE_10);
  }
  sys_dlist_remove(&bd->link);
}

static void z_bdbuf_make_free_and_add_to_lru_list(struct k_bdbuf_buffer *bd) {
  z_bdbuf_set_state(bd, RTEMS_BDBUF_STATE_FREE);
  sys_dlist_prepend(&bdbuf_cache.lru, &bd->link);
}

static void z_bdbuf_make_empty(struct k_bdbuf_buffer *bd) {
  z_bdbuf_set_state(bd, RTEMS_BDBUF_STATE_EMPTY);
}

static void z_bdbuf_make_cached_and_add_to_lru_list(struct k_bdbuf_buffer *bd) {
  z_bdbuf_set_state(bd, RTEMS_BDBUF_STATE_CACHED);
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
  if (bd->state == RTEMS_BDBUF_STATE_ACCESS_CACHED
        || bd->state == RTEMS_BDBUF_STATE_ACCESS_EMPTY)
    bd->hold_timer = bdbuf_config.swap_block_hold;
  z_bdbuf_set_state(bd, RTEMS_BDBUF_STATE_MODIFIED);
  sys_dlist_append(&bdbuf_cache.modified, &bd->link);
  if (bd->waiters)
    z_bdbuf_wake(&bdbuf_cache.access_waiters);
  else if (z_bdbuf_has_buffer_waiters())
    z_bdbuf_wake_swapper();
}

static void z_bdbuf_add_to_lru_list_after_access (struct k_bdbuf_buffer *bd) {
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
  if (rtems_bdbuf_tracer)
    printf ("bdbuf:realloc: %tu: %zd -> %zd\n",
            group - bdbuf_cache.groups, group->bds_per_group,
            new_bds_per_group);

  bufs_per_bd = bdbuf_cache.max_bds_per_group / group->bds_per_group;
  for (b = 0, bd = group->bdbuf;
       b < group->bds_per_group;
       b++, bd += bufs_per_bd)
    z_bdbuf_remove_from_tree_and_lru_list (bd);

  group->bds_per_group = new_bds_per_group;
  bufs_per_bd = bdbuf_cache.max_bds_per_group / new_bds_per_group;
  for (b = 1, bd = group->bdbuf + bufs_per_bd;
       b < group->bds_per_group;
       b++, bd += bufs_per_bd)
    z_bdbuf_make_free_and_add_to_lru_list (bd);
  if (b > 1)
    z_bdbuf_wake (&bdbuf_cache.buffer_waiters);
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
  SYS_DLIST_FOR_EACH_NODE(&bdbuf_cache.lru, node) {
    struct k_bdbuf_buffer *bd = (struct k_bdbuf_buffer *)node;
    struct k_bdbuf_buffer *empty_bd = NULL;
    if (rtems_bdbuf_tracer) {
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

static rtems_status_code
rtems_bdbuf_create_task(
  rtems_name name,
  rtems_task_priority priority,
  rtems_task_priority default_priority,
  rtems_id *id
)
{
  rtems_status_code sc;
  size_t stack_size = bdbuf_config.task_stack_size ?
    bdbuf_config.task_stack_size : RTEMS_BDBUF_TASK_STACK_SIZE_DEFAULT;

  priority = priority != 0 ? priority : default_priority;

  sc = rtems_task_create (name,
                          priority,
                          stack_size,
                          RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
                          RTEMS_LOCAL | RTEMS_NO_FLOATING_POINT,
                          id);

  return sc;
}




static struct k_bdbuf_swapout_transfer *z_bdbuf_swapout_transfer_alloc(void) {
  /*
   * @note chrisj The rtems_blkdev_request and the array at the end is a hack.
   * I am disappointment at finding code like this in RTEMS. The request should
   * have been a sys_dlist_t. Simple, fast and less storage as the node
   * is already part of the buffer structure.
   */
  size_t transfer_size = sizeof(struct k_bdbuf_swapout_transfer)
    + (bdbuf_config.max_write_blocks * sizeof (rtems_blkdev_sg_buffer));
  return k_calloc(1, transfer_size);
}

static void
rtems_bdbuf_transfer_done(rtems_blkdev_request* req, rtems_status_code status);

static void
rtems_bdbuf_swapout_transfer_init(struct k_bdbuf_swapout_transfer* transfer,
                                   rtems_id id)
{
  rtems_chain_initialize_empty (&transfer->bds);
  transfer->dd = BDBUF_INVALID_DEV;
  transfer->syncing = false;
  transfer->write_req.req = RTEMS_BLKDEV_REQ_WRITE;
  transfer->write_req.done = rtems_bdbuf_transfer_done;
  transfer->write_req.io_task = id;
}

static size_t rtems_bdbuf_swapout_worker_size(void) {
  return sizeof(struct k_bdbuf_swapout_worker) +
    (bdbuf_config.max_write_blocks * sizeof(rtems_blkdev_sg_buffer));
}

static void
bdbuf_swapout_worker_task (rtems_task_argument arg);

static rtems_status_code
rtems_bdbuf_swapout_workers_create (void)
{
  rtems_status_code  sc;
  size_t             w;
  size_t             worker_size;
  char              *worker_current;

  worker_size = rtems_bdbuf_swapout_worker_size ();
  worker_current = calloc (1, bdbuf_config.swapout_workers * worker_size);
  sc = worker_current != NULL ? RTEMS_SUCCESSFUL : RTEMS_NO_MEMORY;

  bdbuf_cache.swapout_workers = (struct k_bdbuf_swapout_worker *) worker_current;

  for (w = 0;
       sc == RTEMS_SUCCESSFUL && w < bdbuf_config.swapout_workers;
       w++, worker_current += worker_size)
  {
    struct k_bdbuf_swapout_worker *worker = (struct k_bdbuf_swapout_worker *) worker_current;

    sc = rtems_bdbuf_create_task (rtems_build_name('B', 'D', 'o', 'a' + w),
                                  bdbuf_config.swapout_worker_priority,
                                  RTEMS_BDBUF_SWAPOUT_WORKER_TASK_PRIORITY_DEFAULT,
                                  &worker->id);
    if (sc == RTEMS_SUCCESSFUL)
    {
      rtems_bdbuf_swapout_transfer_init (&worker->transfer, worker->id);

      sys_dlist_append (&bdbuf_cache.swapout_free_workers, &worker->link);
      worker->enabled = true;

      sc = rtems_task_start (worker->id,
                             bdbuf_swapout_worker_task,
                             (rtems_task_argument) worker);
    }
  }

  return sc;
}

static size_t
rtems_bdbuf_read_request_size (uint32_t transfer_count)
{
  return sizeof (rtems_blkdev_request)
    + sizeof (rtems_blkdev_sg_buffer) * transfer_count;
}

static rtems_status_code
rtems_bdbuf_do_init (void)
{
  struct k_bdbuf_group*  group;
  struct k_bdbuf_buffer* bd;
  uint8_t*            buffer;
  size_t              b;
  rtems_status_code   sc;

  if (rtems_bdbuf_tracer)
    printf ("bdbuf:init\n");

  if (rtems_interrupt_is_in_progress())
    return RTEMS_CALLED_FROM_ISR;

  /*
   * Check the configuration table values.
   */

  if ((bdbuf_config.buffer_max % bdbuf_config.buffer_min) != 0)
    return RTEMS_INVALID_NUMBER;

  if (rtems_bdbuf_read_request_size (bdbuf_config.max_read_ahead_blocks)
      > RTEMS_MINIMUM_STACK_SIZE / 8U)
    return RTEMS_INVALID_NUMBER;

  bdbuf_cache.sync_device = BDBUF_INVALID_DEV;

  rtems_chain_initialize_empty (&bdbuf_cache.swapout_free_workers);
  rtems_chain_initialize_empty (&bdbuf_cache.lru);
  rtems_chain_initialize_empty (&bdbuf_cache.modified);
  rtems_chain_initialize_empty (&bdbuf_cache.sync);
  rtems_chain_initialize_empty (&bdbuf_cache.read_ahead_chain);

  rtems_mutex_set_name (&bdbuf_cache.lock, "bdbuf lock");
  rtems_mutex_set_name (&bdbuf_cache.sync_lock, "bdbuf sync lock");
  rtems_condition_variable_set_name (&bdbuf_cache.access_waiters.cond_var,
                                     "bdbuf access");
  rtems_condition_variable_set_name (&bdbuf_cache.transfer_waiters.cond_var,
                                     "bdbuf transfer");
  rtems_condition_variable_set_name (&bdbuf_cache.buffer_waiters.cond_var,
                                     "bdbuf buffer");

  z_bdbuf_lock_cache ();

  /*
   * Compute the various number of elements in the cache.
   */
  bdbuf_cache.buffer_min_count =
    bdbuf_config.size / bdbuf_config.buffer_min;
  bdbuf_cache.max_bds_per_group =
    bdbuf_config.buffer_max / bdbuf_config.buffer_min;
  bdbuf_cache.group_count =
    bdbuf_cache.buffer_min_count / bdbuf_cache.max_bds_per_group;

  /*
   * Allocate the memory for the buffer descriptors.
   */
  bdbuf_cache.bds = calloc (sizeof (struct k_bdbuf_buffer),
                            bdbuf_cache.buffer_min_count);
  if (!bdbuf_cache.bds)
    goto error;

  /*
   * Allocate the memory for the buffer descriptors.
   */
  bdbuf_cache.groups = calloc (sizeof (struct k_bdbuf_group),
                               bdbuf_cache.group_count);
  if (!bdbuf_cache.groups)
    goto error;

  /*
   * Allocate memory for buffer memory. The buffer memory will be cache
   * aligned. It is possible to k_free the memory allocated by
   * rtems_cache_aligned_malloc() with k_free().
   */
  bdbuf_cache.buffers = rtems_cache_aligned_malloc(bdbuf_cache.buffer_min_count
                                                   * bdbuf_config.buffer_min);
  if (bdbuf_cache.buffers == NULL)
    goto error;

  /*
   * The cache is empty after opening so we need to add all the buffers to it
   * and initialise the groups.
   */
  for (b = 0, group = bdbuf_cache.groups,
         bd = bdbuf_cache.bds, buffer = bdbuf_cache.buffers;
       b < bdbuf_cache.buffer_min_count;
       b++, bd++, buffer += bdbuf_config.buffer_min)
  {
    bd->dd    = BDBUF_INVALID_DEV;
    bd->group  = group;
    bd->buffer = buffer;

    sys_dlist_append (&bdbuf_cache.lru, &bd->link);

    if ((b % bdbuf_cache.max_bds_per_group) ==
        (bdbuf_cache.max_bds_per_group - 1))
      group++;
  }

  for (b = 0,
         group = bdbuf_cache.groups,
         bd = bdbuf_cache.bds;
       b < bdbuf_cache.group_count;
       b++,
         group++,
         bd += bdbuf_cache.max_bds_per_group)
  {
    group->bds_per_group = bdbuf_cache.max_bds_per_group;
    group->bdbuf = bd;
  }

  /*
   * Create and start swapout task.
   */

  bdbuf_cache.swapout_transfer = z_bdbuf_swapout_transfer_alloc ();
  if (!bdbuf_cache.swapout_transfer)
    goto error;

  bdbuf_cache.swapout_enabled = true;

  sc = rtems_bdbuf_create_task (rtems_build_name('B', 'S', 'W', 'P'),
                                bdbuf_config.swapout_priority,
                                RTEMS_BDBUF_SWAPOUT_TASK_PRIORITY_DEFAULT,
                                &bdbuf_cache.swapout);
  if (sc != RTEMS_SUCCESSFUL)
    goto error;

  rtems_bdbuf_swapout_transfer_init (bdbuf_cache.swapout_transfer, bdbuf_cache.swapout);

  sc = rtems_task_start (bdbuf_cache.swapout,
                         bdbuf_swapout_task,
                         (rtems_task_argument) bdbuf_cache.swapout_transfer);
  if (sc != RTEMS_SUCCESSFUL)
    goto error;

  if (bdbuf_config.swapout_workers > 0)
  {
    sc = rtems_bdbuf_swapout_workers_create ();
    if (sc != RTEMS_SUCCESSFUL)
      goto error;
  }

  if (bdbuf_config.max_read_ahead_blocks > 0)
  {
    bdbuf_cache.read_ahead_enabled = true;
    sc = rtems_bdbuf_create_task (rtems_build_name('B', 'R', 'D', 'A'),
                                  bdbuf_config.read_ahead_priority,
                                  RTEMS_BDBUF_READ_AHEAD_TASK_PRIORITY_DEFAULT,
                                  &bdbuf_cache.read_ahead_task);
    if (sc != RTEMS_SUCCESSFUL)
      goto error;

    sc = rtems_task_start (bdbuf_cache.read_ahead_task,
                           bdbuf_read_ahead_task,
                           0);
    if (sc != RTEMS_SUCCESSFUL)
      goto error;
  }

  z_bdbuf_unlock_cache ();

  return RTEMS_SUCCESSFUL;

error:

  if (bdbuf_cache.read_ahead_task != 0)
    rtems_task_delete (bdbuf_cache.read_ahead_task);

  if (bdbuf_cache.swapout != 0)
    rtems_task_delete (bdbuf_cache.swapout);

  if (bdbuf_cache.swapout_workers)
  {
    char   *worker_current = (char *) bdbuf_cache.swapout_workers;
    size_t  worker_size = rtems_bdbuf_swapout_worker_size ();
    size_t  w;

    for (w = 0;
         w < bdbuf_config.swapout_workers;
         w++, worker_current += worker_size)
    {
      struct k_bdbuf_swapout_worker *worker = (struct k_bdbuf_swapout_worker *) worker_current;

      if (worker->id != 0) {
        rtems_task_delete (worker->id);
      }
    }
  }

  k_free (bdbuf_cache.buffers);
  k_free (bdbuf_cache.groups);
  k_free (bdbuf_cache.bds);
  k_free (bdbuf_cache.swapout_transfer);
  k_free (bdbuf_cache.swapout_workers);

  z_bdbuf_unlock_cache ();

  return RTEMS_UNSATISFIED;
}

static void
rtems_bdbuf_init_once (void)
{
  bdbuf_cache.init_status = rtems_bdbuf_do_init();
}

rtems_status_code
rtems_bdbuf_init (void)
{
  int eno;

  eno = pthread_once (&bdbuf_cache.once, rtems_bdbuf_init_once);
  _Assert (eno == 0);
  (void) eno;

  return bdbuf_cache.init_status;
}


static void z_bdbuf_wait_for_event(struct k_sem *sem) {
  int ret = k_sem_take(sem, K_FOREVER);
  if (ret)
    z_bdbuf_fatal (K_BDBUF_FATAL_WAIT_EVNT);
}

static void z_bdbuf_wait_for_transient_event(struct k_sem *sem)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;

  sc = rtems_event_transient_receive (RTEMS_WAIT, RTEMS_NO_TIMEOUT);
  if (sc != RTEMS_SUCCESSFUL)
    z_bdbuf_fatal (K_BDBUF_FATAL_WAIT_TRANS_EVNT);
}

static void
rtems_bdbuf_wait_for_access (struct k_bdbuf_buffer *bd)
{
  while (true)
  {
    switch (bd->state)
    {
      case RTEMS_BDBUF_STATE_MODIFIED:
        z_bdbuf_group_release (bd);
        /* Fall through */
      case RTEMS_BDBUF_STATE_CACHED:
        rtems_chain_extract_unprotected (&bd->link);
        /* Fall through */
      case RTEMS_BDBUF_STATE_EMPTY:
        return;
      case RTEMS_BDBUF_STATE_ACCESS_CACHED:
      case RTEMS_BDBUF_STATE_ACCESS_EMPTY:
      case RTEMS_BDBUF_STATE_ACCESS_MODIFIED:
      case RTEMS_BDBUF_STATE_ACCESS_PURGED:
        z_bdbuf_wait (bd, &bdbuf_cache.access_waiters);
        break;
      case RTEMS_BDBUF_STATE_SYNC:
      case RTEMS_BDBUF_STATE_TRANSFER:
      case RTEMS_BDBUF_STATE_TRANSFER_PURGED:
        z_bdbuf_wait (bd, &bdbuf_cache.transfer_waiters);
        break;
      default:
        z_bdbuf_fatal_with_state (bd->state, K_BDBUF_FATAL_STATE_7);
    }
  }
}

static void
rtems_bdbuf_request_sync_for_modified_buffer (struct k_bdbuf_buffer *bd)
{
  z_bdbuf_set_state (bd, RTEMS_BDBUF_STATE_SYNC);
  rtems_chain_extract_unprotected (&bd->link);
  sys_dlist_append (&bdbuf_cache.sync, &bd->link);
  z_bdbuf_wake_swapper ();
}

/**
 * @brief Waits until the buffer is ready for recycling.
 *
 * @retval @c true Buffer is valid and may be recycled.
 * @retval @c false Buffer is invalid and has to searched again.
 */
static bool
rtems_bdbuf_wait_for_recycle (struct k_bdbuf_buffer *bd)
{
  while (true)
  {
    switch (bd->state)
    {
      case RTEMS_BDBUF_STATE_FREE:
        return true;
      case RTEMS_BDBUF_STATE_MODIFIED:
        rtems_bdbuf_request_sync_for_modified_buffer (bd);
        break;
      case RTEMS_BDBUF_STATE_CACHED:
      case RTEMS_BDBUF_STATE_EMPTY:
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
          z_bdbuf_anonymous_wait (&bdbuf_cache.buffer_waiters);
          return false;
        }
      case RTEMS_BDBUF_STATE_ACCESS_CACHED:
      case RTEMS_BDBUF_STATE_ACCESS_EMPTY:
      case RTEMS_BDBUF_STATE_ACCESS_MODIFIED:
      case RTEMS_BDBUF_STATE_ACCESS_PURGED:
        z_bdbuf_wait (bd, &bdbuf_cache.access_waiters);
        break;
      case RTEMS_BDBUF_STATE_SYNC:
      case RTEMS_BDBUF_STATE_TRANSFER:
      case RTEMS_BDBUF_STATE_TRANSFER_PURGED:
        z_bdbuf_wait (bd, &bdbuf_cache.transfer_waiters);
        break;
      default:
        z_bdbuf_fatal_with_state (bd->state, K_BDBUF_FATAL_STATE_8);
    }
  }
}

static void
rtems_bdbuf_wait_for_sync_done (struct k_bdbuf_buffer *bd)
{
  while (true)
  {
    switch (bd->state)
    {
      case RTEMS_BDBUF_STATE_CACHED:
      case RTEMS_BDBUF_STATE_EMPTY:
      case RTEMS_BDBUF_STATE_MODIFIED:
      case RTEMS_BDBUF_STATE_ACCESS_CACHED:
      case RTEMS_BDBUF_STATE_ACCESS_EMPTY:
      case RTEMS_BDBUF_STATE_ACCESS_MODIFIED:
      case RTEMS_BDBUF_STATE_ACCESS_PURGED:
        return;
      case RTEMS_BDBUF_STATE_SYNC:
      case RTEMS_BDBUF_STATE_TRANSFER:
      case RTEMS_BDBUF_STATE_TRANSFER_PURGED:
        z_bdbuf_wait (bd, &bdbuf_cache.transfer_waiters);
        break;
      default:
        z_bdbuf_fatal_with_state (bd->state, K_BDBUF_FATAL_STATE_9);
    }
  }
}

static void
rtems_bdbuf_wait_for_buffer (void)
{
  if (!rtems_chain_is_empty (&bdbuf_cache.modified))
    z_bdbuf_wake_swapper ();

  z_bdbuf_anonymous_wait (&bdbuf_cache.buffer_waiters);
}

static void
rtems_bdbuf_sync_after_access (struct k_bdbuf_buffer *bd)
{
  z_bdbuf_set_state (bd, RTEMS_BDBUF_STATE_SYNC);

  sys_dlist_append (&bdbuf_cache.sync, &bd->link);

  if (bd->waiters)
    z_bdbuf_wake (&bdbuf_cache.access_waiters);

  z_bdbuf_wake_swapper ();
  rtems_bdbuf_wait_for_sync_done (bd);

  /*
   * We may have created a cached or empty buffer which may be recycled.
   */
  if (bd->waiters == 0
        && (bd->state == RTEMS_BDBUF_STATE_CACHED
          || bd->state == RTEMS_BDBUF_STATE_EMPTY))
  {
    if (bd->state == RTEMS_BDBUF_STATE_EMPTY)
    {
      z_bdbuf_remove_from_tree (bd);
      z_bdbuf_make_free_and_add_to_lru_list (bd);
    }
    z_bdbuf_wake (&bdbuf_cache.buffer_waiters);
  }
}

static struct k_bdbuf_buffer *
rtems_bdbuf_get_buffer_for_read_ahead (struct k_disk_device *dd,
                                       blkdev_bnum_t  block)
{
  struct k_bdbuf_buffer *bd = NULL;

  bd = z_bdbuf_avl_search (&bdbuf_cache.tree, dd, block);

  if (bd == NULL)
  {
    bd = z_bdbuf_get_buffer_from_lru_list (dd, block);

    if (bd != NULL)
      z_bdbuf_group_obtain (bd);
  }
  else
    /*
     * The buffer is in the cache.  So it is already available or in use, and
     * thus no need for a read ahead.
     */
    bd = NULL;

  return bd;
}

static struct k_bdbuf_buffer *
rtems_bdbuf_get_buffer_for_access (struct k_disk_device *dd,
                                   blkdev_bnum_t  block)
{
  struct k_bdbuf_buffer *bd = NULL;

  do
  {
    bd = z_bdbuf_avl_search (&bdbuf_cache.tree, dd, block);

    if (bd != NULL)
    {
      if (bd->group->bds_per_group != dd->bds_per_group)
      {
        if (rtems_bdbuf_wait_for_recycle (bd))
        {
          z_bdbuf_remove_from_tree_and_lru_list (bd);
          z_bdbuf_make_free_and_add_to_lru_list (bd);
          z_bdbuf_wake (&bdbuf_cache.buffer_waiters);
        }
        bd = NULL;
      }
    }
    else
    {
      bd = z_bdbuf_get_buffer_from_lru_list (dd, block);

      if (bd == NULL)
        rtems_bdbuf_wait_for_buffer ();
    }
  }
  while (bd == NULL);

  rtems_bdbuf_wait_for_access (bd);
  z_bdbuf_group_obtain (bd);

  return bd;
}

static rtems_status_code
rtems_bdbuf_get_media_block (const struct k_disk_device *dd,
                             blkdev_bnum_t        block,
                             blkdev_bnum_t       *media_block_ptr)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;

  if (block < dd->block_count)
  {
    /*
     * Compute the media block number. Drivers work with media block number not
     * the block number a BD may have as this depends on the block size set by
     * the user.
     */
    *media_block_ptr = z_bdbuf_media_block (dd, block) + dd->start;
  }
  else
  {
    sc = RTEMS_INVALID_ID;
  }

  return sc;
}

rtems_status_code
rtems_bdbuf_get (struct k_disk_device   *dd,
                 blkdev_bnum_t    block,
                 struct k_bdbuf_buffer **bd_ptr)
{
  rtems_status_code   sc = RTEMS_SUCCESSFUL;
  struct k_bdbuf_buffer *bd = NULL;
  blkdev_bnum_t   media_block;

  z_bdbuf_lock_cache ();

  sc = rtems_bdbuf_get_media_block (dd, block, &media_block);
  if (sc == RTEMS_SUCCESSFUL)
  {
    /*
     * Print the block index relative to the physical disk.
     */
    if (rtems_bdbuf_tracer)
      printf ("bdbuf:get: %" PRIu32 " (%" PRIu32 ") (dev = %08x)\n",
              media_block, block, (unsigned) dd->dev);

    bd = rtems_bdbuf_get_buffer_for_access (dd, media_block);

    switch (bd->state)
    {
      case RTEMS_BDBUF_STATE_CACHED:
        z_bdbuf_set_state (bd, RTEMS_BDBUF_STATE_ACCESS_CACHED);
        break;
      case RTEMS_BDBUF_STATE_EMPTY:
        z_bdbuf_set_state (bd, RTEMS_BDBUF_STATE_ACCESS_EMPTY);
        break;
      case RTEMS_BDBUF_STATE_MODIFIED:
        /*
         * To get a modified buffer could be considered a bug in the caller
         * because you should not be getting an already modified buffer but
         * user may have modified a byte in a block then decided to seek the
         * start and write the whole block and the file system will have no
         * record of this so just gets the block to fill.
         */
        z_bdbuf_set_state (bd, RTEMS_BDBUF_STATE_ACCESS_MODIFIED);
        break;
      default:
        z_bdbuf_fatal_with_state (bd->state, K_BDBUF_FATAL_STATE_2);
        break;
    }

    if (rtems_bdbuf_tracer)
    {
      k_bdbuf_show_users ("get", bd);
      k_bdbuf_show_usage ();
    }
  }

  z_bdbuf_unlock_cache ();

  *bd_ptr = bd;

  return sc;
}

/**
 * Call back handler called by the low level driver when the transfer has
 * completed. This function may be invoked from interrupt handler.
 *
 * @param arg Arbitrary argument specified in block device request
 *            structure (in this case - pointer to the appropriate
 *            block device request structure).
 * @param status I/O completion status
 */
static void
rtems_bdbuf_transfer_done (rtems_blkdev_request* req, rtems_status_code status)
{
  req->status = status;

  rtems_event_transient_send (req->io_task);
}

static rtems_status_code
rtems_bdbuf_execute_transfer_request (struct k_disk_device    *dd,
                                      rtems_blkdev_request *req,
                                      bool                  cache_locked)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;
  uint32_t transfer_index = 0;
  bool wake_transfer_waiters = false;
  bool wake_buffer_waiters = false;

  if (cache_locked)
    z_bdbuf_unlock_cache ();

  /* The return value will be ignored for transfer requests */
  dd->ioctl (dd->phys_dev, RTEMS_BLKIO_REQUEST, req);

  /* Wait for transfer request completion */
  z_bdbuf_wait_for_transient_event ();
  sc = req->status;

  z_bdbuf_lock_cache ();

  /* Statistics */
  if (req->req == RTEMS_BLKDEV_REQ_READ)
  {
    dd->stats.read_blocks += req->bufnum;
    if (sc != RTEMS_SUCCESSFUL)
      ++dd->stats.read_errors;
  }
  else
  {
    dd->stats.write_blocks += req->bufnum;
    ++dd->stats.write_transfers;
    if (sc != RTEMS_SUCCESSFUL)
      ++dd->stats.write_errors;
  }

  for (transfer_index = 0; transfer_index < req->bufnum; ++transfer_index)
  {
    struct k_bdbuf_buffer *bd = req->bufs [transfer_index].user;
    bool waiters = bd->waiters;

    if (waiters)
      wake_transfer_waiters = true;
    else
      wake_buffer_waiters = true;

    z_bdbuf_group_release (bd);

    if (sc == RTEMS_SUCCESSFUL && bd->state == RTEMS_BDBUF_STATE_TRANSFER)
      z_bdbuf_make_cached_and_add_to_lru_list (bd);
    else
      z_bdbuf_discard_buffer (bd);

    if (rtems_bdbuf_tracer)
      k_bdbuf_show_users ("transfer", bd);
  }

  if (wake_transfer_waiters)
    z_bdbuf_wake (&bdbuf_cache.transfer_waiters);

  if (wake_buffer_waiters)
    z_bdbuf_wake (&bdbuf_cache.buffer_waiters);

  if (!cache_locked)
    z_bdbuf_unlock_cache ();

  if (sc == RTEMS_SUCCESSFUL || sc == RTEMS_UNSATISFIED)
    return sc;
  else
    return RTEMS_IO_ERROR;
}

static rtems_status_code
rtems_bdbuf_execute_read_request (struct k_disk_device  *dd,
                                  struct k_bdbuf_buffer *bd,
                                  uint32_t            transfer_count)
{
  rtems_blkdev_request *req = NULL;
  blkdev_bnum_t media_block = bd->block;
  uint32_t media_blocks_per_block = dd->media_blocks_per_block;
  uint32_t block_size = dd->block_size;
  uint32_t transfer_index = 1;

  /*
   * TODO: This type of request structure is wrong and should be removed.
   */
#define bdbuf_alloc(size) __builtin_alloca (size)

  req = bdbuf_alloc (rtems_bdbuf_read_request_size (transfer_count));

  req->req = RTEMS_BLKDEV_REQ_READ;
  req->done = rtems_bdbuf_transfer_done;
  req->io_task = rtems_task_self ();
  req->bufnum = 0;

  z_bdbuf_set_state (bd, RTEMS_BDBUF_STATE_TRANSFER);

  req->bufs [0].user   = bd;
  req->bufs [0].block  = media_block;
  req->bufs [0].length = block_size;
  req->bufs [0].buffer = bd->buffer;

  if (rtems_bdbuf_tracer)
    k_bdbuf_show_users ("read", bd);

  while (transfer_index < transfer_count)
  {
    media_block += media_blocks_per_block;

    bd = rtems_bdbuf_get_buffer_for_read_ahead (dd, media_block);

    if (bd == NULL)
      break;

    z_bdbuf_set_state (bd, RTEMS_BDBUF_STATE_TRANSFER);

    req->bufs [transfer_index].user   = bd;
    req->bufs [transfer_index].block  = media_block;
    req->bufs [transfer_index].length = block_size;
    req->bufs [transfer_index].buffer = bd->buffer;

    if (rtems_bdbuf_tracer)
      k_bdbuf_show_users ("read", bd);

    ++transfer_index;
  }

  req->bufnum = transfer_index;

  return rtems_bdbuf_execute_transfer_request (dd, req, true);
}

static bool
rtems_bdbuf_is_read_ahead_active (const struct k_disk_device *dd)
{
  return !rtems_chain_is_node_off_chain (&dd->read_ahead.node);
}

static void
rtems_bdbuf_read_ahead_cancel (struct k_disk_device *dd)
{
  if (rtems_bdbuf_is_read_ahead_active (dd))
  {
    rtems_chain_extract_unprotected (&dd->read_ahead.node);
    rtems_chain_set_off_chain (&dd->read_ahead.node);
  }
}

static void
rtems_bdbuf_read_ahead_reset (struct k_disk_device *dd)
{
  rtems_bdbuf_read_ahead_cancel (dd);
  dd->read_ahead.trigger = RTEMS_DISK_READ_AHEAD_NO_TRIGGER;
}

static void
rtems_bdbuf_read_ahead_add_to_chain (struct k_disk_device *dd)
{
  rtems_status_code sc;
  sys_dlist_t *chain = &bdbuf_cache.read_ahead_chain;

  if (rtems_chain_is_empty (chain))
  {
    sc = rtems_event_send (bdbuf_cache.read_ahead_task,
                           RTEMS_BDBUF_READ_AHEAD_WAKE_UP);
    if (sc != RTEMS_SUCCESSFUL)
      z_bdbuf_fatal (K_BDBUF_FATAL_RA_WAKE_UP);
  }

  sys_dlist_append (chain, &dd->read_ahead.node);
}

static void
rtems_bdbuf_check_read_ahead_trigger (struct k_disk_device *dd,
                                      blkdev_bnum_t  block)
{
  if (bdbuf_cache.read_ahead_task != 0
      && dd->read_ahead.trigger == block
      && !rtems_bdbuf_is_read_ahead_active (dd))
  {
    dd->read_ahead.nr_blocks = RTEMS_DISK_READ_AHEAD_SIZE_AUTO;
    rtems_bdbuf_read_ahead_add_to_chain(dd);
  }
}

static void
rtems_bdbuf_set_read_ahead_trigger (struct k_disk_device *dd,
                                    blkdev_bnum_t  block)
{
  if (dd->read_ahead.trigger != block)
  {
    rtems_bdbuf_read_ahead_cancel (dd);
    dd->read_ahead.trigger = block + 1;
    dd->read_ahead.next = block + 2;
  }
}

rtems_status_code
rtems_bdbuf_read (struct k_disk_device   *dd,
                  blkdev_bnum_t    block,
                  struct k_bdbuf_buffer **bd_ptr)
{
  rtems_status_code     sc = RTEMS_SUCCESSFUL;
  struct k_bdbuf_buffer   *bd = NULL;
  blkdev_bnum_t     media_block;

  z_bdbuf_lock_cache ();

  sc = rtems_bdbuf_get_media_block (dd, block, &media_block);
  if (sc == RTEMS_SUCCESSFUL)
  {
    if (rtems_bdbuf_tracer)
      printf ("bdbuf:read: %" PRIu32 " (%" PRIu32 ") (dev = %08x)\n",
              media_block, block, (unsigned) dd->dev);

    bd = rtems_bdbuf_get_buffer_for_access (dd, media_block);
    switch (bd->state)
    {
      case RTEMS_BDBUF_STATE_CACHED:
        ++dd->stats.read_hits;
        z_bdbuf_set_state (bd, RTEMS_BDBUF_STATE_ACCESS_CACHED);
        break;
      case RTEMS_BDBUF_STATE_MODIFIED:
        ++dd->stats.read_hits;
        z_bdbuf_set_state (bd, RTEMS_BDBUF_STATE_ACCESS_MODIFIED);
        break;
      case RTEMS_BDBUF_STATE_EMPTY:
        ++dd->stats.read_misses;
        rtems_bdbuf_set_read_ahead_trigger (dd, block);
        sc = rtems_bdbuf_execute_read_request (dd, bd, 1);
        if (sc == RTEMS_SUCCESSFUL)
        {
          z_bdbuf_set_state (bd, RTEMS_BDBUF_STATE_ACCESS_CACHED);
          rtems_chain_extract_unprotected (&bd->link);
          z_bdbuf_group_obtain (bd);
        }
        else
        {
          bd = NULL;
        }
        break;
      default:
        z_bdbuf_fatal_with_state (bd->state, K_BDBUF_FATAL_STATE_4);
        break;
    }

    rtems_bdbuf_check_read_ahead_trigger (dd, block);
  }

  z_bdbuf_unlock_cache ();

  *bd_ptr = bd;

  return sc;
}

void
rtems_bdbuf_peek (struct k_disk_device *dd,
                  blkdev_bnum_t block,
                  uint32_t nr_blocks)
{
  z_bdbuf_lock_cache ();

  if (bdbuf_cache.read_ahead_enabled && nr_blocks > 0)
  {
    rtems_bdbuf_read_ahead_reset(dd);
    dd->read_ahead.next = block;
    dd->read_ahead.nr_blocks = nr_blocks;
    rtems_bdbuf_read_ahead_add_to_chain(dd);
  }

  z_bdbuf_unlock_cache ();
}

static rtems_status_code
rtems_bdbuf_check_bd_and_lock_cache (struct k_bdbuf_buffer *bd, const char *kind)
{
  if (bd == NULL)
    return RTEMS_INVALID_ADDRESS;
  if (rtems_bdbuf_tracer)
  {
    printf ("bdbuf:%s: %" PRIu32 "\n", kind, bd->block);
    k_bdbuf_show_users (kind, bd);
  }
  z_bdbuf_lock_cache();

  return RTEMS_SUCCESSFUL;
}

rtems_status_code
rtems_bdbuf_release (struct k_bdbuf_buffer *bd)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;

  sc = rtems_bdbuf_check_bd_and_lock_cache (bd, "release");
  if (sc != RTEMS_SUCCESSFUL)
    return sc;

  switch (bd->state)
  {
    case RTEMS_BDBUF_STATE_ACCESS_CACHED:
      z_bdbuf_add_to_lru_list_after_access (bd);
      break;
    case RTEMS_BDBUF_STATE_ACCESS_EMPTY:
    case RTEMS_BDBUF_STATE_ACCESS_PURGED:
      z_bdbuf_discard_buffer_after_access (bd);
      break;
    case RTEMS_BDBUF_STATE_ACCESS_MODIFIED:
      z_bdbuf_add_to_modified_list_after_access (bd);
      break;
    default:
      z_bdbuf_fatal_with_state (bd->state, K_BDBUF_FATAL_STATE_0);
      break;
  }

  if (rtems_bdbuf_tracer)
    k_bdbuf_show_usage ();

  z_bdbuf_unlock_cache ();

  return RTEMS_SUCCESSFUL;
}

rtems_status_code
rtems_bdbuf_release_modified (struct k_bdbuf_buffer *bd)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;

  sc = rtems_bdbuf_check_bd_and_lock_cache (bd, "release modified");
  if (sc != RTEMS_SUCCESSFUL)
    return sc;

  switch (bd->state)
  {
    case RTEMS_BDBUF_STATE_ACCESS_CACHED:
    case RTEMS_BDBUF_STATE_ACCESS_EMPTY:
    case RTEMS_BDBUF_STATE_ACCESS_MODIFIED:
      z_bdbuf_add_to_modified_list_after_access (bd);
      break;
    case RTEMS_BDBUF_STATE_ACCESS_PURGED:
      z_bdbuf_discard_buffer_after_access (bd);
      break;
    default:
      z_bdbuf_fatal_with_state (bd->state, K_BDBUF_FATAL_STATE_6);
      break;
  }

  if (rtems_bdbuf_tracer)
    k_bdbuf_show_usage ();

  z_bdbuf_unlock_cache ();

  return RTEMS_SUCCESSFUL;
}

rtems_status_code
rtems_bdbuf_sync (struct k_bdbuf_buffer *bd)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;

  sc = rtems_bdbuf_check_bd_and_lock_cache (bd, "sync");
  if (sc != RTEMS_SUCCESSFUL)
    return sc;

  switch (bd->state)
  {
    case RTEMS_BDBUF_STATE_ACCESS_CACHED:
    case RTEMS_BDBUF_STATE_ACCESS_EMPTY:
    case RTEMS_BDBUF_STATE_ACCESS_MODIFIED:
      rtems_bdbuf_sync_after_access (bd);
      break;
    case RTEMS_BDBUF_STATE_ACCESS_PURGED:
      z_bdbuf_discard_buffer_after_access (bd);
      break;
    default:
      z_bdbuf_fatal_with_state (bd->state, K_BDBUF_FATAL_STATE_5);
      break;
  }

  if (rtems_bdbuf_tracer)
    k_bdbuf_show_usage ();

  z_bdbuf_unlock_cache ();

  return RTEMS_SUCCESSFUL;
}

rtems_status_code
rtems_bdbuf_syncdev (struct k_disk_device *dd)
{
  if (rtems_bdbuf_tracer)
    printf ("bdbuf:syncdev: %08x\n", (unsigned) dd->dev);

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
  bdbuf_cache.sync_active    = true;
  bdbuf_cache.sync_requester = rtems_task_self ();
  bdbuf_cache.sync_device    = dd;

  z_bdbuf_wake_swapper ();
  z_bdbuf_unlock_cache ();
  z_bdbuf_wait_for_transient_event ();
  z_bdbuf_unlock_sync ();

  return RTEMS_SUCCESSFUL;
}

/**
 * Swapout transfer to the driver. The driver will break this I/O into groups
 * of consecutive write requests is multiple consecutive buffers are required
 * by the driver. The cache is not locked.
 *
 * @param transfer The transfer transaction.
 */
static void
rtems_bdbuf_swapout_write (struct k_bdbuf_swapout_transfer* transfer)
{
  sys_dnode_t *node;

  if (rtems_bdbuf_tracer)
    printf ("bdbuf:swapout transfer: %08x\n", (unsigned) transfer->dd->dev);

  /*
   * If there are buffers to transfer to the media transfer them.
   */
  if (!rtems_chain_is_empty (&transfer->bds))
  {
    /*
     * The last block number used when the driver only supports
     * continuous blocks in a single request.
     */
    uint32_t last_block = 0;

    struct k_disk_device *dd = transfer->dd;
    uint32_t media_blocks_per_block = dd->media_blocks_per_block;
    bool need_continuous_blocks =
      (dd->phys_dev->capabilities & RTEMS_BLKDEV_CAP_MULTISECTOR_CONT) != 0;

    /*
     * Take as many buffers as configured and pass to the driver. Note, the
     * API to the drivers has an array of buffers and if a chain was passed
     * we could have just passed the list. If the driver API is updated it
     * should be possible to make this change with little effect in this
     * code. The array that is passed is broken in design and should be
     * removed. Merging members of a struct into the first member is
     * trouble waiting to happen.
     */
    transfer->write_req.status = RTEMS_RESOURCE_IN_USE;
    transfer->write_req.bufnum = 0;

    while ((node = rtems_chain_get_unprotected(&transfer->bds)) != NULL)
    {
      struct k_bdbuf_buffer* bd = (struct k_bdbuf_buffer*) node;
      bool                write = false;

      /*
       * If the device only accepts sequential buffers and this is not the
       * first buffer (the first is always sequential, and the buffer is not
       * sequential then put the buffer back on the transfer chain and write
       * the committed buffers.
       */

      if (rtems_bdbuf_tracer)
        printf ("bdbuf:swapout write: bd:%" PRIu32 ", bufnum:%" PRIu32 " mode:%s\n",
                bd->block, transfer->write_req.bufnum,
                need_continuous_blocks ? "MULTI" : "SCAT");

      if (need_continuous_blocks && transfer->write_req.bufnum &&
          bd->block != last_block + media_blocks_per_block)
      {
        sys_dlist_prepend (&transfer->bds, &bd->link);
        write = true;
      }
      else
      {
        rtems_blkdev_sg_buffer* buf;
        buf = &transfer->write_req.bufs[transfer->write_req.bufnum];
        transfer->write_req.bufnum++;
        buf->user   = bd;
        buf->block  = bd->block;
        buf->length = dd->block_size;
        buf->buffer = bd->buffer;
        last_block  = bd->block;
      }

      /*
       * Perform the transfer if there are no more buffers, or the transfer
       * size has reached the configured max. value.
       */

      if (rtems_chain_is_empty (&transfer->bds) ||
          (transfer->write_req.bufnum >= bdbuf_config.max_write_blocks))
        write = true;

      if (write)
      {
        rtems_bdbuf_execute_transfer_request (dd, &transfer->write_req, false);

        transfer->write_req.status = RTEMS_RESOURCE_IN_USE;
        transfer->write_req.bufnum = 0;
      }
    }

    /*
     * If sync'ing and the deivce is capability of handling a sync IO control
     * call perform the call.
     */
    if (transfer->syncing &&
        (dd->phys_dev->capabilities & RTEMS_BLKDEV_CAP_SYNC))
    {
      /* int result = */ dd->ioctl (dd->phys_dev, RTEMS_BLKDEV_REQ_SYNC, NULL);
      /* How should the error be handled ? */
    }
  }
}

/**
 * Process the modified list of buffers. There is a sync or modified list that
 * needs to be handled so we have a common function to do the work.
 *
 * @param dd_ptr Pointer to the device to handle. If BDBUF_INVALID_DEV no
 * device is selected so select the device of the first buffer to be written to
 * disk.
 * @param chain The modified chain to process.
 * @param transfer The chain to append buffers to be written too.
 * @param sync_active If true this is a sync operation so expire all timers.
 * @param update_timers If true update the timers.
 * @param timer_delta It update_timers is true update the timers by this
 *                    amount.
 */
static void
rtems_bdbuf_swapout_modified_processing (struct k_disk_device  **dd_ptr,
                                         sys_dlist_t* chain,
                                         sys_dlist_t* transfer,
                                         bool                 sync_active,
                                         bool                 update_timers,
                                         uint32_t             timer_delta)
{
  if (!rtems_chain_is_empty (chain))
  {
    sys_dnode_t* node = rtems_chain_head (chain);
    bool              sync_all;

    node = node->next;

    /*
     * A sync active with no valid dev means sync all.
     */
    if (sync_active && (*dd_ptr == BDBUF_INVALID_DEV))
      sync_all = true;
    else
      sync_all = false;

    while (!rtems_chain_is_tail (chain, node))
    {
      struct k_bdbuf_buffer* bd = (struct k_bdbuf_buffer*) node;

      /*
       * Check if the buffer's hold timer has reached 0. If a sync is active
       * or someone waits for a buffer written force all the timers to 0.
       *
       * @note Lots of sync requests will skew this timer. It should be based
       *       on TOD to be accurate. Does it matter ?
       */
      if (sync_all || (sync_active && (*dd_ptr == bd->dd))
          || z_bdbuf_has_buffer_waiters ())
        bd->hold_timer = 0;

      if (bd->hold_timer)
      {
        if (update_timers)
        {
          if (bd->hold_timer > timer_delta)
            bd->hold_timer -= timer_delta;
          else
            bd->hold_timer = 0;
        }

        if (bd->hold_timer)
        {
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

      if (bd->dd == *dd_ptr)
      {
        sys_dnode_t* next_node = node->next;
        sys_dnode_t* tnode = rtems_chain_tail (transfer);

        /*
         * The blocks on the transfer list are sorted in block order. This
         * means multi-block transfers for drivers that require consecutive
         * blocks perform better with sorted blocks and for real disks it may
         * help lower head movement.
         */

        z_bdbuf_set_state (bd, RTEMS_BDBUF_STATE_TRANSFER);

        rtems_chain_extract_unprotected (node);

        tnode = tnode->previous;

        while (node && !rtems_chain_is_head (transfer, tnode))
        {
          struct k_bdbuf_buffer* tbd = (struct k_bdbuf_buffer*) tnode;

          if (bd->block > tbd->block)
          {
            rtems_chain_insert_unprotected (tnode, node);
            node = NULL;
          }
          else
            tnode = tnode->previous;
        }

        if (node)
          sys_dlist_prepend (transfer, node);

        node = next_node;
      }
      else
      {
        node = node->next;
      }
    }
  }
}

/**
 * Process the cache's modified buffers. Check the sync list first then the
 * modified list extracting the buffers suitable to be written to disk. We have
 * a device at a time. The task level loop will repeat this operation while
 * there are buffers to be written. If the transfer fails place the buffers
 * back on the modified list and try again later. The cache is unlocked while
 * the buffers are being written to disk.
 *
 * @param timer_delta It update_timers is true update the timers by this
 *                    amount.
 * @param update_timers If true update the timers.
 * @param transfer The transfer transaction data.
 *
 * @retval true Buffers where written to disk so scan again.
 * @retval false No buffers where written to disk.
 */
static bool
rtems_bdbuf_swapout_processing (unsigned long                 timer_delta,
                                bool                          update_timers,
                                struct k_bdbuf_swapout_transfer* transfer)
{
  struct k_bdbuf_swapout_worker* worker;
  bool                        transfered_buffers = false;
  bool                        sync_active;

  z_bdbuf_lock_cache ();

  /*
   * To set this to true you need the cache and the sync lock.
   */
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
  if (sync_active)
    worker = NULL;
  else
  {
    worker = (struct k_bdbuf_swapout_worker*)
      rtems_chain_get_unprotected (&bdbuf_cache.swapout_free_workers);
    if (worker)
      transfer = &worker->transfer;
  }

  rtems_chain_initialize_empty (&transfer->bds);
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
  rtems_bdbuf_swapout_modified_processing (&transfer->dd,
                                           &bdbuf_cache.sync,
                                           &transfer->bds,
                                           true, false,
                                           timer_delta);

  /*
   * Process the cache's modified list.
   */
  rtems_bdbuf_swapout_modified_processing (&transfer->dd,
                                           &bdbuf_cache.modified,
                                           &transfer->bds,
                                           sync_active,
                                           update_timers,
                                           timer_delta);

  /*
   * We have all the buffers that have been modified for this device so the
   * cache can be unlocked because the state of each buffer has been set to
   * TRANSFER.
   */
  z_bdbuf_unlock_cache ();

  /*
   * If there are buffers to transfer to the media transfer them.
   */
  if (!rtems_chain_is_empty (&transfer->bds))
  {
    if (worker)
    {
      rtems_status_code sc = rtems_event_send (worker->id,
                                               RTEMS_BDBUF_SWAPOUT_SYNC);
      if (sc != RTEMS_SUCCESSFUL)
        z_bdbuf_fatal (K_BDBUF_FATAL_SO_WAKE_2);
    }
    else
    {
      rtems_bdbuf_swapout_write (transfer);
    }

    transfered_buffers = true;
  }

  if (sync_active && !transfered_buffers)
  {
    rtems_id sync_requester;
    z_bdbuf_lock_cache ();
    sync_requester = bdbuf_cache.sync_requester;
    bdbuf_cache.sync_active = false;
    bdbuf_cache.sync_requester = 0;
    z_bdbuf_unlock_cache ();
    if (sync_requester)
      rtems_event_transient_send (sync_requester);
  }

  return transfered_buffers;
}

static void bdbuf_swapout_worker_task(void *arg) {
  struct k_bdbuf_swapout_worker* worker = arg;
  while (worker->enabled) {
    z_bdbuf_wait_for_event(&worker->sync);
    rtems_bdbuf_swapout_write(&worker->transfer);
    z_bdbuf_lock_cache();
    sys_dlist_init(&worker->transfer.bds);
    worker->transfer.dd = BDBUF_INVALID_DEV;
    sys_dlist_append(&bdbuf_cache.swapout_free_workers, &worker->link);
    z_bdbuf_unlock_cache();
  }
  k_free(worker);
  k_thread_abort();
}

static void bdbuf_swapout_workers_close(void) {
  sys_dnode_t* node;
  z_bdbuf_lock_cache ();
  SYS_DLIST_FOR_EACH_NODE(&bdbuf_cache.swapout_free_workers, node) {
    struct k_bdbuf_swapout_worker* worker = CONTAINER_OF(node,
      struct k_bdbuf_swapout_worker, link);
    worker->enabled = false;
    k_sem_give(&worker->sync);
  }
  z_bdbuf_unlock_cache();
}

static void bdbuf_swapout_task(void *arg) {
  const uint32_t period_in_msecs = bdbuf_config.swapout_period;
  struct k_bdbuf_swapout_transfer* transfer = arg;
  uint32_t period_in_ticks;
  uint32_t timer_delta;

  /*
   * Localise the period.
   */
  period_in_ticks = RTEMS_MICROSECONDS_TO_TICKS (period_in_msecs * 1000);

  /*
   * This is temporary. Needs to be changed to use the real time clock.
   */
  timer_delta = period_in_msecs;

  while (bdbuf_cache.swapout_enabled)
  {
    rtems_event_set   out;
    rtems_status_code sc;

    /*
     * Only update the timers once in the processing cycle.
     */
    bool update_timers = true;

    /*
     * If we write buffers to any disk perform a check again. We only write a
     * single device at a time and the cache may have more than one device's
     * buffers modified waiting to be written.
     */
    bool transfered_buffers;

    do
    {
      transfered_buffers = false;

      /*
       * Extact all the buffers we find for a specific device. The device is
       * the first one we find on a modified list. Process the sync queue of
       * buffers first.
       */
      if (rtems_bdbuf_swapout_processing (timer_delta,
                                          update_timers,
                                          transfer))
      {
        transfered_buffers = true;
      }

      /*
       * Only update the timers once.
       */
      update_timers = false;
    }
    while (transfered_buffers);

    sc = rtems_event_receive (RTEMS_BDBUF_SWAPOUT_SYNC,
                              RTEMS_EVENT_ALL | RTEMS_WAIT,
                              period_in_ticks,
                              &out);

    if ((sc != RTEMS_SUCCESSFUL) && (sc != RTEMS_TIMEOUT))
      z_bdbuf_fatal (K_BDBUF_FATAL_SWAPOUT_RE);
  }

  bdbuf_swapout_workers_close();

  k_free (transfer);

  k_thread_abort();
}

static void
rtems_bdbuf_purge_list (sys_dlist_t *purge_list)
{
  bool wake_buffer_waiters = false;
  sys_dnode_t *node = NULL;

  while ((node = rtems_chain_get_unprotected (purge_list)) != NULL)
  {
    struct k_bdbuf_buffer *bd = (struct k_bdbuf_buffer *) node;

    if (bd->waiters == 0)
      wake_buffer_waiters = true;

    z_bdbuf_discard_buffer (bd);
  }

  if (wake_buffer_waiters)
    z_bdbuf_wake (&bdbuf_cache.buffer_waiters);
}

static void
rtems_bdbuf_gather_for_purge (sys_dlist_t *purge_list,
                              const struct k_disk_device *dd)
{
  struct k_bdbuf_buffer *stack [RTEMS_BDBUF_AVL_MAX_HEIGHT];
  struct k_bdbuf_buffer **prev = stack;
  struct k_bdbuf_buffer *cur = bdbuf_cache.tree;

  *prev = NULL;

  while (cur != NULL)
  {
    if (cur->dd == dd)
    {
      switch (cur->state)
      {
        case RTEMS_BDBUF_STATE_FREE:
        case RTEMS_BDBUF_STATE_EMPTY:
        case RTEMS_BDBUF_STATE_ACCESS_PURGED:
        case RTEMS_BDBUF_STATE_TRANSFER_PURGED:
          break;
        case RTEMS_BDBUF_STATE_SYNC:
          z_bdbuf_wake (&bdbuf_cache.transfer_waiters);
          /* Fall through */
        case RTEMS_BDBUF_STATE_MODIFIED:
          z_bdbuf_group_release (cur);
          /* Fall through */
        case RTEMS_BDBUF_STATE_CACHED:
          rtems_chain_extract_unprotected (&cur->link);
          sys_dlist_append (purge_list, &cur->link);
          break;
        case RTEMS_BDBUF_STATE_TRANSFER:
          z_bdbuf_set_state (cur, RTEMS_BDBUF_STATE_TRANSFER_PURGED);
          break;
        case RTEMS_BDBUF_STATE_ACCESS_CACHED:
        case RTEMS_BDBUF_STATE_ACCESS_EMPTY:
        case RTEMS_BDBUF_STATE_ACCESS_MODIFIED:
          z_bdbuf_set_state (cur, RTEMS_BDBUF_STATE_ACCESS_PURGED);
          break;
        default:
          z_bdbuf_fatal (K_BDBUF_FATAL_STATE_11);
      }
    }

    if (cur->avl.left != NULL)
    {
      /* Left */
      ++prev;
      *prev = cur;
      cur = cur->avl.left;
    }
    else if (cur->avl.right != NULL)
    {
      /* Right */
      ++prev;
      *prev = cur;
      cur = cur->avl.right;
    }
    else
    {
      while (*prev != NULL
             && (cur == (*prev)->avl.right || (*prev)->avl.right == NULL))
      {
        /* Up */
        cur = *prev;
        --prev;
      }
      if (*prev != NULL)
        /* Right */
        cur = (*prev)->avl.right;
      else
        /* Finished */
        cur = NULL;
    }
  }
}

static void
rtems_bdbuf_do_purge_dev (struct k_disk_device *dd)
{
  sys_dlist_t purge_list;

  rtems_chain_initialize_empty (&purge_list);
  rtems_bdbuf_read_ahead_reset (dd);
  rtems_bdbuf_gather_for_purge (&purge_list, dd);
  rtems_bdbuf_purge_list (&purge_list);
}

void
rtems_bdbuf_purge_dev (struct k_disk_device *dd)
{
  z_bdbuf_lock_cache ();
  rtems_bdbuf_do_purge_dev (dd);
  z_bdbuf_unlock_cache ();
}

rtems_status_code
rtems_bdbuf_set_block_size (struct k_disk_device *dd,
                            uint32_t           block_size,
                            bool               sync)
{
  rtems_status_code sc = RTEMS_SUCCESSFUL;

  /*
   * We do not care about the synchronization status since we will purge the
   * device later.
   */
  if (sync)
    rtems_bdbuf_syncdev (dd);

  z_bdbuf_lock_cache ();

  if (block_size > 0)
  {
    size_t bds_per_group = z_bdbuf_bds_per_group (block_size);

    if (bds_per_group != 0)
    {
      int block_to_media_block_shift = 0;
      uint32_t media_blocks_per_block = block_size / dd->media_block_size;
      uint32_t one = 1;

      while ((one << block_to_media_block_shift) < media_blocks_per_block)
      {
        ++block_to_media_block_shift;
      }

      if ((dd->media_block_size << block_to_media_block_shift) != block_size)
        block_to_media_block_shift = -1;

      dd->block_size = block_size;
      dd->block_count = dd->size / media_blocks_per_block;
      dd->media_blocks_per_block = media_blocks_per_block;
      dd->block_to_media_block_shift = block_to_media_block_shift;
      dd->bds_per_group = bds_per_group;

      rtems_bdbuf_do_purge_dev (dd);
    }
    else
    {
      sc = RTEMS_INVALID_NUMBER;
    }
  }
  else
  {
    sc = RTEMS_INVALID_NUMBER;
  }

  z_bdbuf_unlock_cache ();

  return sc;
}

static void bdbuf_read_ahead_task(void *arg) {
  sys_dlist_t *chain = &bdbuf_cache.read_ahead_chain;

  while (bdbuf_cache.read_ahead_enabled) {
    sys_dnode_t *node;
    k_sem_take(&read_ahead_sem, K_FOREVER);
    z_bdbuf_wait_for_event(RTEMS_BDBUF_READ_AHEAD_WAKE_UP);
    z_bdbuf_lock_cache ();
    while ((node = rtems_chain_get_unprotected (chain)) != NULL)
    {
      struct k_disk_device *dd =
        RTEMS_CONTAINER_OF (node, struct k_disk_device, read_ahead.node);
      blkdev_bnum_t block = dd->read_ahead.next;
      blkdev_bnum_t media_block = 0;
      rtems_status_code sc =
        rtems_bdbuf_get_media_block (dd, block, &media_block);

      rtems_chain_set_off_chain (&dd->read_ahead.node);

      if (sc == RTEMS_SUCCESSFUL)
      {
        struct k_bdbuf_buffer *bd =
          rtems_bdbuf_get_buffer_for_read_ahead (dd, media_block);

        if (bd != NULL)
        {
          uint32_t transfer_count = dd->read_ahead.nr_blocks;
          uint32_t blocks_until_end_of_disk = dd->block_count - block;
          uint32_t max_transfer_count = bdbuf_config.max_read_ahead_blocks;

          if (transfer_count == RTEMS_DISK_READ_AHEAD_SIZE_AUTO) {
            transfer_count = blocks_until_end_of_disk;

            if (transfer_count >= max_transfer_count)
            {
              transfer_count = max_transfer_count;
              dd->read_ahead.trigger = block + transfer_count / 2;
              dd->read_ahead.next = block + transfer_count;
            }
            else
            {
              dd->read_ahead.trigger = RTEMS_DISK_READ_AHEAD_NO_TRIGGER;
            }
          } else {
            if (transfer_count > blocks_until_end_of_disk) {
              transfer_count = blocks_until_end_of_disk;
            }

            if (transfer_count > max_transfer_count) {
              transfer_count = max_transfer_count;
            }

            ++dd->stats.read_ahead_peeks;
          }

          ++dd->stats.read_ahead_transfers;
          rtems_bdbuf_execute_read_request (dd, bd, transfer_count);
        }
      }
      else
      {
        dd->read_ahead.trigger = RTEMS_DISK_READ_AHEAD_NO_TRIGGER;
      }
    }

    z_bdbuf_unlock_cache ();
  }
  k_thread_abort();
}

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