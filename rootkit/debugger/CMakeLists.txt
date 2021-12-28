# Debugger component
zephyr_library_named(debugger)

zephyr_library_sources_ifdef(CONFIG_CM_BACKTRACE   cm_backtrace.c)
zephyr_library_sources_ifdef(CONFIG_CM_WATCHPOINT  core_debug.c)

