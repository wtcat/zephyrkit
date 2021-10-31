# Initialize
if (CONFIG_USER_FS_MOUNT)
    zephyr_library()
    zephyr_library_sources(fsinit.c)
    if (CONFIG_FILE_SYSTEM_LITTLEFS)
        zephyr_library_link_libraries(LITTLEFS)
    elseif (CONFIG_FAT_FILESYSTEM_ELM)
        zephyr_library_link_libraries(ELMFAT)
    endif()
endif()


