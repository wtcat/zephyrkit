#include <errno.h>
#include <sys/printk.h>

#include "base/fatal.h"

static int pr_error(void *ctx, const char *fmt, va_list ap) ;
static const struct printer default_printer = {
	.print = pr_error
};
static const struct printer *err_printer = &default_printer;

/* 
 * Overwrite default 
 */
void k_sys_fatal_error_handler(unsigned int reason,
	const z_arch_esf_t *esf) {
	extern void arch_system_halt(unsigned int);
    STRUCT_SECTION_FOREACH(fatal_extension, iter) {
        iter->fatal(err_printer, reason, esf);
    }
	arch_system_halt(reason); 
}

static int pr_error(void *ctx, const char *fmt, va_list ap) {
	(void) ctx;
	vprintk(fmt, ap);
	return 0;
}

int fatal_printer_register(const struct printer *printer) {
    if (printer != NULL) {
        err_printer = printer;
        return 0;
    }
    return -EINVAL;
}

void fatal_printer_reset(void) {
    err_printer = &default_printer;
}
