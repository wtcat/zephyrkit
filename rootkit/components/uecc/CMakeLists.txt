# Components

zephyr_library_named(uecc)

zephyr_include_directories(.)

zephyr_library_sources(
  uECC.c
  uECC_ll.c
)
