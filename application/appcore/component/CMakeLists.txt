# Application Components
zephyr_library_named(component)

zephyr_library_sources(${ZEPHYR_BASE}/misc/empty_file.c)
add_subdirectory_ifdef(CONFIG_BT bt)