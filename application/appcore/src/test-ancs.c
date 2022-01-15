/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ydy_boards_config.h"
//#if (DEBUG_PART_ENABLE ==1)
//#define DEBUG_INFO_ENABLE   1
//#define DEBUG_ERROR_ENABLE  1
//#endif

#undef DEBUG_STRING
#define DEBUG_STRING "[BLE_ANCS]"
#include "debug.h"
#include "ble_core.h"
#include "ancs_resolve.h"
#include "ios_msg_process.h"
#include "ble_ancs_and_ams.h"
#include <zephyr.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/ancs_client.h>
#include <bluetooth/services/ams_client.h>
#include <bluetooth/services/gatts_client.h>
#include <nrfx_clock.h>
#include <settings/settings.h>


#define printk	DEBUG_INFO
/* Allocated size for attribute data. */
#define ATTR_DATA_SIZE BT_ANCS_ATTR_DATA_MAX

enum {
	SERVICE_DISCOVERY_ONGOING,
	SERVICE_CHANGED_INDICATED
};
typedef int (*dm_start_routine)(struct bt_conn *conn);

struct dm_start_entry {
	dm_start_routine start;
	const char *str;
	bool is_found;
};

static int dm_start_gatts(struct bt_conn *conn);
static int dm_start_ancs(struct bt_conn *conn);
static int dm_start_ams(struct bt_conn *conn);
static void dm_reset_state(void);
static void dm_start_next(struct bt_conn *conn);
static void dm_finish_this(struct bt_conn *conn, bool is_found);
static void ancs_get_msg_info(void);
static struct bt_ancs_client ancs_c;

static struct bt_ams_client ams_c;

static struct bt_gatts_client gatts_c;

static bool ancs_discovery_busy = false;
static int ancs_call_status = 0;

static struct dm_start_entry dm_start_table[] = {
	//{dm_start_gatts, "GATT Service"},
	{dm_start_ancs, "ANCS"},
	{dm_start_ams, "AMS"},
	{NULL}
};
static uint32_t dm_start_counter;
static atomic_t dm_start_flags;

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

/* String literals for the iOS notification categories.
 * Used then printing to UART.
 */
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

/* String literals for the iOS notification event types.
 * Used then printing to UART.
 */
static const char *lit_eventid[BT_ANCS_EVT_ID_COUNT] = { "Added",
							 "Modified",
							 "Removed" };

/* String literals for the iOS notification attribute types.
 * Used when printing to UART.
 */
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

/* String literals for the iOS notification attribute types.
 * Used When printing to UART.
 */
static const char *lit_appid[BT_ANCS_APP_ATTR_COUNT] = { "Display Name" };

static void bt_ancs_notification_source_handler(struct bt_ancs_client *ancs_c,
		int err, const struct bt_ancs_evt_notif *notif);
static void bt_ancs_data_source_handler(struct bt_ancs_client *ancs_c,
		const struct bt_ancs_attr_response *response);

