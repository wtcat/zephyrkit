#ifndef ZEPHYR_INCLUDE_LINKER_EXTERN_SECTION_TAGS_H_
#define ZEPHYR_INCLUDE_LINKER_EXTERN_SECTION_TAGS_H_

#include <toolchain.h>
#include <devicetree.h>

#if !defined(_ASMLANGUAGE)
#if DT_NODE_HAS_STATUS(DT_CHOSEN(psram), okay)
#define __psram_data   Z_GENERIC_SECTION(.psram_data)
#define __psram_bss    Z_GENERIC_SECTION(.psram_bss)
#define __psram_noinit Z_GENERIC_SECTION(.psram_noinit)
#else
#define __psram_data   
#define __psram_bss
#define __psram_noinit
#endif /* DT_NODE_HAS_STATUS(DT_CHOSEN(psram), okay) */
#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_LINKER_EXTERN_SECTION_TAGS_H_ */