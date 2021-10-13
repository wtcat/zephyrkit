
zephyr_library_named(unit_test)

if (CONFIG_FATFS_TEST)
zephyr_library_sources(fatfs_unit_test.c)
target_link_libraries(unit_test PUBLIC ELMFAT)
endif()

zephyr_library_sources_ifdef(CONFIG_GPIO_TEST   gpio_unit_test.c)
zephyr_library_sources_ifdef(CONFIG_LIS2DH_TEST lis2dh_unit_test.c)
#zephyr_library_sources_ifdef(CONFIG_ADC_TEST adc_unit_test.c)
zephyr_library_sources_ifdef(CONFIG_ADC_TEST adc_sensor_service_unit_test.c)
zephyr_library_sources_ifdef(CONFIG_ICM42607_TEST icm42607_test.c)
zephyr_library_sources(empty_test.c)

add_subdirectory(ble)