static const enum bt_ams_track_attribute_id entity_update_track[] = {
	BT_AMS_TRACK_ATTRIBUTE_ID_ARTIST,
	BT_AMS_TRACK_ATTRIBUTE_ID_ALBUM,
	BT_AMS_TRACK_ATTRIBUTE_ID_TITLE,
	BT_AMS_TRACK_ATTRIBUTE_ID_DURATION
};
static int bt_conn_security_req(void)
{
	int sec_err =  0;
	if (bt_conn_get_security(ble_get_connect_handle()) < BT_SECURITY_L2)
	{
		bt_conn_set_security(ble_get_connect_handle(), BT_SECURITY_L2);
		if (sec_err) {
			printk("Failed to set security (err %d)\n",
					sec_err);
		}
		return 0;
	}
	return 1;
}
static char msg_buff[400 + 1];
static void ea_read_cb(struct bt_ams_client *ams_c, uint8_t err, const uint8_t *data, size_t len)
{
	if (!err) {
		memcpy(msg_buff, data, len);
		msg_buff[len] = '\0';
		printk("AMS EA: %s\n", msg_buff);
	} else {
		printk("AMS EA read error 0x%02X\n", err);
	}
}
static void ea_write_cb(struct bt_ams_client *ams_c, uint8_t err)
{
	if (!err) {
		err = bt_ams_read_entity_attribute(ams_c, ea_read_cb);
	} else {
		printk("AMS EA write error 0x%02X\n", err);
	}
}
static void notify_rc_cb(struct bt_ams_client *ams_c,
			 const uint8_t *data, size_t len)
{
	size_t i;
	char str_hex[4];

	if (len > 0) {
		/* Print the accepted Remote Command values. */
		sprintf(msg_buff, "%02X", *data);

		for (i = 1; i < len; i++) {
			sprintf(str_hex, ",%02X", *(data + i));
			strcat(msg_buff, str_hex);
		}

		printk("AMS RC: %s\n", msg_buff);
	}
}
static void notify_eu_cb(struct bt_ams_client *ams_c,
			 const struct bt_ams_entity_update_notif *notif,
			 int err)
{
	uint8_t attr_val;
	char str_hex[9];

	if (!err) {
		switch(notif->ent_attr.entity) {
		case BT_AMS_ENTITY_ID_PLAYER:
			attr_val = notif->ent_attr.attribute.player;
			break;
		case BT_AMS_ENTITY_ID_QUEUE:
			attr_val = notif->ent_attr.attribute.queue;
			break;
		case BT_AMS_ENTITY_ID_TRACK:
			attr_val = notif->ent_attr.attribute.track;
			break;
		default:
			err = -EINVAL;
		}
	}

	if (!err) {
		sprintf(str_hex, "%02X,%02X,%02X",
			notif->ent_attr.entity, attr_val, notif->flags);
		memcpy(msg_buff, notif->data, notif->len);
		msg_buff[notif->len] = '\0';
		printk("AMS EU: %s %s\n", str_hex, msg_buff);
		if(attr_val == BT_AMS_ENTITY_ID_PLAYER)
		{
			ios_msg_protocol_handle(IOS_AMS_MUSIC_PLAYER,&msg_buff,strlen(msg_buff));
		}
		else if(attr_val == BT_AMS_ENTITY_ID_TRACK)
		{
			ios_msg_protocol_handle(IOS_AMS_MUSIC_NAME,&msg_buff,strlen(msg_buff));
		}
	
		/* Read truncated song title. */
		if (notif->ent_attr.entity == BT_AMS_ENTITY_ID_TRACK &&
		    notif->ent_attr.attribute.track == BT_AMS_TRACK_ATTRIBUTE_ID_TITLE &&
		    (notif->flags & (0x1U << BT_AMS_ENTITY_UPDATE_FLAG_TRUNCATED))) {
			err = bt_ams_write_entity_attribute(ams_c, &notif->ent_attr, ea_write_cb);
			if (err) {
				printk("Cannot write to Entity Attribute (err %d)", err);
			}
		}
	} else {
		printk("AMS EU invalid\n");
	}
}
static void rc_write_cb(struct bt_ams_client *ams_c, uint8_t err)
{
	if (err) {
		printk("AMS RC write error 0x%02X\n", err);
	}
}

