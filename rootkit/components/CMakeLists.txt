# Components

zephyr_include_directories(.)

add_subdirectory(base)
add_subdirectory(stp)
add_subdirectory(protobuf-c)
add_subdirectory(services)

add_subdirectory_ifdef(CONFIG_MOTION           motion)
add_subdirectory_ifdef(CONFIG_GUIX             gui)
add_subdirectory_ifdef(CONFIG_BLE_COMPONENT    ble)
add_subdirectory_ifdef(CONFIG_BLE_COMPONENT    uecc)
add_subdirectory_ifdef(CONFIG_XMODEM           xmodem)
add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M     cm_backtrace)

