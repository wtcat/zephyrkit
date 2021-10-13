# Components

zephyr_library_named(ble)

add_subdirectory(ambiq_ble)
add_subdirectory(ble-host)
add_subdirectory(ble-profiles)
add_subdirectory(wsf)

add_dependencies(ble zephyr)

