#Cmake for zephyr module

#Application modules(For userspace)
list(APPEND ZEPHYR_EXTRA_MODULES $ENV{ZEPHYR_BASE}/../rootkit)
list(APPEND SYSCALL_INCLUDE_DIRS $ENV{ZEPHYR_BASE}/../rootkit/include/drivers_ext)