static void eu_write_cb(struct bt_ams_client *ams_c, uint8_t err)
{
	if (err) {
		printk("AMS EU write error 0x%02X\n", err);
	}
}
static void enable_ams_notifications(struct bt_ams_client *ams_c)
{
	int err;
	struct bt_ams_entity_attribute_list entity_attribute_list;

	err = bt_ams_subscribe_remote_command(ams_c, notify_rc_cb);
	if (err) {
		printk("Cannot subscribe to Remote Command notification (err %d)\n",
		       err);
	}

	err = bt_ams_subscribe_entity_update(ams_c, notify_eu_cb);
	if (err) {
		printk("Cannot subscribe to Entity Update notification (err %d)\n",
		       err);
	}

	entity_attribute_list.entity = BT_AMS_ENTITY_ID_TRACK;
	entity_attribute_list.attribute.track = entity_update_track;
	entity_attribute_list.attribute_count = ARRAY_SIZE(entity_update_track);
	err = bt_ams_write_entity_update(ams_c, &entity_attribute_list, eu_write_cb);
	if (err) {
		printk("Cannot write to Entity Update (err %d)\n",
		       err);
	}
}
static void discover_ams_completed_cb(struct bt_gatt_dm *dm, void *ctx)
{
	int err;
	struct bt_ams_client *ams_c = (struct bt_ams_client *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);
	bool is_found;

	printk("The AMS discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_ams_handles_assign(dm, ams_c);
	if (err) {
		printk("Could not init AMS client object, error: %d\n", err);
	} else {
		enable_ams_notifications(ams_c);
	}

	is_found = !err;

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n",
		       err);
	}

	dm_finish_this(conn, is_found);
}
static void discover_ams_not_found_cb(struct bt_conn *conn, void *ctx)
{
	printk("The AMS could not be found during the discovery\n");

	dm_finish_this(conn, false);
}

static void discover_ams_error_found_cb(struct bt_conn *conn, int err, void *ctx)
{
	printk("The AMS discovery procedure failed, err %d\n", err);

	dm_finish_this(conn, false);
}
static const struct bt_gatt_dm_cb discover_ams_cb = {
	.completed = discover_ams_completed_cb,
	.service_not_found = discover_ams_not_found_cb,
	.error_found = discover_ams_error_found_cb,
};
static void enable_ancs_notifications(struct bt_ancs_client *ancs_c)
{
	int err;

	err = bt_ancs_subscribe_notification_source(ancs_c,
			bt_ancs_notification_source_handler);
	if (err) {
		printk("Failed to enable Notification Source notification (err %d)\n",
		       err);
	}

	err = bt_ancs_subscribe_data_source(ancs_c,
			bt_ancs_data_source_handler);
	if (err) {
		printk("Failed to enable Data Source notification (err %d)\n",
		       err);
	}
}

static void discover_ancs_completed_cb(struct bt_gatt_dm *dm, void *ctx)
{
	int err;
	struct bt_ancs_client *ancs_c = (struct bt_ancs_client *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);
	bool is_found;

	printk("The ANCS discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_ancs_handles_assign(dm, ancs_c);
	if (err) {
		printk("Could not init ANCS client object, error: %d\n", err);
	} else {
		if (bt_conn_get_security(ancs_c->conn) < BT_SECURITY_L2) {
			err = bt_conn_set_security(ancs_c->conn, BT_SECURITY_L2);
			if (err) {
				printk("Failed to set security (err %d)\n",
				       err);
			}
		}else
		{
			enable_ancs_notifications(ancs_c);
		}
	}

	is_found = !err;

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n",
		       err);
	}
	ios_msg_protocol_handle(IOS_ANCS_CONNECT,NULL,0);
	if (bt_conn_get_security(ancs_c->conn) >= BT_SECURITY_L2)
	{
		dm_finish_this(ancs_c->conn, is_found);
	}
	ancs_discovery_busy = false;
}
static void discover_ancs_not_found_cb(struct bt_conn *conn, void *ctx)
{
	printk("The ANCS could not be found during the discovery\n");
	ios_msg_protocol_handle(IOS_ANCS_DISCONNECT,NULL,0);
	ancs_discovery_busy = false;
	dm_finish_this(conn, false);
}
static void discover_ancs_error_found_cb(struct bt_conn *conn, int err, void *ctx)
{
	printk("The ANCS discovery procedure failed, err %d\n", err);
	ios_msg_protocol_handle(IOS_ANCS_DISCONNECT,NULL,0);
	ancs_discovery_busy = false;
	dm_finish_this(conn, false);
}

