#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources_ifdef(CONFIG_SHELL_BT_NUS shell_bt_nus.c)
zephyr_sources_ifdef(CONFIG_SHELL_DBG shell_dbg.c)
