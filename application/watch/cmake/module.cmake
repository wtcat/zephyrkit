#Cmake for zephyr module

#Application modules(For userspace)
list(APPEND ZEPHYR_EXTRA_MODULES ${ZEPHYR_BASE}}/../rootkit)
list(APPEND ZEPHYR_EXTRA_MODULES ${ZEPHYR_BASE}/../rootkit/drivers)
list(APPEND ZEPHYR_EXTRA_MODULES ${ZEPHYR_BASE}/../rootkit/drivers_ext)
list(APPEND SYSCALL_INCLUDE_DIRS ${ZEPHYR_BASE}/../rootkit/include/drivers_ext)
