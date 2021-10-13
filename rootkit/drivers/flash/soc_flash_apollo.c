/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Linaro Limited
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sys/printk.h"
#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/flash.h>
#include <stdint.h>
#include <string.h>
#include "soc_flash_apollo.h"

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(flash_apollo);

#define DT_DRV_COMPAT ambiq_apollo3p_flash_controller


static const struct flash_parameters flash_apollo_parameters = {
	.write_block_size = 4,
	.erase_value = 0xff,
};

#if defined(CONFIG_MULTITHREADING)
/* semaphore for locking flash resources (tickers) */
static struct k_sem sem_lock;
#define SYNC_INIT() k_sem_init(&sem_lock, 1, 1)
#define SYNC_LOCK() k_sem_take(&sem_lock, K_FOREVER)
#define SYNC_UNLOCK() k_sem_give(&sem_lock)
#else
#define SYNC_INIT()
#define SYNC_LOCK()
#define SYNC_UNLOCK()
#endif


static int write(off_t addr, const void *data, size_t len);
static int erase(uint32_t addr, uint32_t size);

static inline bool is_aligned_32(uint32_t data)
{
	return (data & 0x3) ? false : true;
}

static inline bool is_regular_addr_valid(off_t addr, size_t len)
{
	uint32_t ui32InstNum = AM_HAL_FLASH_ADDR2INST(addr);

	if (!is_aligned_32(addr) || (len % sizeof(uint32_t))) {
		LOG_ERR("not word-aligned: 0x%08lx:%zu",
				(unsigned long)addr, len);
		return false;
	}

    if ( ( ui32InstNum < AM_HAL_FLASH_NUM_INSTANCES ) || ( ui32InstNum == 0xA11 ) )
    {
		return true;
    }
    else
    {
        return false;
    }
}

static int apollo_flash_read(const struct device *dev, off_t addr,
			    void *data, size_t len)
{
	//TODO: check addr is valid
	if (!is_regular_addr_valid(addr, len))
	{
		return 0;
	}

	if (!len) {
		return 0;
	}

	memcpy(data, (void *)addr, len);

	return 0;
}

static int apollo_flash_write(const struct device *dev, off_t addr,
			     const void *data, size_t len)
{
	int ret;
	if (!is_regular_addr_valid(addr, len))
	{
		return 0;
	}

	if (!len) {
		return 0;
	}

	SYNC_LOCK();

	ret = write(addr, data, len);

	SYNC_UNLOCK();

	return ret;
}

static int apollo_flash_erase(const struct device *dev, off_t addr, size_t size)
{
	int ret;
	//TODO: check addr is valid or not
	if (!is_regular_addr_valid(addr, size))
	{
		return 0;
	}

	SYNC_LOCK();
	ret = erase(addr, size);
	SYNC_UNLOCK();

	return ret;
}

static int apollo_flash_write_protection(const struct device *dev, bool enable)
{
	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static struct flash_pages_layout dev_layout;

static void apollo_flash_pages_layout(const struct device *dev,
				     const struct flash_pages_layout **layout,
				     size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
apollo_flash_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_apollo_parameters;
}

static const struct flash_driver_api apollo_flash_api = {
	.read = apollo_flash_read,
	.write = apollo_flash_write,
	.erase = apollo_flash_erase,
	.write_protection = apollo_flash_write_protection,
	.get_parameters = apollo_flash_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = apollo_flash_pages_layout,
#endif
};

static int apollo_flash_init(const struct device *dev)
{
	SYNC_INIT();
	printk("apollo soc flash drv init!\n");

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	dev_layout.pages_count = AM_HAL_FLASH_INSTANCE_PAGES * 4 ;
	dev_layout.pages_size = AM_HAL_FLASH_PAGE_SIZE;
#endif

	return 0;
}

DEVICE_DT_DEFINE(DT_DRV_INST(0), apollo_flash_init, device_pm_control_nop,
		 NULL, NULL,
		 POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		 &apollo_flash_api);

static int erase(uint32_t addr, uint32_t size)
{
	uint32_t len_remianing = size;
	uint32_t addr_curr = addr;

	if (AM_HAL_FLASH_ADDR2INST(addr_curr) == 0) {
		if (AM_HAL_FLASH_ADDR2PAGE(addr_curr) < START_PAGE_INST_0) {
			printk("[error] erar flash instance 0, page num  must < 6!\n");
			return EIO;
		}
	}

	do {
		uint32_t offset_from_page_start = addr_curr % AM_HAL_FLASH_PAGE_SIZE;
		if (offset_from_page_start){
			printk("[waring]addr is not page start addr, some data may lose!\n");
		}
		
		int32_t i32ReturnCode = am_hal_flash_page_erase( AM_HAL_FLASH_PROGRAM_KEY,
							AM_HAL_FLASH_ADDR2INST(addr_curr), AM_HAL_FLASH_ADDR2PAGE(addr_curr) );
		
		if (i32ReturnCode) {
			printk("FLASH erase page num: 0x%08x "
						"i32ReturnCode = 0x%x.\n",
						AM_HAL_FLASH_ADDR2ABSPAGE(addr_curr),
						i32ReturnCode);
			return EIO;
		}
		addr_curr += (AM_HAL_FLASH_PAGE_SIZE - offset_from_page_start);
		if (len_remianing > (AM_HAL_FLASH_PAGE_SIZE - offset_from_page_start)) {
			len_remianing -= (AM_HAL_FLASH_PAGE_SIZE - offset_from_page_start);
		} else {
			len_remianing = 0;
		}
	} while (len_remianing > 0);

	return 0;
}

static int write(off_t addr, const void *data, size_t len)
{
	uint32_t *pui32Dst;
	int32_t i32ReturnCode;

	uint32_t len_of_words = len / 4;

	if (len_of_words <= 0) {
		printk("[warning] num of words = 0.\n");
	}

	pui32Dst = (uint32_t *) addr;
    i32ReturnCode = am_hal_flash_program_main(AM_HAL_FLASH_PROGRAM_KEY,
                                              (uint32_t *)data,
                                              pui32Dst,
                                              len_of_words);

    if (i32ReturnCode) {
        printk("FLASH program page at 0x%08x "
                             "i32ReturnCode = 0x%x.\n",
                             addr,
                             i32ReturnCode);
		return EIO;
    } else {
		return 0;
	}
}