static const struct bt_gatt_dm_cb discover_ancs_cb = {
	.completed = discover_ancs_completed_cb,
	.service_not_found = discover_ancs_not_found_cb,
	.error_found = discover_ancs_error_found_cb,
};
static void indicate_sc_cb(struct bt_gatts_client *gatts_c,
			   const struct bt_gatts_handle_range *handle_range,
			   int err)
{
	if (!err) {
		atomic_set_bit(&dm_start_flags, SERVICE_CHANGED_INDICATED);
		dm_start_next(gatts_c->conn);
	}
}
static void enable_gatts_indications(struct bt_gatts_client *gatts_c)
{
	int err;

	err = bt_gatts_subscribe_service_changed(gatts_c, indicate_sc_cb);
	if (err) {
		printk("Cannot subscribe to Service Changed indication (err %d)\n",
		       err);
	}
}
static void discover_gatts_completed_cb(struct bt_gatt_dm *dm, void *ctx)
{
	int err;
	struct bt_gatts_client *gatts_c = (struct bt_gatts_client *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);
	bool is_found;

	printk("GATT Service discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_gatts_handles_assign(dm, gatts_c);
	if (err) {
		printk("Could not init GATT Service client object, error: %d\n", err);
	} else {
		enable_gatts_indications(gatts_c);
	}

	is_found = !err;

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n",
		       err);
	}

	dm_finish_this(conn, is_found);
}

static void discover_gatts_not_found_cb(struct bt_conn *conn, void *ctx)
{
	printk("GATT Service could not be found during the discovery\n");

	dm_finish_this(conn, false);
}

static void discover_gatts_error_found_cb(struct bt_conn *conn, int err, void *ctx)
{
	printk("The GATT Service discovery procedure failed, err %d\n", err);

	dm_finish_this(conn, false);
}

static const struct bt_gatt_dm_cb discover_gatts_cb = {
	.completed = discover_gatts_completed_cb,
	.service_not_found = discover_gatts_not_found_cb,
	.error_found = discover_gatts_error_found_cb,
};

static int dm_start_gatts(struct bt_conn *conn)
{
	return bt_gatt_dm_start(conn, BT_UUID_GATT, &discover_gatts_cb, &gatts_c);
}

static int dm_start_ancs(struct bt_conn *conn)
{
	return bt_gatt_dm_start(conn, BT_UUID_ANCS, &discover_ancs_cb, &ancs_c);
}

static int dm_start_ams(struct bt_conn *conn)
{
	return bt_gatt_dm_start(conn, BT_UUID_AMS, &discover_ams_cb, &ams_c);
}

