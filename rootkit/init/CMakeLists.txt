# Initialize
if (CONFIG_USER_FS_MOUNT)
    if (FILE_SYSTEM_LITTLEFS)
        zephyr_library()
        zephyr_library_sources(fsinit.c)
        zephyr_library_link_libraries(LITTLEFS)
    endif()
endif()


