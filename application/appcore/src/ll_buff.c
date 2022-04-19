/*
 * CopyRight 2022 wtcat
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef __ZEPHYR__
#include <sys/cbprintf.h>
#endif

#include "ll_buff.h"


#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef __noinit
#define __noinit 
#endif


#define LOG_RAM (&_ll_log_buffer)

static struct ll_buffer_context _ll_log_buffer __attribute__((section(".ram_reserved.ll_buff")));
struct ll_buffer_context *_ll_log_context = LOG_RAM;

struct ll_printer _ll_log_printer = {
	.context = LOG_RAM,
	.print = ll_buffer_context_print
};
	static struct k_spinlock ll_lock;
k_spinlock_key_t k_spin_lock(struct k_spinlock *l);

static void ll_out(int c, void *ctx) {
	struct ll_buffer_context *bc = ctx;
	bc->buffer[bc->wrp++] = (char)c;
	if (bc->wrp >= LL_BUFF_LEN) {
		bc->full = true;
		bc->wrp = 0;
	}
	if (bc->full)
		bc->rdp = bc->wrp + 1;
}

int ll_buffer_context_print(void *ctx, const char *fmt, va_list ap) {
	struct ll_buffer_context *bc = ctx;
	k_spinlock_key_t key = k_spin_lock(&bc->lock);
	if (!ll_buffer_context_ready(bc))
		ll_buffer_context_reset(bc);
	if (bc->rdlock) {
		k_spin_unlock(&bc->lock, key);
		return -EBUSY;
	}
	int len = cbvprintf(ll_out, ctx, fmt, ap);
	k_spin_unlock(&bc->lock, key);
	return len;
}

size_t ll_buffer_context_read(struct ll_buffer_context *bc, char *dst, 
	size_t maxlen, size_t *offset) {
	k_spinlock_key_t key = k_spin_lock(&bc->lock);
	if (!ll_buffer_context_ready(bc)) {
		k_spin_unlock(&bc->lock, key);
		return 0;
	}
	size_t ofs = *offset;
	char *buffer = dst;
	while (maxlen > 0) {
		size_t len;
		if (ofs > bc->wrp)
			len = MIN(maxlen, LL_BUFF_LEN - ofs);
		else
			len = MIN(maxlen, bc->wrp - ofs);
		if (len == 0)
			break;
		memcpy(buffer, &bc->buffer[ofs], len);
		buffer += len;
		maxlen -= len;
		LL_BUFF_POINTER_ADD(ofs, len); 
	}
	k_spin_unlock(&bc->lock, key);
	*offset = ofs;
	return (size_t)(buffer - dst);
}

void ll_buffer_context_dump(struct ll_buffer_context *buff, 
	int (*putc)(int c)) {
	char buffer[64];
	if (putc == NULL)
		return;
	ll_buffer_copy(buffer, sizeof(buffer), 
		for (int i = 0; i < buffer_len; i++) {putc(buffer[i]);});
}

