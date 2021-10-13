# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

#list(APPEND ZEPHYR_EXTRA_MODULES ${ZEPHYR_BASE}/../hal_stm32)
#list(APPEND ZEPHYR_EXTRA_MODULES ${ZEPHYR_BASE}/../cmsis)
#list(APPEND ZEPHYR_EXTRA_MODULES ${ZEPHYR_BASE}/../hal_nordic)
list(APPEND ZEPHYR_EXTRA_MODULES ${ZEPHYR_BASE}/../rootkit)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)
