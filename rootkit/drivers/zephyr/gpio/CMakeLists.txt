# GPIO driver

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources_ifdef(CONFIG_GPIO_APOLLO3P gpio_apollo3p.c)
