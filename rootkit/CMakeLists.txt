
add_subdirectory_ifdef(CONFIG_USER_SUBSYSTEM     subsys)
add_subdirectory_ifdef(CONFIG_USER_DEVICE_DRIVER drivers)
add_subdirectory_ifdef(CONFIG_USER_DEVICE_DRIVER drivers_ext)
add_subdirectory_ifdef(CONFIG_USER_LIBRARIES     lib)
