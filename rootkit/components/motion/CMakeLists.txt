zephyr_include_directories(./)
zephyr_include_directories(./daily_sport_service/)


zephyr_library()


zephyr_library_sources_ifdef(CONFIG_MOTION cywee_motion.c)
zephyr_library_sources_ifdef(CONFIG_MOTION cywee_cmd.c)

zephyr_library_include_directories(daily_sport_service)

zephyr_library_sources(
	daily_sport_service/daily_sport_ctrl.c
	daily_sport_service/gui_sport_cmd.c
	#daily_sport_service/view_service_display.c
	#daily_sport_service/view_service_night_mode.c
	#daily_sport_service/view_service_misc.c
	#daily_sport_service/view_service_alarm.c
)

zephyr_library_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/CWM_LIB_gcc_m4.a)



