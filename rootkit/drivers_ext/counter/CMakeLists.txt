# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_COUNTER_RTC_APOLLO3P counter_apollo3p.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE counter_handlers.c)