static void dm_reset_state(void)
{
	uint32_t i;

	dm_start_counter = 0;
	dm_start_flags = ATOMIC_INIT(0);

	for (i = 0; dm_start_table[i].start; i++) {
		dm_start_table[i].is_found = false;
	}
}
static void dm_start_next(struct bt_conn *conn)
{
	int err;

	if (atomic_test_and_set_bit(&dm_start_flags, SERVICE_DISCOVERY_ONGOING)) {
		return;
	}

	if (atomic_test_and_clear_bit(&dm_start_flags, SERVICE_CHANGED_INDICATED)) {
		/* When there is a service change, try to discover services again. */
		dm_start_counter = 0;
	}

	while (dm_start_table[dm_start_counter].start) {
		if (!dm_start_table[dm_start_counter].is_found) {
			err = dm_start_table[dm_start_counter].start(conn);
			if (err) {
				printk("Failed to start %s discovery (err %d)\n",
					dm_start_table[dm_start_counter].str, err);
			} else {
				break;
			}
		}

		dm_start_counter++;
	}
}
static void dm_finish_this(struct bt_conn *conn, bool is_found)
{
	dm_start_table[dm_start_counter].is_found = is_found;

	dm_start_counter++;

	atomic_clear_bit(&dm_start_flags, SERVICE_DISCOVERY_ONGOING);

	dm_start_next(conn);
}
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);

	//bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void pairing_confirm(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	bt_conn_auth_pairing_confirm(conn);

	printk("Pairing confirmed: %s\n", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason: %d\n", addr, reason);

	//bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.pairing_confirm = pairing_confirm,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

/**@brief Function for printing an iOS notification.
 *
 * @param[in] notif  Pointer to the iOS notification.
 */
static void notif_print(const struct bt_ancs_evt_notif *notif)
{
	printk("\nNotification\n");
	printk("Event:       %s\n", lit_eventid[notif->evt_id]);
	printk("Category ID: %s\n", lit_catid[notif->category_id]);
	printk("Category Cnt:%u\n", (unsigned int)notif->category_count);
	printk("UID:         %u\n", (unsigned int)notif->notif_uid);

	printk("Flags:\n");
	if (notif->evt_flags.silent) {
		printk(" Silent\n");
	}
	if (notif->evt_flags.important) {
		printk(" Important\n");
	}
	if (notif->evt_flags.pre_existing) {
		printk(" Pre-existing\n");
	}
	if (notif->evt_flags.positive_action) {
		printk(" Positive Action\n");
	}
	if (notif->evt_flags.negative_action) {
		printk(" Negative Action\n");
	}
	if(notif->evt_id == BT_ANCS_EVENT_ID_NOTIFICATION_ADDED)
	{
		if(!notif->evt_flags.pre_existing)
		{
			if(notif->category_id == 1)
			{
				ancs_call_status = 1; //来电
			}else if(notif->category_id == 2)
			{
				ancs_call_status = 2; //未接
			}else
			{
				ancs_call_status = 0;
			}
			ancs_get_msg_info();
		}
	}else if(notif->evt_id == BT_ANCS_EVENT_ID_NOTIFICATION_REMOVED)
	{
		if(notif->category_id == 1)
		{
			ancs_call_status = 0;
		}
	}
	
}

/**@brief Function for printing iOS notification attribute data.
 *
 * @param[in] attr Pointer to an iOS notification attribute.
 */
static void notif_attr_print(const struct bt_ancs_attr *attr)
{
	if (attr->attr_len != 0) {
		printk("%s: %s\n", lit_attrid[attr->attr_id],(char *)attr->attr_data);
		ancs_resolve_evt_ios_attr(attr->attr_id,attr->attr_data,attr->attr_len);
	} else if (attr->attr_len == 0) {
		printk("%s: (N/A)\n", lit_attrid[attr->attr_id]);
	}
}

/**@brief Function for printing iOS notification attribute data.
 *
 * @param[in] attr Pointer to an iOS App attribute.
 */
static void app_attr_print(const struct bt_ancs_attr *attr)
{
	if (attr->attr_len != 0) {
		printk("%s: %s\n", lit_appid[attr->attr_id],
		       (char *)attr->attr_data);
	} else if (attr->attr_len == 0) {
		printk("%s: (N/A)\n", lit_appid[attr->attr_id]);
	}
}

/**@brief Function for printing out errors that originated from the Notification Provider (iOS).
 *
 * @param[in] err_code_np Error code received from NP.
 */
static void err_code_print(uint8_t err_code_np)
{
	switch (err_code_np) {
	case BT_ATT_ERR_ANCS_NP_UNKNOWN_COMMAND:
		printk("Error: Command ID was not recognized by the Notification Provider.\n");
		break;

	case BT_ATT_ERR_ANCS_NP_INVALID_COMMAND:
		printk("Error: Command failed to be parsed on the Notification Provider.\n");
		break;

	case BT_ATT_ERR_ANCS_NP_INVALID_PARAMETER:
		printk("Error: Parameter does not refer to an existing object on the Notification Provider.\n");
		break;

	case BT_ATT_ERR_ANCS_NP_ACTION_FAILED:
		printk("Error: Perform Notification Action Failed on the Notification Provider.\n");
		break;

	default:
		break;
	}
}

static void bt_ancs_notification_source_handler(struct bt_ancs_client *ancs_c,
		int err, const struct bt_ancs_evt_notif *notif)
{
	if (!err) {
		notification_latest = *notif;
		notif_print(&notification_latest);
	}
}

static void bt_ancs_data_source_handler(struct bt_ancs_client *ancs_c,
		const struct bt_ancs_attr_response *response)
{
	//printk("bt_ancs_data_source_handler:%d",response->command_id);
	switch (response->command_id) {
	case BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES:
		notif_attr_latest = response->attr;
		notif_attr_print(&notif_attr_latest);
		if (response->attr.attr_id ==
		    BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER) {
			notif_attr_app_id_latest = response->attr;
		}
		break;

	case BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES:
		app_attr_print(&response->attr);
		break;

	default:
		/* No implementation needed. */
		break;
	}
}

static void bt_ancs_write_response_handler(struct bt_ancs_client *ancs_c,
					   uint8_t err)
{
	err_code_print(err);
}

static int ancs_c_init(void)
{
	int err;

	err = bt_ancs_client_init(&ancs_c);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER,
		attr_appid, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_app_attr(&ancs_c,
		BT_ANCS_APP_ATTR_ID_DISPLAY_NAME,
		attr_disp_name, sizeof(attr_disp_name));
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_TITLE,
		attr_title, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_MESSAGE,
		attr_message, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_SUBTITLE,
		attr_subtitle, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_MESSAGE_SIZE,
		attr_message_size, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_DATE,
		attr_date, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_POSITIVE_ACTION_LABEL,
		attr_posaction, ATTR_DATA_SIZE);
	if (err) {
		return err;
	}

	err = bt_ancs_register_attr(&ancs_c,
		BT_ANCS_NOTIF_ATTR_ID_NEGATIVE_ACTION_LABEL,
		attr_negaction, ATTR_DATA_SIZE);

	return err;
}

