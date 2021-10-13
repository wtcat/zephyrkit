# Device driver subdirectories

zephyr_include_directories(.)
zephyr_include_directories(../include)

add_subdirectory_ifdef(CONFIG_COUNTER_EXTRA counter)