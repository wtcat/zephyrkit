# SPDX-License-Identifier: Apache-2.0
zephyr_include_directories(cyttsp5_mcu/)
zephyr_sources_ifdef(CONFIG_KSCAN_CYTTSP5
    cyttsp5_mcu/pt_mcu_port.c
    cyttsp5_mcu/pt_mcu_tma5xx.c
)
zephyr_sources_ifdef(CONFIG_GPIO_KEYS gpio_keys.c)


