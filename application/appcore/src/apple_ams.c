#include <zephyr.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>

#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/ams_client.h>

#include <logging/log.h>

#include "bt_disc.h"

LOG_MODULE_REGISTER(bt_ams);

static struct bt_ams_client ams_instance;
static const enum bt_ams_track_attribute_id entity_update_track[] = {
	BT_AMS_TRACK_ATTRIBUTE_ID_ARTIST,
	BT_AMS_TRACK_ATTRIBUTE_ID_ALBUM,
	BT_AMS_TRACK_ATTRIBUTE_ID_TITLE,
	BT_AMS_TRACK_ATTRIBUTE_ID_DURATION
};

static char msg_buff[400 + 1];
static void ams_read_entity_cb(struct bt_ams_client *ams_c, uint8_t err, 
	const uint8_t *data, size_t len) {
	if (!err) {
		memcpy(msg_buff, data, len);
		msg_buff[len] = '\0';
		printk("AMS EA: %s\n", msg_buff);
	} else {
		printk("AMS EA read error 0x%02X\n", err);
	}
}

static void ams_remote_command_notify(struct bt_ams_client *ams_c,
	const uint8_t *data, size_t len) {
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

static void ams_write_entity_update_cb(struct bt_ams_client *ams_c, uint8_t err) {
	if (!err) {
		err = bt_ams_read_entity_attribute(ams_c, ams_read_entity_cb);
	} else {
		printk("AMS EA write error 0x%02X\n", err);
	}
}

static void ams_entity_updated_notify(struct bt_ams_client *ams_c,
	const struct bt_ams_entity_update_notif *notif, int err) {
	uint8_t attr_val;
	char str_hex[9];
	if (err) {
		LOG_WRN("AMS entity update invalid\n");
		return;
	}
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

	sprintf(str_hex, "%02X,%02X,%02X", notif->ent_attr.entity, 
		attr_val, notif->flags);
	memcpy(msg_buff, notif->data, notif->len);
	msg_buff[notif->len] = '\0';
	printk("AMS EU: %s %s\n", str_hex, msg_buff);
	if(attr_val == BT_AMS_ENTITY_ID_PLAYER)
	{
		//ios_msg_protocol_handle(IOS_AMS_MUSIC_PLAYER,&msg_buff,strlen(msg_buff));
	}
	else if(attr_val == BT_AMS_ENTITY_ID_TRACK)
	{
		//ios_msg_protocol_handle(IOS_AMS_MUSIC_NAME,&msg_buff,strlen(msg_buff));
	}
	/* Read truncated song title. */
	if (notif->ent_attr.entity == BT_AMS_ENTITY_ID_TRACK &&
		notif->ent_attr.attribute.track == BT_AMS_TRACK_ATTRIBUTE_ID_TITLE &&
		(notif->flags & (0x1U << BT_AMS_ENTITY_UPDATE_FLAG_TRUNCATED))) {
		err = bt_ams_write_entity_attribute(ams_c, &notif->ent_attr, 
			ams_write_entity_update_cb);
		if (err) 
			LOG_ERR("Cannot write to Entity Attribute (err %d)", err);
	}
}

static void ams_write_entity_cb(struct bt_ams_client *ams, 
	uint8_t err) {
	if (err) {
		LOG_ERR("AMS EU write error 0x%02X\n", err);
	}
}

static void ams_notifications_enable(struct bt_ams_client *ams) {
	struct bt_ams_entity_attribute_list list;
	int err = bt_ams_subscribe_remote_command(ams, ams_remote_command_notify);
	if (err) {
		LOG_ERR("Cannot subscribe to Remote Command notification (err %d)\n", err);
		return;
	}
	err = bt_ams_subscribe_entity_update(ams, ams_entity_updated_notify);
	if (err) {
		LOG_ERR("Cannot subscribe to Entity Update notification (err %d)\n", err);
		return;
	}
	list.entity = BT_AMS_ENTITY_ID_TRACK;
	list.attribute.track = entity_update_track;
	list.attribute_count = ARRAY_SIZE(entity_update_track);
	err = bt_ams_write_entity_update(ams, &list, ams_write_entity_cb);
	if (err) 
		LOG_ERR("Cannot write to Entity Update (err %d)\n", err);
}

static void ams_discover_completed(struct bt_gatt_dm *dm, void *ctx) {
	struct bt_ams_client *ams = (struct bt_ams_client *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);
	LOG_INF("The AMS discovery procedure succeeded\n");
	bt_gatt_dm_data_print(dm);
	int err = bt_ams_handles_assign(dm, ams);
	if (err) 
		LOG_ERR("Could not init AMS client object, error: %d\n", err);
	else
		ams_notifications_enable(ams);
	err = bt_gatt_dm_data_release(dm);
	if (err) {
		LOG_ERR("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
	bt_discovery_start(conn);
}

static void ams_discover_not_found(struct bt_conn *conn, void *ctx) {
	LOG_WRN("The AMS could not be found during the discovery\n");
	bt_discovery_finish(conn, false);
}

static void ams_discover_error_found(struct bt_conn *conn, int err, 
    void *ctx) {
	LOG_WRN("The AMS discovery procedure failed, err %d\n", err);
	bt_discovery_finish(conn, false);
}

static const struct bt_gatt_dm_cb discover_ams_cb = {
	.completed = ams_discover_completed,
	.service_not_found = ams_discover_not_found,
	.error_found = ams_discover_error_found,
};

static int ams_discover(struct bt_conn *conn) {
	return bt_gatt_dm_start(conn, BT_UUID_AMS, &discover_ams_cb, 
		&ams_instance);
}

static void ams_remote_command_write_cb(struct bt_ams_client *ams, 
	uint8_t err) {
	if (err) {
		LOG_ERR("AMS RC write error 0x%02x\n", (int)err);
	}
}

int ams_remote_command_send(enum bt_ams_remote_command_id cmd,
	bt_ams_write_cb cb) {
	return bt_ams_write_remote_command(&ams_instance, cmd, 
		cb? cb: ams_remote_command_write_cb);			  
}

int ams_init(void) {
	static struct bt_disc_node dn = {
		.name     = "AMS",
		.discover = ams_discover
	};
	int err = bt_gattp_discovery_register(&dn);
	if (!err)
		return bt_ams_client_init(&ams_instance);
	return err;
}