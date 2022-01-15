/*
 * Copyright(C) 2021
 * Author: wtcat
 */

#include <sys/printk.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(bthost_init);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_DIS_VAL))
};

static void bt_connected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    LOG_ERR("Connection failed (err 0x%02x)\n", err);
  } else {
    LOG_INF("Connected\n");
  }
}

static void bt_disconnected(struct bt_conn *conn, uint8_t reason) {
  LOG_INF("Disconnected (reason 0x%02x)\n", reason);
}

static void auth_cancel(struct bt_conn *conn) {
  char addr[BT_ADDR_LE_STR_LEN];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  LOG_INF("Pairing cancelled: %s\n", addr);
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey) {
  char addr[BT_ADDR_LE_STR_LEN];
  char passkey_str[7];
  bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
  snprintk(passkey_str, 7, "%06u", passkey);
  printk("Passkey for %s: %s\n", addr, passkey_str);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = bt_connected,
    .disconnected = bt_disconnected,
};

static struct bt_conn_auth_cb auth_cb_display = {
    .cancel = auth_cancel, 
	.passkey_display = auth_passkey_display
};

void bt_host_default_ready_cb(int err) {
	if (err != 0)
		LOG_ERR("Bluetooth initialized failed with %d\n", err);
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}
	bt_conn_auth_cb_register(&auth_cb_display);
}
