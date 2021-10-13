zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MMC5603 
	mmc5603.c
	mmc5603_i2c.c
)

