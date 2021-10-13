#
# Copyright (c) 2018 - 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_BT)
  add_subdirectory(bluetooth)
endif()
  add_subdirectory_ifdef(CONFIG_NVS_CUSTOM    nvs)
  add_subdirectory_ifdef(CONFIG_SETTING_USE_NVS_CUSTOM  setting)
