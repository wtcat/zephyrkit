# Initialize

if (CONFIG_FILE_SYSTEM)
zephyr_library()
zephyr_library_sources(fsinit.c)
zephyr_library_link_libraries(LITTLEFS)
endif()


