# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_compile_options(-Og)
add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M common/cortex_m)
add_subdirectory(CMSIS)

if(SOC_FAMILY)
  add_subdirectory(${SOC_FAMILY})
else()
  add_subdirectory(${SOC_NAME})
endif()

