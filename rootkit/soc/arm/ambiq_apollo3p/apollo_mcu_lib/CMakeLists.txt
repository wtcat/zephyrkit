add_subdirectory_ifdef(CONFIG_HAS_APOLLO_LIB mcu)

zephyr_include_directories(utils)

zephyr_library_sources(
  utils/am_util_delay.c
)

if (CONFIG_BLE_COMPONENT OR
    CONFIG_BT)
  zephyr_library_sources( 
    utils/am_util_ble.c
  )
endif()


