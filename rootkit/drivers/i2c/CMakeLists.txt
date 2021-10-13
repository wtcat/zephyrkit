# GPIO driver

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources_ifdef(CONFIG_I2C_APOLLO3P i2c_apollo3p.c)
