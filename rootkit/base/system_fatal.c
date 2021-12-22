#include "base/system_fatal.h"

/* 
 * Overwrite default 
 */
void k_sys_fatal_error_handler(unsigned int reason,
	const z_arch_esf_t *esf) {
	extern void arch_system_halt(unsigned int);
    STRUCT_SECTION_FOREACH(fatal_extension, iter) {
        iter->fatal(reason, esf);
    }
	arch_system_halt(reason); 
}
