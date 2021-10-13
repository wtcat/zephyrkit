/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Arm Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <sys/__assert.h>
#include <drivers/flash.h>
#include <drivers/timer/system_timer.h>
#include <usb/usb_device.h>
#include <soc.h>
#include <linker/linker-defs.h>

#include "kernel.h"
#include "storage/flash_map.h"
#include "sys/printk.h"
#include "target.h"

#include "bootutil/bootutil_log.h"
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/fault_injection_hardening.h"
#include "flash_map_backend/flash_map_backend.h"

#include "boot_event.h"

#ifdef CONFIG_MCUBOOT_SERIAL
#include "boot_serial/boot_serial.h"
#include "serial_adapter/serial_adapter.h"

const struct boot_uart_funcs boot_funcs = {
	.read = console_read,
	.write = console_write
};
#endif

#ifdef CONFIG_BOOT_WAIT_FOR_USB_DFU
#include <usb/class/usb_dfu.h>
#endif

#if CONFIG_MCUBOOT_CLEANUP_ARM_CORE
#include <arm_cleanup.h>
#endif

#if defined(CONFIG_LOG) && !defined(CONFIG_LOG_IMMEDIATE)
#ifdef CONFIG_LOG_PROCESS_THREAD
#warning "The log internal thread for log processing can't transfer the log"\
		 "well for MCUBoot."
#else
#include <logging/log_ctrl.h>

#define BOOT_LOG_PROCESSING_INTERVAL K_MSEC(30) /* [ms] */

/* log are processing in custom routine */
K_THREAD_STACK_DEFINE(boot_log_stack, CONFIG_MCUBOOT_LOG_THREAD_STACK_SIZE);
struct k_thread boot_log_thread;
volatile bool boot_log_stop = false;
K_SEM_DEFINE(boot_log_sem, 1, 1);

/* log processing need to be initalized by the application */
#define ZEPHYR_BOOT_LOG_START() zephyr_boot_log_start()
#define ZEPHYR_BOOT_LOG_STOP() zephyr_boot_log_stop()
#endif /* CONFIG_LOG_PROCESS_THREAD */
#else
/* synchronous log mode doesn't need to be initalized by the application */
#define ZEPHYR_BOOT_LOG_START() do { } while (false)
#define ZEPHYR_BOOT_LOG_STOP() do { } while (false)
#endif /* defined(CONFIG_LOG) && !defined(CONFIG_LOG_IMMEDIATE) */

#ifdef CONFIG_SOC_FAMILY_NRF
#include <hal/nrf_power.h>

static inline bool boot_skip_serial_recovery()
{
#if NRF_POWER_HAS_RESETREAS
	uint32_t rr = nrf_power_resetreas_get(NRF_POWER);

	return !(rr == 0 || (rr & NRF_POWER_RESETREAS_RESETPIN_MASK));
#else
	return false;
#endif
}
#else
static inline bool boot_skip_serial_recovery()
{
	return false;
}
#endif

MCUBOOT_LOG_MODULE_REGISTER(mcuboot);

void os_heap_init(void);

#if defined(CONFIG_ARM)

#ifdef CONFIG_SW_VECTOR_RELAY
extern void *_vector_table_pointer;
#endif

struct arm_vector_table {
	uint32_t msp;
	uint32_t reset;
};

extern void sys_clock_disable(void);

