# Components

zephyr_sources(
  observer.c
)

# zephyr_sources_ifdef(CONFIG_GUIX  mac_addr.c)
zephyr_sources_ifdef(CONFIG_GUIX  evmgr.c)
zephyr_sources_ifdef(CONFIG_GUIX  ev_shell.c)