# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if (CONFIG_ICM42607)
  zephyr_library_sources(icm42607.c)
  zephyr_library_sources(icm42607_setup.c)
  zephyr_library_sources_ifdef(CONFIG_SPI icm42607_spi.c)
  zephyr_library_sources_ifdef(CONFIG_ICM42607_TRIGGER icm42607_trigger.c)
endif()
