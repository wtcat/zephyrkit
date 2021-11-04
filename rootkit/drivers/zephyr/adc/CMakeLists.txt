# ADC driver

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources_ifdef(CONFIG_ADC_APOLLO3P adc_apollo.c)

