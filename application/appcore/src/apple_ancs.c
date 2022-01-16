/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/ancs_client.h>
#include <bluetooth/services/gattp.h>

#include <logging/log.h>
#include <settings/settings.h>

LOG_MODULE_REGISTER(ancs_client);

#define DEBUG_ANCS


#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* Allocated size for attribute data. */
#define ATTR_DATA_SIZE BT_ANCS_ATTR_DATA_MAX

enum {
	DISCOVERY_ANCS_ONGOING,
	DISCOVERY_ANCS_SUCCEEDED,
	SERVICE_CHANGED_INDICATED
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_LIMITED | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_SOLICIT128, BT_UUID_ANCS_VAL),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct bt_ancs_client ancs_c;
static struct bt_gattp gattp;
static atomic_t discovery_flags;

/* Local copy to keep track of the newest arriving notifications. */
static struct bt_ancs_evt_notif notification_latest;
/* Local copy of the newest notification attribute. */
static struct bt_ancs_attr notif_attr_latest;
/* Local copy of the newest app attribute. */
static struct bt_ancs_attr notif_attr_app_id_latest;
/* Buffers to store attribute data. */
static uint8_t attr_appid[ATTR_DATA_SIZE];
static uint8_t attr_title[ATTR_DATA_SIZE];
static uint8_t attr_subtitle[ATTR_DATA_SIZE];
static uint8_t attr_message[ATTR_DATA_SIZE];
static uint8_t attr_message_size[ATTR_DATA_SIZE];
static uint8_t attr_date[ATTR_DATA_SIZE];
static uint8_t attr_posaction[ATTR_DATA_SIZE];
static uint8_t attr_negaction[ATTR_DATA_SIZE];
static uint8_t attr_disp_name[ATTR_DATA_SIZE];

#if defined(DEBUG_ANCS)
static const char *lit_catid[BT_ANCS_CATEGORY_ID_COUNT] = {
	"Other",
	"Incoming Call",
	"Missed Call",
	"Voice Mail",
	"Social",
	"Schedule",
	"Email",
	"News",
	"Health And Fitness",
	"Business And Finance",
	"Location",
	"Entertainment"
};

const char *lit_eventid[BT_ANCS_EVT_ID_COUNT] = { 
	"Added", "Modified", "Removed" 
};
							 
static const char *lit_attrid[BT_ANCS_NOTIF_ATTR_COUNT] = {
	"App Identifier",
	"Title",
	"Subtitle",
	"Message",
	"Message Size",
	"Date",
	"Positive Action Label",
	"Negative Action Label"
};
#endif

static const char *lit_appid[BT_ANCS_APP_ATTR_COUNT] = { "Display Name" };
static void discover_ancs_first(struct bt_conn *conn);
static void discover_ancs_again(struct bt_conn *conn);
static void bt_ancs_notification_source_handler(struct bt_ancs_client *ancs,
		int err, const struct bt_ancs_evt_notif *notif);
static void bt_ancs_data_source_handler(struct bt_ancs_client *ancs,
		const struct bt_ancs_attr_response *response);

#if defined(DEBUG_ANCS)
static void notif_print(const struct bt_ancs_evt_notif *notif) {
	LOG_INF("\nNotification\n");
	LOG_INF("Event:       %s\n", lit_eventid[notif->evt_id]);
	LOG_INF("Category ID: %s\n", lit_catid[notif->category_id]);
	LOG_INF("Category Cnt:%u\n", (unsigned int)notif->category_count);
	LOG_INF("UID:         %u\n", (unsigned int)notif->notif_uid);
	LOG_INF("Flags:\n");
	if (notif->evt_flags.silent) 
		LOG_INF(" Silent\n");
	if (notif->evt_flags.important) 
		LOG_INF(" Important\n");
	if (notif->evt_flags.pre_existing) 
		LOG_INF(" Pre-existing\n");
	if (notif->evt_flags.positive_action) 
		LOG_INF(" Positive Action\n");
	if (notif->evt_flags.negative_action) 
		LOG_INF(" Negative Action\n");
}
#else
#define notif_print(info) (void)info
#endif

