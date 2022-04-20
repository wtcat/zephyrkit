# Debugger component
zephyr_library_named(debugger)

zephyr_library_sources_ifdef(CONFIG_CM_BACKTRACE   cm_backtrace.c)
zephyr_library_sources_ifdef(CONFIG_CM_WATCHPOINT  core_debug.c)

if (CONFIG_CM_UNWIND_BACKTRACE)
zephyr_compile_options(
    -funwind-tables
    -mpoke-function-name
)
target_sources(debugger
    PRIVATE
    unwind.c
)
endif()



