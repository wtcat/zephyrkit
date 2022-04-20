#ifndef ZEPHYR_UNWIND_H_
#define ZEPHYR_UNWIND_H_

#include <stdarg.h>
#include "base/printer.h"

#ifdef __cplusplus
extern "C"{
#endif

struct k_thread;

void unwind_backtrace(struct printer *printer, struct k_thread *th);
void backtrace_dump(struct printer *printer, const void *esf);

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_UNWIND_H_ */