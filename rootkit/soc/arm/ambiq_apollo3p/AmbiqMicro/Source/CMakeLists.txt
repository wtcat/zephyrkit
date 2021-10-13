# Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_SOC_APOLLO1 system_apollo1.c)
zephyr_library_sources_ifdef(CONFIG_SOC_APOLLO2 system_apollo2.c)
zephyr_library_sources_ifdef(CONFIG_SOC_APOLLO3 system_apollo3.c)
zephyr_library_sources_ifdef(CONFIG_SOC_APOLLO3P system_apollo3p.c)

