
zephyr_library_include_directories(
  ambiq
  ambiq/apollo3
)

zephyr_library_sources(
  ambiq/apollo3/hci_drv_apollo3.c
  
  ambiq/hci_cmd.c
  ambiq/hci_cmd_ae.c
  ambiq/hci_cmd_cte.c
  ambiq/hci_cmd_past.c
  ambiq/hci_cmd_phy.c
  ambiq/hci_core.c
  ambiq/hci_core_ps.c
  ambiq/hci_evt.c
  ambiq/hci_tr.c
  ambiq/hci_vs.c
  ambiq/hci_vs_ae.c
)