static void enable_ancs_notifications(struct bt_ancs_client *ancs) {
	int err = bt_ancs_subscribe_notification_source(ancs,
			bt_ancs_notification_source_handler);
	if (err) {
		LOG_ERR("Failed to enable Notification "
			"Source notification (err %d)\n", err);
	}
	err = bt_ancs_subscribe_data_source(ancs,
			bt_ancs_data_source_handler);
	if (err) {
		LOG_ERR("Failed to enable Data Source notification (err %d)\n", err); 
	}
}

static void discover_ancs_completed_cb(struct bt_gatt_dm *dm, 
	void *ctx) {
	int err;
	struct bt_ancs_client *ancs = (struct bt_ancs_client *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);

	bt_gatt_dm_data_print(dm);
	err = bt_ancs_handles_assign(dm, ancs);
	if (err) {
		LOG_ERR("Could not init ANCS client object, error: %d\n", err);
	} else {
		atomic_set_bit(&discovery_flags, DISCOVERY_ANCS_SUCCEEDED);
		enable_ancs_notifications(ancs);
	}
	err = bt_gatt_dm_data_release(dm);
	if (err) {
		LOG_ERR("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
	atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	discover_ancs_again(conn);
}

static void discover_ancs_not_found_cb(struct bt_conn *conn, 
	void *ctx) {
	LOG_WRN("ANCS could not be found during the discovery\n");
	atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	discover_ancs_again(conn);
}

static void discover_ancs_error_found_cb(struct bt_conn *conn, 
	int err, void *ctx) {
	LOG_WRN("The discovery procedure for ANCS failed, err %d\n", err);
	atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	discover_ancs_again(conn);
}

static const struct bt_gatt_dm_cb discover_ancs_cb = {
	.completed = discover_ancs_completed_cb,
	.service_not_found = discover_ancs_not_found_cb,
	.error_found = discover_ancs_error_found_cb,
};

static void indicate_sc_cb(struct bt_gattp *gattp,
	const struct bt_gattp_handle_range *handle_range,
	int err) {
	if (!err) {
		atomic_set_bit(&discovery_flags, SERVICE_CHANGED_INDICATED);
		discover_ancs_again(gattp->conn);
	}
}

static void enable_gattp_indications(struct bt_gattp *gattp) {
	int err = bt_gattp_subscribe_service_changed(gattp, indicate_sc_cb);
	if (err) {
		LOG_ERR("Cannot subscribe to "
			"Service Changed indication (err %d)\n", err);
	}
}

static void discover_gattp_completed_cb(struct bt_gatt_dm *dm, void *ctx) {
	struct bt_gattp *gattp = (struct bt_gattp *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);
	LOG_DBG("The discovery procedure for GATT Service succeeded\n");
	bt_gatt_dm_data_print(dm);
	int err = bt_gattp_handles_assign(dm, gattp);
	if (err) {
		LOG_ERR("Could not init GATT Service client object, error: %d\n", err);
	} else {
		enable_gattp_indications(gattp);
	}
	err = bt_gatt_dm_data_release(dm);
	if (err) {
		LOG_ERR("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
	discover_ancs_first(conn);
}

static void discover_gattp_not_found_cb(struct bt_conn *conn, void *ctx) {
	LOG_WRN("GATT Service could not be found during the discovery\n");
	discover_ancs_first(conn);
}

static void discover_gattp_error_found_cb(struct bt_conn *conn, 
	int err, void *ctx) {
	LOG_WRN("The discovery procedure for GATT Service failed, err %d\n", err);
	discover_ancs_first(conn);
}

static const struct bt_gatt_dm_cb discover_gattp_cb = {
	.completed = discover_gattp_completed_cb,
	.service_not_found = discover_gattp_not_found_cb,
	.error_found = discover_gattp_error_found_cb,
};

static void discover_gattp(struct bt_conn *conn) {
	int err = bt_gatt_dm_start(conn, BT_UUID_GATT, 
		&discover_gattp_cb, &gattp);
	if (err) 
		LOG_ERR("Failed to start discovery for GATT Service (err %d)\n", err);
}

static void discover_ancs(struct bt_conn *conn, bool retry) {
	/* 1 Service Discovery at a time */
	if (atomic_test_and_set_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING)) 
		return;
	/* If ANCS is found, do not discover again. */
	if (atomic_test_bit(&discovery_flags, DISCOVERY_ANCS_SUCCEEDED)) {
		atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
		return;
	}
	/* Check that Service Changed indication is received before discovering ANCS again. */
	if (retry) {
		if (!atomic_test_and_clear_bit(&discovery_flags, 
			SERVICE_CHANGED_INDICATED)) {
			atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
			return;
		}
	}
	int err = bt_gatt_dm_start(conn, BT_UUID_ANCS,
		 &discover_ancs_cb, &ancs_c);
	if (err) {
		LOG_ERR("Failed to start discovery for ANCS (err %d)\n", err);
		atomic_clear_bit(&discovery_flags, DISCOVERY_ANCS_ONGOING);
	}
}

static inline void discover_ancs_first(struct bt_conn *conn) {
	discover_ancs(conn, false);
}

static inline void discover_ancs_again(struct bt_conn *conn) {
	discover_ancs(conn, true);
}

static void connected(struct bt_conn *conn, uint8_t err) {
	int sec_err;
	char addr[BT_ADDR_LE_STR_LEN];
	if (err) {
		LOG_ERR("Connection failed (err 0x%02x)\n", err);
		return;
	}
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	sec_err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (sec_err) {
		LOG_ERR("Failed to set security (err %d)\n",
		       sec_err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Disconnected from %s (reason 0x%02x)\n", addr, reason);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
	enum bt_security_err err) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	if (!err) {
		LOG_INF("Security changed: %s level %u\n", addr, level);
		if (bt_conn_get_security(conn) >= BT_SECURITY_L2) {
			discovery_flags = ATOMIC_INIT(0);
			discover_gattp(conn);
		}
	} else {
		LOG_ERR("Security failed: %s level %u err %d\n", 
			addr, level, err);  
	}
}

static void auth_cancel(struct bt_conn *conn) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing cancelled: %s\n", addr);
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void pairing_complete(struct bt_conn *conn, bool bonded) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, 
	enum bt_security_err reason) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_ERR("Pairing failed conn: %s, reason %d\n", addr, reason);
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void notif_attr_print(const struct bt_ancs_attr *attr)
{
	if (attr->attr_len != 0) {
		LOG_INF("%s: %s\n", lit_attrid[attr->attr_id],
		       (char *)attr->attr_data);
	} else if (attr->attr_len == 0) {
		LOG_INF("%s: (N/A)\n", lit_attrid[attr->attr_id]);
	}
}

static void app_attr_print(const struct bt_ancs_attr *attr) {
	if (attr->attr_len != 0) {
		LOG_INF("%s: %s\n", lit_appid[attr->attr_id],
		       (char *)attr->attr_data);
	} else if (attr->attr_len == 0) {
		LOG_INF("%s: (N/A)\n", lit_appid[attr->attr_id]);
	}
}

static void err_code_print(uint8_t err_code_np) {
	switch (err_code_np) {
	case BT_ATT_ERR_ANCS_NP_UNKNOWN_COMMAND:
		LOG_ERR("Error: Command ID was not recognized by "
			"the Notification Provider.\n");
		break;
	case BT_ATT_ERR_ANCS_NP_INVALID_COMMAND:
		LOG_ERR("Error: Command failed to be parsed on the "
			"Notification Provider.\n");
		break;
	case BT_ATT_ERR_ANCS_NP_INVALID_PARAMETER:
		LOG_ERR("Error: Parameter does not refer to an existing "
			"object on the Notification Provider.\n");
		break;
	case BT_ATT_ERR_ANCS_NP_ACTION_FAILED:
		LOG_ERR("Error: Perform Notification Action Failed on "
			"the Notification Provider.\n");
		break;
	default:
		break;
	}
}

static void bt_ancs_write_done(struct bt_ancs_client *ancs,
				 uint8_t err) {
	if (err != 0)
    	LOG_ERR("ANCS request attributes failed(%d)\n", err);
}

static void bt_ancs_notification_source_handler(struct bt_ancs_client *ancs,
		int err, const struct bt_ancs_evt_notif *notif) {
	if (err != 0)
		LOG_ERR("ANCS notification error(%d)\n", err);
	notif_print(notif);
	notification_latest = *notif;
	switch (notif->evt_id) {
	case BT_ANCS_EVENT_ID_NOTIFICATION_ADDED:
		if (!notif->evt_flags.pre_existing)
			bt_ancs_request_attrs(ancs, notif, bt_ancs_write_done);
		break;
	case BT_ANCS_EVENT_ID_NOTIFICATION_MODIFIED:
		break;
	case BT_ANCS_EVENT_ID_NOTIFICATION_REMOVED:
		break;
	default:
		break;
	}
}

static void bt_ancs_data_source_handler(struct bt_ancs_client *ancs,
	const struct bt_ancs_attr_response *rsp) {
	switch (rsp->command_id) {
	case BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES:
		notif_attr_latest = rsp->attr;
		notif_attr_print(&notif_attr_latest);
		if (rsp->attr.attr_id ==
		    BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER) {
			notif_attr_app_id_latest = rsp->attr;
		}
		break;
	case BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES:
		app_attr_print(&rsp->attr);
		break;
	case BT_ANCS_COMMAND_ID_PERFORM_NOTIF_ACTION:
	default:
		break;
	}
}

static int bt_ancs_service_init(struct bt_ancs_client *ancs) {
	int err = bt_ancs_client_init(ancs);
	if (err) 
		goto _out;
	err = bt_ancs_register_attr(ancs,
		BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER,
		attr_appid, ATTR_DATA_SIZE);
	if (err) 
		goto _out;
	err = bt_ancs_register_app_attr(ancs,
		BT_ANCS_APP_ATTR_ID_DISPLAY_NAME,
		attr_disp_name, sizeof(attr_disp_name));
	if (err) 
		goto _out;
	err = bt_ancs_register_attr(ancs,
		BT_ANCS_NOTIF_ATTR_ID_TITLE,
		attr_title, ATTR_DATA_SIZE);
	if (err) 
		goto _out;
	err = bt_ancs_register_attr(ancs,
		BT_ANCS_NOTIF_ATTR_ID_MESSAGE,
		attr_message, ATTR_DATA_SIZE);
	if (err) 
		goto _out;
	err = bt_ancs_register_attr(ancs,
		BT_ANCS_NOTIF_ATTR_ID_SUBTITLE,
		attr_subtitle, ATTR_DATA_SIZE);
	if (err) 
		goto _out;
	err = bt_ancs_register_attr(ancs,
		BT_ANCS_NOTIF_ATTR_ID_MESSAGE_SIZE,
		attr_message_size, ATTR_DATA_SIZE);
	if (err) 
		goto _out;
	err = bt_ancs_register_attr(ancs,
		BT_ANCS_NOTIF_ATTR_ID_DATE,
		attr_date, ATTR_DATA_SIZE);
	if (err) 
		goto _out;
	err = bt_ancs_register_attr(ancs,
		BT_ANCS_NOTIF_ATTR_ID_POSITIVE_ACTION_LABEL,
		attr_posaction, ATTR_DATA_SIZE);
	if (err) 
		goto _out;
	err = bt_ancs_register_attr(ancs,
		BT_ANCS_NOTIF_ATTR_ID_NEGATIVE_ACTION_LABEL,
		attr_negaction, ATTR_DATA_SIZE);
_out:
	return err;
}


BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};


static void apple_ancs_init(void) {
	int err = bt_ancs_service_init(&ancs_c);
	if (err) {
		LOG_ERR("ANCS client init failed (err %d)\n", err);
		return;
	}
	err = bt_gattp_init(&gattp);
	if (err) {
		LOG_ERR("GATT Service client init failed (err %d)\n", err);
		return;
	}
}

void bt_apple_ancs_ready(int err) {
	if (err != 0)
		LOG_ERR("Bluetooth initialize failed(%d)\n", err);
	apple_ancs_init();
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) 
		LOG_ERR("Failed to register authorization callbacks\n");
}