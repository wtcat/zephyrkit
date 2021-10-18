
zephyr_sources_ifdef(CONFIG_STP
  stp_core.c
  stp.c
  opc_proto.c
  opc_chan.c
)

add_subdirectory_ifdef(CONFIG_PROTOBUF app)
add_subdirectory_ifdef(CONFIG_STP_OTA ota)
add_subdirectory_ifdef(CONFIG_PROTOBUF proto)

zephyr_sources_ifdef(CONFIG_STP_UART stp_uart.c)
zephyr_sources_ifdef(CONFIG_STP_LOOPBACK stp_loopback.c)

if (CONFIG_BT)
  zephyr_sources_ifdef(CONFIG_STP_BT stp_bt.c)
endif()

if (CONFIG_STP_RBTREE)
  zephyr_linker_sources(RAM_SECTIONS stp.ld)
else()
  zephyr_linker_sources(RAM_SECTIONS stp_rom.ld)
endif()
