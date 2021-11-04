/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_FLASH_APOLLO_H__
#define __SOC_FLASH_APOLLO_H__

#include <kernel.h>

#define START_PAGE_INST_0       6
#define START_ADDR_INST_0       0xC000

//
// Macros to describe the flash
//
#define FLASH_ADDR              AM_HAL_FLASH_ADDR
#define FLASH_INST_SIZE         AM_HAL_FLASH_INSTANCE_SIZE
#define FLASH_INST_WORDS        (AM_HAL_FLASH_INSTANCE_SIZE / 4)
#define FLASH_PAGE_SIZE         AM_HAL_FLASH_PAGE_SIZE
#define FLASH_INST_NUM          AM_HAL_FLASH_NUM_INSTANCES

//
// Flash row macros (specific to the particular flash architecture).
//
#define FLASH_ROW_WIDTH_BYTES   AM_HAL_FLASH_ROW_WIDTH_BYTES
#define FLASH_ROW_WIDTH_WDS     (AM_HAL_FLASH_ROW_WIDTH_BYTES / 4)
#define INFO_ROW_WIDTH_WDS      FLASH_ROW_WIDTH_WDS
#define FLASH_INST_ROWS_NUM     (FLASH_INST_WORDS / FLASH_ROW_WIDTH_WDS)


#endif /* !__SOC_FLASH_NRF_H__ */
