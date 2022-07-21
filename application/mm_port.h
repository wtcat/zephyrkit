#ifndef MM_PORT_H_
#define MM_PORT_H_

#if defined(__linux__) || defined(_WIN32)
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define MM_ASSERT(n) assert(n)
#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define MUTEX_LOCK_DECLARE
#define MUTEX_LOCK(...)
#define MUTEX_UNLOCK(...)
#elif defined(__ZEPHYR__)
#include <zephyr.h>
#include <init.h>
#include <sys/__assert.h>

#define MM_ASSERT(n) __ASSERT_NO_MSG(n)
#define PRINTF(...)
#define MUTEX_LOCK_DECLARE unsigned int __key;
#define MUTEX_LOCK(s)       __key = irq_lock()
#define MUTEX_UNLOCK(s)     irq_unlock(__key)
#else
#error "Unknown operation system"
#endif

#endif /* MM_PORT_H_ */

