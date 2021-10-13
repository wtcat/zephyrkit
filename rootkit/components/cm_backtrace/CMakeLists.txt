
if (CONFIG_CORTEXM_BACKTRACE)
  zephyr_library_named(cm_backtrace)
  zephyr_library_sources(cm_backtrace.c)
endif()