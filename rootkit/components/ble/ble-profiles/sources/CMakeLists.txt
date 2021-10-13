# Components

zephyr_include_directories(
  include/app/
  services/
)

add_subdirectory(apps)
add_subdirectory(profiles)

zephyr_library_sources(
  services/svc_alert.c
  services/svc_batt.c
  services/svc_bps.c
  services/svc_core.c
  services/svc_cps.c
  services/svc_cscs.c
  services/svc_cte.c
  services/svc_dis.c
  services/svc_gls.c
  services/svc_gyro.c
  services/svc_hid.c
  services/svc_hrs.c
  services/svc_hts.c
  services/svc_ipss.c
  services/svc_plxs.c
  services/svc_px.c
  services/svc_rscs.c
  services/svc_scpss.c
  services/svc_temp.c
  services/svc_time.c
  services/svc_uricfg.c
  services/svc_wdxs.c
  services/svc_wp.c
  services/svc_wss.c
)

