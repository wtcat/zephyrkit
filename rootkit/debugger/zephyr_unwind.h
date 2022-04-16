#ifndef ZEPHYR_UNWIND_H_
#define ZEPHYR_UNWIND_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C"{
#endif

struct k_thread;

struct printer {
    int (*print)(void *context, const char *fmt, va_list ap_list);
    void *context;
};

static inline int virt_print(const struct printer *printer, 
    const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = printer->print(printer->context, fmt, ap);
    va_end(ap);
    return len;
}

void unwind_backtrace(struct printer *printer, struct k_thread *th);
void system_backtrace(struct printer *printer, const void *esf, 
	bool force_reboot);

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_UNWIND_H_ */