/*
 * Copyright(C) 2021
 * Author: wtcat
 */

#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>

#include <zephyr.h>
#include <device.h>
#include <drivers/kscan.h>

#include <bluetooth/gatt.h>
#include <bluetooth/services/bas.h>
#include <bluetooth/services/hrs.h>

#include <usb/usb_device.h>
#include <logging/log.h>

#include "component/bt/bthost_init.h"
#include "blkdev/bdbuf.h"

LOG_MODULE_REGISTER(main);

static void bas_notify(void) {
	uint8_t battery_level = bt_bas_get_battery_level();
	battery_level--;
	if (!battery_level)
		battery_level = 100U;
	bt_bas_set_battery_level(battery_level);
}

static void hrs_notify(void) {
	static uint8_t heartrate = 90U;
	/* Heartrate measurements simulation */
	heartrate++;
	if (heartrate == 160U)
		heartrate = 90U;
	bt_hrs_notify(heartrate);
}

static void keyboard_notify(const struct device *dev, uint32_t row, 
    uint32_t column, bool pressed) {
    printk("Key(%u): %s\n", row, pressed? "Pressed": "Released");
}

int main(void) {
	int err;

	bt_host_startup();
	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
    const struct device *key = device_get_binding("buttons");
    if (key) {
        kscan_config(key, keyboard_notify);
        kscan_enable_callback(key);
    }

	err = usb_enable(NULL);
	if (err != 0) {
		LOG_ERR("Failed to enable USB");
	}

	for ( ; ; ) {
		k_sleep(K_SECONDS(1));

		/* Heartrate measurements simulation */
		hrs_notify();

		/* Battery level simulation */
		bas_notify();
	}
	return 0;
}
