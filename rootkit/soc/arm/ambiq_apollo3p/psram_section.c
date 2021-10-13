#include <string.h>
#include <devicetree.h>

#if DT_NODE_HAS_STATUS(DT_CHOSEN(psram), okay)
extern char __psram_data_rom_start[];
extern char __psram_start[];
extern char __psram_data_start[];
extern char __psram_data_end[];
extern char __psram_bss_start[];
extern char __psram_bss_end[];
extern char __psram_noinit_start[];
extern char __psram_noinit_end[];
extern char __psram_end[];
#endif

void __psram_section_init(void)
{
#if DT_NODE_HAS_STATUS(DT_CHOSEN(psram), okay)
	memset(__psram_bss_start, 0, __psram_bss_end - __psram_bss_start);
	(void)memcpy(&__psram_data_start, &__psram_data_rom_start,
		 __psram_data_end - __psram_data_start);
#endif
}