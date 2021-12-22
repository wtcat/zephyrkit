#ifndef BASE_SYSTEM_FATAL_H_
#define BASE_SYSTEM_FATAL_H_

#include <toolchain.h>
#include <arch/cpu.h>

#ifdef __cplusplus
extern "C"{
#endif

struct fatal_extension {
    void (*fatal)(unsigned int reason, const z_arch_esf_t *esf);
};

#define SYS_FATAL_DEFINE(_name)	\
	static const STRUCT_SECTION_ITERABLE(fatal_extension, \
					_CONCAT(sys_fatal_,	_name))
							
#ifdef __cplusplus
}
#endif
#endif /* BASE_SYSTEM_FATAL_H_ */
