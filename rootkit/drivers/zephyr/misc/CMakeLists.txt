# Misc driver

zephyr_sources_ifdef(CONFIG_GUI_GPIO_KEY  apollo3p_gpio_keys.c)
zephyr_sources_ifdef(CONFIG_LCD_RM69330  rm6903_lcd.c)
zephyr_sources_ifdef(CONFIG_LCD_RM69092  rm69092_lcd.c)
zephyr_sources_ifdef(CONFIG_TP_APOLLO3P   ite7259_tp.c)
zephyr_sources_ifdef(CONFIG_MSPI_APOLLO3P mspi_apollo3p.c)
add_subdirectory_ifdef(CONFIG_TP_CYTTSP5_APOLLO3P cyttsp5_mcu)

