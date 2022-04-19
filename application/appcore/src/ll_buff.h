/*
 * CopyRight 2022 wtcat
 */
 
#ifndef LL_BUFFER_H_
#define LL_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <spinlock.h>

#ifdef __cplusplus
extern "C"{
#endif

struct ll_printer {
    int (*print)(void *context, const char *fmt, va_list ap_list);
    void *context;
};

struct ll_buffer_context {
#define LL_BUFF_MAGIC 0xdeadbeef
#define LL_BUFF_LEN   8192
#define LL_BUFF_POINTER_ADD(p, len) (p) = ((p) + (len)) & (LL_BUFF_LEN - 1);
#define LL_BUFF_BEGIN(ctx) (ctx)->rdp
	uint32_t magic;
	size_t rdp;
	size_t wrp;
	struct k_spinlock lock;
	uint16_t rdlock;
	uint16_t full;
	char buffer[LL_BUFF_LEN];
};

extern struct ll_buffer_context * _ll_log_context;
extern struct ll_printer _ll_log_printer;


static inline int ll_print(const struct ll_printer *printer, 
    const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = printer->print(printer->context, fmt, ap);
    va_end(ap);
    return len;
}

static inline void ll_buffer_context_read_lock(struct ll_buffer_context *buff) {
	buff->rdlock = true;
}

static inline void ll_buffer_context_read_unlock(struct ll_buffer_context *buff) {
	buff->rdlock = false;
}

static inline bool ll_buffer_context_ready(struct ll_buffer_context *buff) {
	return buff->magic == LL_BUFF_MAGIC;
}

static inline void ll_buffer_context_reset(struct ll_buffer_context *buff) {
	buff->rdlock = buff->full = 0;
	buff->rdp = 0;
	buff->wrp = 0;
	buff->magic = LL_BUFF_MAGIC;
}

int ll_buffer_context_print(void *ctx, const char *fmt, va_list ap);
size_t ll_buffer_context_read(struct ll_buffer_context *buff, char *dst, 
	size_t maxlen, size_t *offset);
void ll_buffer_context_dump(struct ll_buffer_context *buff, 
	int (*putc)(int c));

#define ll_buffer_context_copy(_context, _buf, _bufsize, _command) ({ \
	ll_buffer_context_read_lock(_context);	\
	size_t __ofs = LL_BUFF_BEGIN(_context), __len = 0;	\
	size_t _buf##_len = ll_buffer_context_read(_context, _buf, _bufsize, &__ofs);	\
	if (_buf##_len > 0) {	\
		do {	\
			_command \
			_buf##_len = ll_buffer_context_read(_context, _buf, _bufsize, &__ofs);	\
			__len += _buf##_len; \
		} while (_buf##_len > 0);	\
	}	\
	ll_buffer_context_read_unlock(_context);	\
	__len;	\
})

#define ll_buffer_reset() \
    ll_buffer_context_reset(_ll_log_context)

#define ll_buffer_print(fmt, ...) \
	ll_print(&_ll_log_printer, fmt, ##__VA_ARGS__)
	
#define ll_buffer_read(buff, maxlen, offset) \
	ll_buffer_context_read(_ll_log_context, buff, maxlen, offset)

#define ll_buffer_read_lock() ({\
	ll_buffer_context_read_lock(_ll_log_context); \
	_ll_log_context->rdp; })

#define ll_buffer_read_unlock() \
	ll_buffer_context_read_unlock(_ll_log_context)

#define ll_buffer_begin() \
	_ll_log_context->rdp

#define ll_buffer_dump(putc_fn) \
	ll_buffer_context_dump(_ll_log_context, putc_fn)

#define ll_buffer_copy(_buf, _bufsize, _command) \
	ll_buffer_context_copy(_ll_log_context, _buf, _bufsize, _command)


#define ll_info(fmt, ...) \
	ll_buffer_print(fmt, ##__VA_ARGS__)

#define ll_warn(fmt, ...) \
	ll_buffer_print("Warning***[%s: %d]: "fmt, __func__, __LINE__, ##__VA_ARGS__)

#define ll_error(fmt, ...) \
	ll_buffer_print("Error***[%s: %d]: "fmt, __func__, __LINE__, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif /* LL_BUFFER_H_ */

