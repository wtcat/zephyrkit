if (CONFIG_GUIX_USER_MODE)
zephyr_library_named(guix_app)

#zephyr_library_app_memory(guix_partition)
endif()

zephyr_library_sources(
	weather.c
	stopwatch.c
	settings.c
	message.c
	#map.c
	keyboard.c
	healthy.c
	guix_smart_watch_specifications.c
	#game.c
	#expression_evaluate.c
	demo_guix_smart_watch.c
	custom_pixelmap_button_draw.c
	custom_checkbox.c
	contact_info.c
	contact.c
	clock_add.c
	clock.c
	#calculator.c
	alarm.c
        scroll_wheel.c
)

if (NOT CONFIG_GUI_SPLIT_BINRES)
zephyr_library_sources(
	guix_smart_watch_resources.c
)

endif ()