static int ams_c_init(void)
{
	return bt_ams_client_init(&ams_c);
}

static int gatts_c_init(void)
{
	return bt_gatts_client_init(&gatts_c);
}
static void ancs_get_msg_info(void)
{
	int err;
	err = bt_ancs_request_attrs(&ancs_c, &notification_latest,
						    bt_ancs_write_response_handler);
	if (err) {
		printk("Failed requesting attributes for a notification (err: %d)\n",
			   err);
	}
}
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;
	int err;
#if 0
	if (buttons & KEY_REQ_NOTI_ATTR) {
		err = bt_ancs_request_attrs(&ancs_c, &notification_latest,
					    bt_ancs_write_response_handler);
		if (err) {
			printk("Failed requesting attributes for a notification (err: %d)\n",
			       err);
		}
	}

	if (buttons & KEY_REQ_APP_ATTR) {
		if (notif_attr_app_id_latest.attr_id ==
			    BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER &&
		    notif_attr_app_id_latest.attr_len != 0) {
			printk("Request for %s:\n",
			       notif_attr_app_id_latest.attr_data);
			err = bt_ancs_request_app_attr(
				&ancs_c, notif_attr_app_id_latest.attr_data,
				notif_attr_app_id_latest.attr_len,
				bt_ancs_write_response_handler);
			if (err) {
				printk("Failed requesting attributes for a given app (err: %d)\n",
				       err);
			}
		}
	}

	if (buttons & KEY_POS_ACTION) {
		if (notification_latest.evt_flags.positive_action) {
			printk("Performing Positive Action.\n");
			err = bt_ancs_notification_action(
				&ancs_c, notification_latest.notif_uid,
				BT_ANCS_ACTION_ID_POSITIVE,
				bt_ancs_write_response_handler);
			if (err) {
				printk("Failed performing action (err: %d)\n",
				       err);
			}
		}
	}

	if (buttons & KEY_NEG_ACTION) {
		if (notification_latest.evt_flags.negative_action) {
			printk("Performing Negative Action.\n");
			err = bt_ancs_notification_action(
				&ancs_c, notification_latest.notif_uid,
				BT_ANCS_ACTION_ID_NEGATIVE,
				bt_ancs_write_response_handler);
			if (err) {
				printk("Failed performing action (err: %d)\n",
				       err);
			}
		}
	}
#endif
}

static int cmd_remote_command_play(void)
{
	int err;
	err = bt_ams_write_remote_command(&ams_c,
					  BT_AMS_REMOTE_COMMAND_ID_PLAY,
					  rc_write_cb);

	printk("AMS RC Play --> %d", err);

	return 0;
}

