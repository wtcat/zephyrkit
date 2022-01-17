#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>

#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/ancs_client.h>
#include <bluetooth/services/gattp.h>

#include <logging/log.h>
#include <settings/settings.h>

#include "bt_disc.h"
#include "apple_ancs.h"
#include "apple_ams.h"

LOG_MODULE_REGISTER(bt_user);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static struct bt_conn *user_conn;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN)
};


static void exchange_done(struct bt_conn *conn, uint8_t err,
	struct bt_gatt_exchange_params *params) {
    int ec;
    if (err != 0) {
        LOG_ERR("Gatt exchange parameters failed(%d)\n", (int)err);
        return;
    }
    LOG_INFO("MTU has been updated to %d\n", bt_gatt_get_mtu(conn));
#if 0
    struct bt_conn_info info;
    ec = bt_conn_get_info(conn, &info);
    if (ec) {
        LOG_ERR("Get connect information failed(%d"\n, ec);
        return;
    }
#endif
}

static int bt_user_exchange_mtu(struct bt_conn *conn) {
    static struct bt_gatt_exchange_params param = {
        .func = exchange_done
    };
	return bt_gatt_exchange_mtu(conn, &param);
}

static void bt_user_connected(struct bt_conn *conn, uint8_t err) {
	char addr[BT_ADDR_LE_STR_LEN];
    int ret;
	if (err) {
		LOG_ERR("Connection failed (err 0x%02x)\n", err);
		return;
	}
    user_conn = bt_conn_ref(conn);
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	ret = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (ret) {
		LOG_ERR("Failed to set security (err %d)\n", ret);
        return;
	}
    ret = bt_user_exchange_mtu(conn);
    if (ret) {
        LOG_ERR("GATT exchange mtu failed(%d)\n", ret);
        return;
    }
}

static void bt_user_disconnected(struct bt_conn *conn, uint8_t reason) {
	char addr[BT_ADDR_LE_STR_LEN];
    if (user_conn == conn) {
        bt_conn_unref(user_conn);
        user_conn = NULL;
    }
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Disconnected from %s (reason 0x%02x)\n", addr, reason);
}

static void bt_user_security_changed(struct bt_conn *conn, bt_security_t level,
	enum bt_security_err err) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	if (!err) {
		LOG_INF("Security changed: %s level %u\n", addr, level);
		if (bt_conn_get_security(conn) >= BT_SECURITY_L2) {
			bt_gattp_discovery_start(conn);
		}
	} else {
		LOG_ERR("Security failed: %s level %u err %d\n", 
			addr, level, err);  
	}
}

static void bt_user_auth_cancel(struct bt_conn *conn) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing cancelled: %s\n", addr);
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void bt_user_pairing_complete(struct bt_conn *conn, bool bonded) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void bt_user_pairing_failed(struct bt_conn *conn, 
	enum bt_security_err reason) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_ERR("Pairing failed conn: %s, reason %d\n", addr, reason);
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = bt_user_connected,
	.disconnected = bt_user_disconnected,
	.security_changed = bt_user_security_changed
};

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = bt_user_auth_cancel,
	.pairing_complete = bt_user_pairing_complete,
	.pairing_failed = bt_user_pairing_failed
};

void bt_post_action(int err) {
	if (err != 0) {
		LOG_ERR("Bluetooth initialize failed(%d)\n", err);
        return;
    }
	if (IS_ENABLED(CONFIG_SETTINGS)) 
		settings_load();
	if (IS_ENABLED(CONFIG_BT_ANCS_CLIENT)) {
	    err = apple_ancs_init(NULL, NULL);
		if (err)
			LOG_ERR("ANCS initialize failed(%d)\n", err);
	}
    if (IS_ENABLED(CONFIG_BT_AMS_CLIENT)) {
		err = apple_ams_init();
		if (err)
			LOG_ERR("AMS initialize failed(%d)\n", err);
	}
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) 
		LOG_ERR("Failed to register authorization callbacks\n");
}