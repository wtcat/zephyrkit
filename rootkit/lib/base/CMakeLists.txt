# Components

add_subdirectory(init)

zephyr_sources(
  observer.c
  timeconv.c
  mac_addr.c
  vibration.c
  evmgr.c
  ev_shell.c
)




