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

#include "apple_ancs.h"
#include "bt_disc.h"

LOG_MODULE_REGISTER(app_ancs);

#define DEBUG_ANCS


#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN sizeof(DEVICE_NAME)

/* Allocated size for attribute data. */
#define ATTR_DATA_SIZE BT_ANCS_ATTR_DATA_MAX

enum {
	DISCOVERY_ANCS_ONGOING,
	DISCOVERY_ANCS_SUCCEEDED,
	SERVICE_CHANGED_INDICATED
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_SOLICIT128, BT_UUID_ANCS_VAL)
};


static struct bt_ancs_client ancs_c;

/* Local copy to keep track of the newest arriving notifications. */
static struct bt_ancs_evt_notif notification_latest;

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

static ancs_notification_handler_t ancs_notification_cb;
static ancs_data_handler_t ancs_data_cb;


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

static void notif_attr_print(const struct bt_ancs_attr *attr) {
	if (attr->attr_len != 0) {
		LOG_INF("%s: %s\n", lit_attrid[attr->attr_id],
		       log_strdup((char *)attr->attr_data));
	} else if (attr->attr_len == 0) {
		LOG_INF("%s: (N/A)\n", lit_attrid[attr->attr_id]);
	}
}

static void app_attr_print(const struct bt_ancs_attr *attr) {
	if (attr->attr_len != 0) {
		LOG_INF("%s: %s\n", lit_appid[attr->attr_id],
		       log_strdup((char *)attr->attr_data));
	} else if (attr->attr_len == 0) {
		LOG_INF("%s: (N/A)\n", lit_appid[attr->attr_id]);
	}
}

#else /* !DEBUG_ANCS */
#define notif_print(info)      (void)info
#define notif_attr_print(attr) (void)attr
#define app_attr_print(attr)   (void)attr
#endif /* DEBUG_ANCS */

static void ancs_error_code_print(uint8_t err) {
	switch (err) {
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
    	ancs_error_code_print(err);
}

static void bt_ancs_notification_source_handler(struct bt_ancs_client *ancs,
		int err, const struct bt_ancs_evt_notif *notif) {
	if (err != 0) {
		LOG_ERR("ANCS notification error(%d)\n", err);
		return;
	}
	notif_print(notif);
	notification_latest = *notif;
	if (ancs_notification_cb)
		ancs_notification_cb(ancs, notif);
	switch (notif->evt_id) {
	case BT_ANCS_EVENT_ID_NOTIFICATION_ADDED:
		if (!notif->evt_flags.pre_existing)
			bt_ancs_request_attrs(ancs, notif, bt_ancs_write_done);
		break;
	case BT_ANCS_EVENT_ID_NOTIFICATION_MODIFIED:
	case BT_ANCS_EVENT_ID_NOTIFICATION_REMOVED:
	default:
		break;
	}
}

static void bt_ancs_data_source_handler(struct bt_ancs_client *ancs,
	const struct bt_ancs_attr_response *rsp) {
	if (ancs_data_cb)
		ancs_data_cb(ancs, rsp);
	switch (rsp->command_id) {
	case BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES:
		notif_attr_print(&rsp->attr);
		break;
	case BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES:
		app_attr_print(&rsp->attr);
		break;
	case BT_ANCS_COMMAND_ID_PERFORM_NOTIF_ACTION:
	default:
		break;
	}
}

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
	struct bt_ancs_client *ancs = (struct bt_ancs_client *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);
	LOG_INF("ANCS discovery completed\n");
	bt_gatt_dm_data_print(dm);
	int err = bt_ancs_handles_assign(dm, ancs);
	if (err) {
		LOG_ERR("Could not init ANCS client object, error: %d\n", err);
	} else {
		enable_ancs_notifications(ancs);
	}
	err = bt_gatt_dm_data_release(dm);
	if (err) {
		LOG_ERR("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
	bt_discovery_start(conn);
}

static void discover_ancs_not_found_cb(struct bt_conn *conn, 
	void *ctx) {
	LOG_WRN("ANCS could not be found during the discovery\n");
	bt_discovery_finish(conn, false);
}

static void discover_ancs_error_found_cb(struct bt_conn *conn, 
	int err, void *ctx) {
	LOG_WRN("The discovery procedure for ANCS failed, err %d\n", err);
	bt_discovery_finish(conn, false);
}

static const struct bt_gatt_dm_cb discover_ancs_cb = {
	.completed = discover_ancs_completed_cb,
	.service_not_found = discover_ancs_not_found_cb,
	.error_found = discover_ancs_error_found_cb,
};

static void connected(struct bt_conn *conn, uint8_t err) {
	int ret;
	char addr[BT_ADDR_LE_STR_LEN];
	if (err) {
		LOG_ERR("Connection failed (err 0x%02x)\n", err);
		return;
	}
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	if (bt_conn_get_security(conn) < BT_SECURITY_L2) {
		ret = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (ret) 
			LOG_ERR("Failed to set security (err %d)\n", ret); 
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_WRN("Disconnected from %s (reason 0x%02x)\n", log_strdup(addr), reason);
}

static int ancs_discover(struct bt_conn *conn) {
	return bt_gatt_dm_start(conn, BT_UUID_ANCS,
		 &discover_ancs_cb, &ancs_c);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
	enum bt_security_err err) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	if (!err) {
		LOG_INF("Security changed: %s level %u\n", log_strdup(addr), level);
		if (bt_conn_get_security(conn) >= BT_SECURITY_L2)
			bt_gattp_discovery_start(conn);
	} else {
		LOG_ERR("Security failed: %s level %u err %d\n", 
			log_strdup(addr), level, err);  
	}
}

static void auth_cancel(struct bt_conn *conn) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing cancelled: %s\n", log_strdup(addr));
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void pairing_complete(struct bt_conn *conn, bool bonded) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Pairing completed: %s, bonded: %d\n", log_strdup(addr), bonded);
}

static void pairing_failed(struct bt_conn *conn, 
	enum bt_security_err reason) {
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_ERR("Pairing failed conn: %s, reason %d\n", log_strdup(addr), reason);
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static int bt_ancs_service_init(struct bt_ancs_client *ancs) {
	static struct bt_disc_node disc_nodes = {
		.name     = "ANCS",
		.discover = ancs_discover
	};
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
	if (err) 
		goto _out;
	err = bt_gattp_discovery_register(&disc_nodes);
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


int apple_ancs_init(ancs_notification_handler_t notif_cb,
	ancs_data_handler_t data_cb) {
	int err = bt_ancs_service_init(&ancs_c);
	if (err) {
		LOG_ERR("ANCS client init failed (err %d)\n", err);
		return err;
	}
	ancs_notification_cb = notif_cb;
	ancs_data_cb = data_cb;
	return 0;
}



void bt_apple_ancs_ready(int err) {
	if (err != 0)
		LOG_ERR("Bluetooth initialize failed(%d)\n", err);
	if (IS_ENABLED(CONFIG_SETTINGS)) 
		settings_load();
	apple_ancs_init(NULL, NULL);
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) 
		LOG_ERR("Failed to register authorization callbacks\n");
}