/*
 * Copyright(C) 2021
 * Author: wtcat
 */

#include <errno.h>
#include <string.h>
#include <sys/byteorder.h>
#include <sys/printk.h>

#include <device.h>
#include <drivers/kscan.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/bas.h>
#include <bluetooth/services/hrs.h>

#include <logging/log.h>
#include <usb/usb_device.h>

#include "blkdev/bdbuf.h"
#include "component/bt/bthost_init.h"

#include "debugger/core_debug.h"

LOG_MODULE_REGISTER(main);

//#define USE_BREAKPOINT_TEST

extern void bt_apple_ancs_ready(int err);

#if defined(USE_BREAKPOINT_TEST)
volatile int __test_var = 1;
#endif

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
  printk("Key(%u): %s\n", row, pressed ? "Pressed" : "Released");
}

static void blkdev_test(void) {
  struct k_blkdev *bdev;
  char buffer[32] = {"Hello, world!"};
  bdev = k_blkdev_get("blkdev");
  if (!bdev)
    return;
  k_blkdev_write(bdev, buffer, strlen(buffer), 0);
  memset(buffer, 0, sizeof(buffer));
  k_blkdev_read(bdev, buffer, 13, 0);
  printk("%s\n", buffer);
}

int main(void) {
  int err;

#if defined(USE_BREAKPOINT_TEST)
  core_watchpoint_install(3, &__test_var, MEM_WRITE | MEM_SIZE_1);
#endif
  malloc(16);
  // blkdev_test();
  //err = bt_enable(bt_host_default_ready_cb);
   err = bt_enable(bt_apple_ancs_ready);
  if (err)
     LOG_ERR("Bluetooth initialize failed(%d)\n", err);
    
  /* Implement notification. At the moment there is no suitable way
   * of starting delayed work so we do it here
   */
  const struct device *key = device_get_binding("buttons");
  if (key) {
    kscan_config(key, keyboard_notify);
    kscan_enable_callback(key);
  }

#if defined(USE_BREAKPOINT_TEST)
  if (__test_var == 1)
    __test_var = 2;
  core_watchpoint_uninstall(3);
#endif
  err = usb_enable(NULL);
  if (err != 0) {
    LOG_ERR("Failed to enable USB");
  }
  for (;;) {
    k_sleep(K_SECONDS(1));

    // /* Heartrate measurements simulation */
    // hrs_notify();

    // /* Battery level simulation */
    // bas_notify();
  }
  return 0;
}
