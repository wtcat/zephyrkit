zephyr_include_directories(
  profiles/amdtpc/
  profiles/amdtpcommon/
  profiles/amdtps/
  profiles/amota/
  profiles/ancc/
  profiles/vole/
  profiles/volecommon/
  services/
)

############profile###############
zephyr_library_sources(
  profiles/amdtpcommon/amdtp_common.c
  profiles/amdtps/amdtps_main.c
  #profiles/amdtpc/amdtpc_main.c
  #profiles/amota/amotas_main.c
  #profiles/ancc/ancc_main.c
  #profiles/vole/voles_main.c
  #profiles/volecommon/vole_common.c
)

##########service#############
zephyr_library_sources(
  services/svc_amdtp.c
  services/svc_amotas.c
  #services/svc_amvole.c
  #services/svc_cust.c
  #services/svc_throughput.c
)


##############APP DEMO##########
zephyr_include_directories(
  apps/amdtps/
)
zephyr_library_sources(
  apps/amdtps/amdtp_main.c
)
