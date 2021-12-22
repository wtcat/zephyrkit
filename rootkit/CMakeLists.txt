
zephyr_include_directories(
    include
    .
)

add_subdirectory(base)
add_subdirectory_ifdef(CONFIG_USER_SUBSYSTEM     subsys)
add_subdirectory_ifdef(CONFIG_USER_LIBRARIES     lib)
add_subdirectory_ifdef(CONFIG_USER_SYSTEM_INIT   init)
add_subdirectory_ifdef(CONFIG_GUIX               gui)