static int cmd_remote_command_pause(void)
{
	int err;
	err = bt_ams_write_remote_command(&ams_c,
					  BT_AMS_REMOTE_COMMAND_ID_PAUSE,
					  rc_write_cb);

	printk("AMS RC Pause --> %d", err);

	return 0;
}

static int cmd_remote_command_next(void)
{
	int err;
	err = bt_ams_write_remote_command(&ams_c,
					  BT_AMS_REMOTE_COMMAND_ID_NEXT_TRACK,
					  rc_write_cb);

	printk("AMS RC Next Track --> %d", err);

	return 0;
}

static int cmd_remote_command_previous(void)
{
	int err;

	err = bt_ams_write_remote_command(&ams_c,
					  BT_AMS_REMOTE_COMMAND_ID_PREVIOUS_TRACK,
					  rc_write_cb);

	printk("AMS RC Previous Track --> %d", err);

	return 0;
}

static int cmd_remote_command_volume_up(void)
{
	int err;

	err = bt_ams_write_remote_command(&ams_c,
					  BT_AMS_REMOTE_COMMAND_ID_VOLUME_UP,
					  rc_write_cb);

	printk("AMS RC Previous Track --> %d", err);

	return 0;
}

static int cmd_remote_command_volume_down(void)
{
	int err;

	err = bt_ams_write_remote_command(&ams_c,
					  BT_AMS_REMOTE_COMMAND_ID_VOLUME_DOWN,
					  rc_write_cb);

	printk("AMS RC Previous Track --> %d", err);

	return 0;
}


/**@brief ams cmd
*/
void ble_ams_cmd(uint8_t cmd)
{
	switch(cmd)
	{
	case AMS_MUSIC_PLAY:
		cmd_remote_command_play();
		break;
	case AMS_MUSIC_PAUSE:
		cmd_remote_command_pause();
		break;
	case AMS_MUSIC_NEXT:
		cmd_remote_command_next();
		break;
	case AMS_MUSIC_PRE:
		cmd_remote_command_previous();
		break;
	case AMS_MUSIC_VOL_UP:
		cmd_remote_command_volume_up();
		break;
	case AMS_MUSIC_VOL_DOWN:
		cmd_remote_command_volume_down();
		break;
	case ANCS_ANSWER_CALL:
		bt_ancs_notification_action(
				&ancs_c, notification_latest.notif_uid,
				BT_ANCS_ACTION_ID_POSITIVE,
				bt_ancs_write_response_handler);
		break;
	case ANCS_REJECT_CALL:
		bt_ancs_notification_action(
				&ancs_c, notification_latest.notif_uid,
				BT_ANCS_ACTION_ID_NEGATIVE,
				bt_ancs_write_response_handler);
		break;
	}
}

void ble_auth_init(void)
{
	int err;
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks\n");
		return;
	}
}
int ble_ancs_call_status(void)
{
	return ancs_call_status;
}

void ble_ancs_enable_notifications(struct bt_conn *conn)
{
	printk("ble_ancs_enable_notifications sec:%d busy:%d\n",bt_conn_get_security(conn),ancs_discovery_busy);
	if (bt_conn_get_security(conn) >= BT_SECURITY_L2) {
		if(ancs_discovery_busy == false)
		{
			dm_reset_state();
			dm_start_next(conn);
		}
	}
	//enable_notifications();
}
void ble_ancs_start_security(struct bt_conn *conn)
{
	printk("ble_ancs_start_security\n");
	ancs_discovery_busy = true;
	dm_reset_state();
	dm_start_next(conn);
}

void ble_ancs_init(void)
{
	int err;

	printk("Starting Apple Notification Center Service client example\n");

	err = ancs_c_init();
	if (err) {
		printk("ANCS client init failed (err %d)\n", err);
		return;
	}
	err = ams_c_init();
	if (err) {
		printk("AMS client init failed (err %d)\n", err);
		return;
	}
	err = gatts_c_init();
	if (err) {
		printk("GATT Service client init failed (err %d)\n", err);
		return;
	}
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks\n");
		return;
	}
}