static void do_boot(struct boot_rsp *rsp)
{
	struct arm_vector_table *vt;
	uintptr_t flash_base;
	int rc;

	/* The beginning of the image is the ARM vector table, containing
	 * the initial stack pointer address and the reset vector
	 * consecutively. Manually set the stack pointer and jump into the
	 * reset vector
	 */
	rc = flash_device_base(rsp->br_flash_dev_id, &flash_base);
	assert(rc == 0);

	#if 1
	vt = (struct arm_vector_table *)(flash_base +
									 rsp->br_image_off +
									 rsp->br_hdr->ih_hdr_size);
	#else
	vt = (struct arm_vector_table *)(rsp->br_image_off + rsp->br_hdr->ih_hdr_size);

	#endif

	irq_lock();
#ifdef CONFIG_SYS_CLOCK_EXISTS
	sys_clock_disable();
#endif
#ifdef CONFIG_USB
	/* Disable the USB to prevent it from firing interrupts */
	usb_disable();
#endif
#if CONFIG_MCUBOOT_CLEANUP_ARM_CORE
	cleanup_arm_nvic(); /* cleanup NVIC registers */

#ifdef CONFIG_CPU_CORTEX_M7
	/* Disable instruction cache and data cache before chain-load the application */
	SCB_DisableDCache();
	SCB_DisableICache();
#endif

#if CONFIG_CPU_HAS_ARM_MPU
	z_arm_clear_arm_mpu_config();
#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD) && \
	defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	/* Reset limit registers to avoid inflicting stack overflow on image
	 * being booted.
	 */
	__set_PSPLIM(0);
	__set_MSPLIM(0);
#endif

#endif /* CONFIG_MCUBOOT_CLEANUP_ARM_CORE */

#ifdef CONFIG_BOOT_INTR_VEC_RELOC
#if defined(CONFIG_SW_VECTOR_RELAY)
	_vector_table_pointer = vt;
#ifdef CONFIG_CPU_CORTEX_M_HAS_VTOR
	SCB->VTOR = (uint32_t)__vector_relay_table;
#endif
#elif defined(CONFIG_CPU_CORTEX_M_HAS_VTOR)
	SCB->VTOR = (uint32_t)vt;
#endif /* CONFIG_SW_VECTOR_RELAY */
#else /* CONFIG_BOOT_INTR_VEC_RELOC */
#if defined(CONFIG_CPU_CORTEX_M_HAS_VTOR) && defined(CONFIG_SW_VECTOR_RELAY)
	_vector_table_pointer = _vector_start;
	SCB->VTOR = (uint32_t)__vector_relay_table;
#endif
#endif /* CONFIG_BOOT_INTR_VEC_RELOC */

	__set_MSP(vt->msp);
#if CONFIG_MCUBOOT_CLEANUP_ARM_CORE
	__set_CONTROL(0x00); /* application will configures core on its own */
	__ISB();
#endif
	((void (*)(void))vt->reset)();
}

#elif defined(CONFIG_XTENSA)
#define SRAM_BASE_ADDRESS	0xBE030000

static void copy_img_to_SRAM(int slot, unsigned int hdr_offset)
{
	const struct flash_area *fap;
	int area_id;
	int rc;
	unsigned char *dst = (unsigned char *)(SRAM_BASE_ADDRESS + hdr_offset);

	BOOT_LOG_INF("Copying image to SRAM");

	area_id = flash_area_id_from_image_slot(slot);
	rc = flash_area_open(area_id, &fap);
	if (rc != 0) {
		BOOT_LOG_ERR("flash_area_open failed with %d\n", rc);
		goto done;
	}

	rc = flash_area_read(fap, hdr_offset, dst, fap->fa_size - hdr_offset);
	if (rc != 0) {
		BOOT_LOG_ERR("flash_area_read failed with %d\n", rc);
		goto done;
	}

done:
	flash_area_close(fap);
}

/* Entry point (.ResetVector) is at the very beginning of the image.
 * Simply copy the image to a suitable location and jump there.
 */
static void do_boot(struct boot_rsp *rsp)
{
	void *start;

	BOOT_LOG_INF("br_image_off = 0x%x\n", rsp->br_image_off);
	BOOT_LOG_INF("ih_hdr_size = 0x%x\n", rsp->br_hdr->ih_hdr_size);

	/* Copy from the flash to HP SRAM */
	copy_img_to_SRAM(0, rsp->br_hdr->ih_hdr_size);

	/* Jump to entry point */
	start = (void *)(SRAM_BASE_ADDRESS + rsp->br_hdr->ih_hdr_size);
	((void (*)(void))start)();
}

#else
/* Default: Assume entry point is at the very beginning of the image. Simply
 * lock interrupts and jump there. This is the right thing to do for X86 and
 * possibly other platforms.
 */
static void do_boot(struct boot_rsp *rsp)
{
	uintptr_t flash_base;
	void *start;
	int rc;

	rc = flash_device_base(rsp->br_flash_dev_id, &flash_base);
	assert(rc == 0);

	start = (void *)(flash_base + rsp->br_image_off +
					 rsp->br_hdr->ih_hdr_size);

	/* Lock interrupts and dive into the entry point */
	irq_lock();
	((void (*)(void))start)();
}
#endif

#if defined(CONFIG_LOG) && !defined(CONFIG_LOG_IMMEDIATE) &&\
	!defined(CONFIG_LOG_PROCESS_THREAD)
/* The log internal thread for log processing can't transfer log well as has too
 * low priority.
 * Dedicated thread for log processing below uses highest application
 * priority. This allows to transmit all logs without adding k_sleep/k_yield
 * anywhere else int the code.
 */

/* most simple log processing theread */
void boot_log_thread_func(void *dummy1, void *dummy2, void *dummy3)
{
	(void)dummy1;
	(void)dummy2;
	(void)dummy3;

	 log_init();

	 while (1) {
			 if (log_process(false) == false) {
					if (boot_log_stop) {
						break;
					}
					k_sleep(BOOT_LOG_PROCESSING_INTERVAL);
			 }
	 }

	 k_sem_give(&boot_log_sem);
}

