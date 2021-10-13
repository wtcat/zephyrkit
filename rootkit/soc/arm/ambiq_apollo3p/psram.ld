
#if DT_NODE_HAS_STATUS(DT_CHOSEN(psram), okay)
GROUP_START(PSRAM)

	SECTION_PROLOGUE(.psram_bss, (NOLOAD),SUBALIGN(4))
	{
		__psram_start = .;
		__psram_bss_start = .;
		*(.psram_bss)
		*(".psram_bss.*")
		__psram_bss_end = .;
	} GROUP_LINK_IN(PSRAM)

	SECTION_PROLOGUE(.psram_noinit, (NOLOAD),SUBALIGN(4))
	{
		__psram_noinit_start = .;
		*(.psram_noinit)
		*(".psram_noinit.*")
		__psram_noinit_end = .;
	} GROUP_LINK_IN(PSRAM)

	SECTION_PROLOGUE(.psram_data,,SUBALIGN(4))
	{
		__psram_data_start = .;
		*(.psram_data)
		*(".psram_data.*")
		__psram_data_end = .;
	} GROUP_LINK_IN(PSRAM AT> ROMABLE_REGION)

	__psram_end = .;

	__psram_data_rom_start = LOADADDR(.psram_data);

GROUP_END(PSRAM)
#endif