# Components

zephyr_sources(
  observer.c
  system_fatal.c
)

# zephyr_sources_ifdef(CONFIG_GUIX  mac_addr.c)
zephyr_sources_ifdef(CONFIG_GUIX  evmgr.c)
zephyr_sources_ifdef(CONFIG_GUIX  ev_shell.c)

zephyr_linker_sources(SECTIONS base.ld)
