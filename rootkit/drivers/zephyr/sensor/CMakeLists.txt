# Sensor driver subdirectories

add_subdirectory_ifdef(CONFIG_MMC5603	 mmc5603)
add_subdirectory_ifdef(CONFIG_CW6305     cw6305)
add_subdirectory_ifdef(CONFIG_PMW3360    pmw3360)
add_subdirectory_ifdef(CONFIG_PAW3212    paw3212)