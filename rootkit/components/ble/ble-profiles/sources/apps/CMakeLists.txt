# Components

zephyr_include_directories(
  app
  .
)

zephyr_library_sources(
  app/common/app_db.c
  app/common/app_hw.c
  app/common/app_ui.c
  app/common/ui_console.c
  app/common/ui_lcd.c
  app/common/ui_main.c
  app/common/ui_platform.c
  app/common/ui_timer.c

  app/app_disc.c
  app/app_main.c
  app/app_master.c
  app/app_master_ae.c
  app/app_master_leg.c
  app/app_server.c
  app/app_slave.c
  app/app_slave_ae.c
  app/app_slave_leg.c
  app/app_terminal.c
)

