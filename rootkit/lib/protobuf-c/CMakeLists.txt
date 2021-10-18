if (CONFIG_PROTOBUF)
  zephyr_library_named(protobuf)

  zephyr_library_sources(
    protobuf-c.c
  )
endif()

