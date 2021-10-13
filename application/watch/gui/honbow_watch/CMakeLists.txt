zephyr_library_include_directories(
  .
  watch_face_theme
  alarm
  utils
  app_alarm
  app_compass
  app_setting
  app_list
  app_sports
  control_center
  home
  view_service
  data_service
  setting_service
  home/widgets
  app_today
  app_reminders
  app_music
  windows_manager
  app_spo2
  app_heart
  app_breath
  app_timer
)

zephyr_library_sources(
	view_service/view_service_custom.c
	view_service/view_service_display.c
	view_service/view_service_night_mode.c
	view_service/view_service_misc.c
	view_service/view_service_alarm.c
	view_service/view_service_reminders.c
	view_service/view_service_stop_watch.c
	view_service/view_service_rtc_setting.c
)

zephyr_library_sources(
	data_service/data_service_custom.c
)

zephyr_library_sources(
	setting_service/setting_service_custom.c
)

zephyr_library_sources(
	windows_manager/windows_manager.c
)


zephyr_library_sources(
	utils/custom_checkbox.c
	utils/custom_util.c
	utils/custom_pixelmap_button_draw.c
	utils/custom_vertical_total_rows.c
	utils/custom_horizontal_total_rows.c
	utils/custom_binres_theme_load.c
	utils/custom_pixelmap_mirror.c
	utils/custom_icon_utils.c
	utils/custom_window_title.c
	utils/custom_rounded_button.c
	utils/custom_animation.c
	utils/custom_widget_scrollable.c
	utils/custom_radiobox.c
	utils/custom_vertical_scroll_limit.c
	utils/custom_slidebutton_basic.c
	utils/custom_circle_button_draw.c
	utils/custom_slide_delete_widget.c
	utils/custom_checkbox_basic.c
	utils/custom_scrollbar_draw.c
	utils/custom_screen_stack.c
)

zephyr_library_sources(
	watch_face_theme/watch_face_port.c
	watch_face_theme/watch_face_theme.c
	watch_face_theme/watch_face_theme_draw.c
	watch_face_theme/watch_face_theme_1_conf.c
	watch_face_theme/watch_face_manager.c
	watch_face_theme/watch_face_theme_customize.c
)

zephyr_library_sources(
	home/wf_window.c
	home/wf_list_window.c
	home/home_window.c
)

zephyr_library_sources(
	home/widgets/music_widget.c
	home/widgets/home_widget_misc.c
	home/widgets/compass_widget.c
	home/widgets/timer_widget.c
	home/widgets/stopwatch_widget.c
)

zephyr_library_sources(
	app_list/app_list_window.c
	app_list/app_list_style_grid_view.c
	app_list/app_list_define.c
)

zephyr_library_sources(
	app_music/app_music_window.c
	app_music/app_music_main_page.c
)

zephyr_library_sources(
	app_alarm/app_alarm_window.c
	app_alarm/app_alarm_main_page.c
	app_alarm/app_alarm_time_select.c
	app_alarm/app_alarm_page_mgr.c
	app_alarm/app_alarm_rpt_select.c
	app_alarm/app_alarm_vib_duration.c
	app_alarm/app_alarm_edit_page.c
)

zephyr_library_sources(
	app_reminders/app_reminders_window.c
	app_reminders/app_reminders_main_page.c
	app_reminders/app_reminders_page_mgr.c
	app_reminders/app_reminders_time_select.c
	app_reminders/app_reminders_rpt_select.c
	app_reminders/app_reminders_vib_duration.c
	app_reminders/app_reminders_edit_page.c
)

zephyr_library_sources(
	app_sports/app_sports_window.c 
	app_sports/app_outdoor_run_window.c
	app_sports/app_gps_connect.c
	app_sports/app_sport_start_window.c
	app_sports/app_sport_data_window.c
	app_sports/app_sport_paused_window.c
	app_sports/app_sport_summary_window.c
	app_sports/sport_time_short_window.c
	app_sports/sport_goal_set_window.c
)

zephyr_library_sources(
	app_today/app_today_window.c 	
	app_today/app_sleep_data_window.c
	app_today/today_sport_data_window.c
)

# zephyr_library_sources(
# 	app_heart/app_heart_window.c 
# 	app_heart/app_heart_data_window.c
# )

# zephyr_library_sources(
# 	app_spo2/app_spo2_window.c
# 	app_spo2/app_spo2_measuring_window.c
# 	app_spo2/app_spo2_result_window.c
# 	app_spo2/spo2_measuring_fail_window.c
# )

# zephyr_library_sources(
# 	app_breath/app_breath_window.c
# 	app_breath/app_breath_attention_window.c
# 	app_breath/app_breath_inhale_window.c	
# 	app_breath/app_breath_result_window.c
# 	app_breath/app_breath_time_set_window.c
# 	app_breath/app_breath_freq_set_window.c
# )

zephyr_library_sources(
	guix_simple_specifications.c
    demo_guix_main.c
	control_center/control_center_window.c
	app_compass/app_compass_window.c
)

zephyr_library_sources(
	app_setting/app_setting_window.c
	app_setting/app_setting_main_page.c
	app_setting/app_setting_disp_page.c
	app_setting/app_setting_brightness_page.c
	app_setting/app_setting_bright_time.c
	app_setting/app_setting_wrist_raise.c
	app_setting/app_setting_night_mode.c
	app_setting/app_setting_eco_mode.c
	app_setting/app_setting_list_style.c
	app_setting/app_setting_dnd_mode.c
	app_setting/app_setting_vibration_strength.c
	app_setting/app_setting_activity_detect_page.c
	app_setting/app_setting_music_control_page.c
	app_setting/app_setting_system_page.c
	app_setting/app_setting_system_reboot_page.c
	app_setting/app_setting_system_shutdown_page.c
	app_setting/app_setting_system_factory_reset_page.c
)

zephyr_library_sources(
	app_timer/app_timer_window.c
	app_timer/app_timer_select_page.c
	app_timer/app_timer_language.c
	app_timer/app_timer_main_page.c
	app_timer/app_timer_jump_mgr.c
	app_timer/app_timer_custom_page.c
)

zephyr_library_sources(
	app_stop_watch/app_stop_watch_window.c
	app_stop_watch/app_stop_watch_language.c
	app_stop_watch/app_stop_watch_main_page.c
)

zephyr_library_sources(
  popup/alarm_remind_widget.c
  popup/charging_widget.c
  popup/goal_arrived.c
  popup/hrov_widget.c
  popup/language_win.c
  popup/lowbat_widget.c
  popup/message_widget.c
  popup/ota_widget.c
  popup/pairing_win.c
  popup/ringing_widget.c
  popup/sedentary_widget.c
  popup/shutdown_ani_widget.c
  popup/shutdown_widget.c
  popup/sport_wiget.c
  popup/ev_guix.c
)

zephyr_library_sources(
	qrcode/qrcodegen.c
)

zephyr_library_sources(
	guix_language_resources_custom.c
	guix_simple_resources.c
)

zephyr_linker_sources(RAM_SECTIONS app_info.ld)