void zephyr_boot_log_start(void)
{
		/* start logging thread */
		k_thread_create(&boot_log_thread, boot_log_stack,
				K_THREAD_STACK_SIZEOF(boot_log_stack),
				boot_log_thread_func, NULL, NULL, NULL,
				K_HIGHEST_APPLICATION_THREAD_PRIO, 0,
				BOOT_LOG_PROCESSING_INTERVAL);

		k_thread_name_set(&boot_log_thread, "logging");
}

void zephyr_boot_log_stop(void)
{
	boot_log_stop = true;

	/* wait until log procesing thread expired
	 * This can be reworked using a thread_join() API once a such will be
	 * available in zephyr.
	 * see https://github.com/zephyrproject-rtos/zephyr/issues/21500
	 */
	(void)k_sem_take(&boot_log_sem, K_FOREVER);
}
#endif/* defined(CONFIG_LOG) && !defined(CONFIG_LOG_IMMEDIATE) &&\
		!defined(CONFIG_LOG_PROCESS_THREAD) */

#include "bootutil_priv.h"  //for test, get tlv length

void main(void)
{
	send_event_to_display(EVENT_STATUS_BOOT, EVENT_BOOT_ERRNO_NONE);
	k_sleep(K_MSEC(500));//logo time!

	struct boot_rsp rsp;
	int rc;
	fih_int fih_rc = FIH_FAILURE;

	MCUBOOT_WATCHDOG_FEED();

	BOOT_LOG_INF("Starting bootloader");

	os_heap_init();

	ZEPHYR_BOOT_LOG_START();

	(void)rc;

#if (!defined(CONFIG_XTENSA) && defined(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL))
	if (!flash_device_get_binding(DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL)) {
		BOOT_LOG_ERR("Flash device %s not found",
			 DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL);
		while (1)
			;
	}
#elif (defined(CONFIG_XTENSA) && defined(JEDEC_SPI_NOR_0_LABEL))
	if (!flash_device_get_binding(JEDEC_SPI_NOR_0_LABEL)) {
		BOOT_LOG_ERR("Flash device %s not found", JEDEC_SPI_NOR_0_LABEL);
		while (1)
			;
	}
#endif

#ifdef CONFIG_MCUBOOT_SERIAL

	struct device const *detect_port;
	uint32_t detect_value = !CONFIG_BOOT_SERIAL_DETECT_PIN_VAL;

	detect_port = device_get_binding(CONFIG_BOOT_SERIAL_DETECT_PORT);
	__ASSERT(detect_port, "Error: Bad port for boot serial detection.\n");

	/* The default presence value is 0 which would normally be
	 * active-low, but historically the raw value was checked so we'll
	 * use the raw interface.
	 */
	rc = gpio_pin_configure(detect_port, CONFIG_BOOT_SERIAL_DETECT_PIN,
#ifdef GPIO_INPUT
							GPIO_INPUT | GPIO_PULL_UP
#else
							GPIO_DIR_IN | GPIO_PUD_PULL_UP
#endif
		   );
	__ASSERT(rc == 0, "Error of boot detect pin initialization.\n");

#ifdef GPIO_INPUT
	rc = gpio_pin_get_raw(detect_port, CONFIG_BOOT_SERIAL_DETECT_PIN);
	detect_value = rc;
#else
	rc = gpio_pin_read(detect_port, CONFIG_BOOT_SERIAL_DETECT_PIN,
					   &detect_value);
#endif
	__ASSERT(rc >= 0, "Error of the reading the detect pin.\n");
	if (detect_value == CONFIG_BOOT_SERIAL_DETECT_PIN_VAL &&
		!boot_skip_serial_recovery()) {
		BOOT_LOG_INF("Enter the serial recovery mode");
		rc = boot_console_init();
		__ASSERT(rc == 0, "Error initializing boot console.\n");
		boot_serial_start(&boot_funcs);
		__ASSERT(0, "Bootloader serial process was terminated unexpectedly.\n");
	}
#endif

#ifdef CONFIG_BOOT_WAIT_FOR_USB_DFU
	rc = usb_enable(NULL);
	if (rc) {
		BOOT_LOG_ERR("Cannot enable USB");
	} else {
		BOOT_LOG_INF("Waiting for USB DFU");
		wait_for_usb_dfu();
		BOOT_LOG_INF("USB DFU wait time elapsed");
	}
#endif

#if CONFIG_UPDATE_IMAGE_FROM_EXT_FLASH
	//new add api for update from ext flash
	bool check_update_valid(const struct flash_area * src, const struct flash_area * dst);
	fih_int check_img_valid(const struct flash_area * dst, struct image_header * hdr);
	fih_int img_update_process(const struct flash_area * src, const struct image_header * src_hdr ,
							const struct flash_area * dst);
	/********************************************************************************************
	*step 1: get flash_area info from soc and ext flash.
	*step 2: get hdr info from flash area, check need update or not by hdr info.
	*step 3: if need update, check ext flash's img valid or not, if valid ,update it to soc flash.
	*********************************************************************************************/
	static const struct flash_area *_fa_p_ext_flash;
	fih_rc = flash_area_open(FLASH_AREA_ID(update_img), &_fa_p_ext_flash);
	if (fih_rc) {
		BOOT_LOG_ERR("flash area open fail! rc = %d\n", fih_rc);
		goto boot_normal;
	}

	static const struct flash_area *_fa_p_soc_flash;
	fih_rc = flash_area_open(FLASH_AREA_ID(image_0), &_fa_p_soc_flash);
	if (fih_rc) {
		BOOT_LOG_ERR("flash area open fail! rc = %d\n", fih_rc); // should never happen!
		FIH_PANIC;
	}

	//flash_area_read、flash_area_writ api havn't check binding ok or not!
	//check it avoid kernel fault.
	if (NULL == device_get_binding(_fa_p_ext_flash->fa_dev_name)) {
		BOOT_LOG_ERR("ext flash drv binding fail,skip update check!\n"); 
		goto boot_normal;
	}

	//TODO: add force update for regression test?
	if (false == check_update_valid(_fa_p_ext_flash, _fa_p_soc_flash)) {
		BOOT_LOG_INF("no need to update!\n");
	} else {
		struct image_header _hdr_src = { 0 };
		if ( FIH_SUCCESS == check_img_valid(_fa_p_ext_flash, &_hdr_src)) {
			if (FIH_FAILURE  == img_update_process(_fa_p_ext_flash, &_hdr_src, _fa_p_soc_flash) ) {
				BOOT_LOG_ERR("write img fail to soc flash! \n");

				send_event_to_display(EVENT_STATUS_UPDATE_ERROR, EVENT_ERRNO_UPDATE_ERROR_WRITE);
				k_sleep(K_MSEC(1000));  //write to soc flash fail, machine possibly need to be repaired.

				FIH_PANIC;
			}
		} else {
			send_event_to_display(EVENT_STATUS_UPDATE_ERROR, EVENT_ERRNO_UPDATE_ERROR_IMG_NOT_VALID);
			k_sleep(K_MSEC(1000));//update img is not valid, we try boot normal!
		}
	}
#endif
	
boot_normal:
	FIH_CALL(boot_go, fih_rc, &rsp);
	if (fih_not_eq(fih_rc, FIH_SUCCESS)) {

		#if CONFIG_UPDATE_IMAGE_FROM_EXT_FLASH//if soc app img is not valid, check ext flash img for update.
		static uint8_t retry_cnt = 0;
		if (retry_cnt >= 2) {
			goto boot_error;
		}

		if (NULL == device_get_binding(_fa_p_ext_flash->fa_dev_name)) {
			BOOT_LOG_ERR("try to resume fail, ext flash drv binding fail!\n");
			goto boot_error;
		}
		
		BOOT_LOG_ERR("soc flash image is not avalid, try to resume from ext flash,retry cnt = %d! \n",
						 retry_cnt);

		struct image_header _hdr_src = { 0 };
		if ( FIH_SUCCESS == check_img_valid(_fa_p_ext_flash, &_hdr_src)) {
			if (FIH_FAILURE  == img_update_process(_fa_p_ext_flash, &_hdr_src, _fa_p_soc_flash) ) {
				goto boot_error;
			} else {
				retry_cnt++;
				goto boot_normal;
			}
		}

		#endif

		boot_error:
			send_event_to_display(EVENT_STATUS_BOOT, EVENT_BOOT_ERRNO_RESUME_FAIL);
			k_sleep(K_MSEC(1000));//give more time to display thread for show error info.

			BOOT_LOG_ERR("Unable to find bootable image");
			FIH_PANIC;
	}

	BOOT_LOG_INF("Bootloader chainload address offset: 0x%x",
				 rsp.br_image_off);

	BOOT_LOG_INF("Jumping to the first image slot");
	ZEPHYR_BOOT_LOG_STOP();
	do_boot(&rsp);

	BOOT_LOG_ERR("Never should get here");
	while (1)
		;
}
