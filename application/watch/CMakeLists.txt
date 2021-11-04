# Cmake version require
cmake_minimum_required(VERSION 3.13.1)

# Modules path
include(../cmake/modules.cmake)
list(APPEND ZEPHYR_EXTRA_MODULES 
    ${ZEPHYR_BASE}/../rootkit/drivers
#    ${ZEPHYR_BASE}/../rootkit/drivers_ext
)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Project name
project(watch)

target_sources(app PRIVATE version.c empty.c)

# Source files
add_subdirectory(gui)
add_subdirectory(testsuite)

