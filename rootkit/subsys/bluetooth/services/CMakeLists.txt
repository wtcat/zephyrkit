zephyr_include_directories(${BSP_ROOT}/include)
zephyr_sources_ifdef(CONFIG_BT_NUS nus.c)
zephyr_sources_ifdef(CONFIG_BT_NUS_CLIENT nus_client.c)
