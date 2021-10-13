# Sensor driver subdirectories

add_subdirectory_ifdef(CONFIG_MMC5603	 mmc5603)
#add_subdirectory_ifdef(CONFIG_PAH8112	 pah8112)
add_subdirectory_ifdef(CONFIG_GH3011	 gh3011)
add_subdirectory_ifdef(CONFIG_ICM42607 icm42607)
add_subdirectory_ifdef(CONFIG_CW6305 cw6305)

