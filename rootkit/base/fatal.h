#ifndef BASE_FATAL_H_
#define BASE_FATAL_H_

#include <toolchain.h>
#include <arch/cpu.h>

#include "base/printer.h"

#ifdef __cplusplus
extern "C"{
#endif

struct fatal_extension {
    void (*fatal)(const struct printer *printer, unsigned int reason, 
		const z_arch_esf_t *esf);
};

#define SYS_FATAL_DEFINE(_name)	\
	static const STRUCT_SECTION_ITERABLE(fatal_extension, \
					_CONCAT(sys_fatal_,	_name))

int  fatal_printer_register(const struct printer *printer);
void fatal_printer_reset(void);

#ifdef __cplusplus
}
#endif
#endif /* BASE_FATAL_H_ */
