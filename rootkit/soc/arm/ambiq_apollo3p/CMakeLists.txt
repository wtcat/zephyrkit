# SPDX-License-Identifier: Apache-2.0

#zephyr_library()

add_subdirectory(AmbiqMicro)
add_subdirectory(apollo_mcu_lib)
add_subdirectory(CMSIS)

zephyr_library_sources(
  soc.c
  psram_section.c
)

zephyr_library_sources_ifdef(CONFIG_PM power.c)

zephyr_linker_sources(SECTIONS ccm.ld)
zephyr_linker_sources(SECTIONS psram.ld)


