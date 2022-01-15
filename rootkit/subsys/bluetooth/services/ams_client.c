/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/ams_client.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ams_c, CONFIG_BT_AMS_CLIENT_LOG_LEVEL);

enum {
	AMS_RC_WRITE_PENDING,
	AMS_RC_NOTIF_ENABLED,
	AMS_EU_WRITE_PENDING,
	AMS_EU_NOTIF_ENABLED,
	AMS_EA_WRITE_PENDING,
	AMS_EA_READ_PENDING
};

int bt_ams_client_init(struct bt_ams_client *ams_c)
{
	if (!ams_c) {
		return -EINVAL;
	}

	memset(ams_c, 0, sizeof(struct bt_ams_client));

	return 0;
}

static void ams_reinit(struct bt_ams_client *ams_c)
{
	ams_c->conn = NULL;
	ams_c->handle_rc = 0;
	ams_c->handle_rc_ccc = 0;
	ams_c->handle_eu = 0;
	ams_c->handle_eu_ccc = 0;
	ams_c->handle_ea = 0;
	ams_c->state = ATOMIC_INIT(0);
}

int bt_ams_handles_assign(struct bt_gatt_dm *dm,
			  struct bt_ams_client *ams_c)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
		bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
		bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_AMS)) {
		return -ENOTSUP;
	}

	LOG_DBG("AMS found");

	ams_reinit(ams_c);

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_AMS_REMOTE_COMMAND);
	if (!gatt_chrc) {
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_AMS_REMOTE_COMMAND);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ams_c->handle_rc = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ams_c->handle_rc_ccc = gatt_desc->handle;

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_AMS_ENTITY_UPDATE);
	if (!gatt_chrc) {
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_AMS_ENTITY_UPDATE);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ams_c->handle_eu = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ams_c->handle_eu_ccc = gatt_desc->handle;

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_AMS_ENTITY_ATTRIBUTE);
	if (!gatt_chrc) {
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_AMS_ENTITY_ATTRIBUTE);
	if (!gatt_desc) {
		return -EINVAL;
	}
	ams_c->handle_ea = gatt_desc->handle;

	/* Finally - save connection object */
	ams_c->conn = bt_gatt_dm_conn_get(dm);

	return 0;
}

static uint8_t bt_ams_rc_notify_callback(struct bt_conn *conn,
					 struct bt_gatt_subscribe_params *params,
					 const void *data, uint16_t length)
{
	struct bt_ams_client *ams_c;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, notif_params_rc);

	if (ams_c->notify_rc_cb) {
		ams_c->notify_rc_cb(ams_c, data, length);
	}

	return BT_GATT_ITER_CONTINUE;
}

int bt_ams_subscribe_remote_command(struct bt_ams_client *ams_c,
				    bt_ams_notify_cb func)
{
	int err;

	if (!ams_c || !func) {
		return -EINVAL;
	}
	if (!ams_c->conn) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ams_c->state, AMS_RC_NOTIF_ENABLED)) {
		return -EALREADY;
	}

	ams_c->notify_rc_cb = func;

	ams_c->notif_params_rc.notify = bt_ams_rc_notify_callback;
	ams_c->notif_params_rc.value = BT_GATT_CCC_NOTIFY;
	ams_c->notif_params_rc.value_handle = ams_c->handle_rc;
	ams_c->notif_params_rc.ccc_handle = ams_c->handle_rc_ccc;
	atomic_set_bit(ams_c->notif_params_rc.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(ams_c->conn, &ams_c->notif_params_rc);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_RC_NOTIF_ENABLED);
	}

	return err;
}

int bt_ams_unsubscribe_remote_command(struct bt_ams_client *ams_c)
{
	int err;

	if (!ams_c) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&ams_c->state, AMS_RC_NOTIF_ENABLED)) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(ams_c->conn, &ams_c->notif_params_rc);

	atomic_clear_bit(&ams_c->state, AMS_RC_NOTIF_ENABLED);

	return err;
}

static uint8_t bt_ams_eu_notify_callback(struct bt_conn *conn,
					 struct bt_gatt_subscribe_params *params,
					 const void *data, uint16_t length)
{
	struct bt_ams_client *ams_c;
	const uint8_t *value = (const uint8_t *)data;
	struct bt_ams_entity_update_notif notif;
	int err;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, notif_params_eu);

	err = (length >= 3) ? 0: -EINVAL;
	if (!err) {
		notif.ent_attr.entity = (enum bt_ams_entity_id)*(value + 0);
		switch(notif.ent_attr.entity) {
		case BT_AMS_ENTITY_ID_PLAYER:
			notif.ent_attr.attribute.player =
				(enum bt_ams_player_attribute_id)*(value + 1);
			break;
		case BT_AMS_ENTITY_ID_QUEUE:
			notif.ent_attr.attribute.queue =
				(enum bt_ams_queue_attribute_id)*(value + 1);
			break;
		case BT_AMS_ENTITY_ID_TRACK:
			notif.ent_attr.attribute.track =
				(enum bt_ams_track_attribute_id)*(value + 1);
			break;
		default:
			err = -EINVAL;
			break;
		}
		notif.flags = *(value + 2);
		notif.data = value + 3;
		notif.len = length - 3;
	}

	if (ams_c->notify_eu_cb) {
		ams_c->notify_eu_cb(ams_c, &notif, err);
	}

	return BT_GATT_ITER_CONTINUE;
}

int bt_ams_subscribe_entity_update(struct bt_ams_client *ams_c,
				   bt_ams_entity_update_notify_cb func)
{
	int err;

	if (!ams_c || !func) {
		return -EINVAL;
	}
	if (!ams_c->conn) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ams_c->state, AMS_EU_NOTIF_ENABLED)) {
		return -EALREADY;
	}

	ams_c->notify_eu_cb = func;

	ams_c->notif_params_eu.notify = bt_ams_eu_notify_callback;
	ams_c->notif_params_eu.value = BT_GATT_CCC_NOTIFY;
	ams_c->notif_params_eu.value_handle = ams_c->handle_eu;
	ams_c->notif_params_eu.ccc_handle = ams_c->handle_eu_ccc;
	atomic_set_bit(ams_c->notif_params_eu.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(ams_c->conn, &ams_c->notif_params_eu);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_EU_NOTIF_ENABLED);
	}

	return err;
}

int bt_ams_unsubscribe_entity_update(struct bt_ams_client *ams_c)
{
	int err;

	if (!ams_c) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&ams_c->state, AMS_EU_NOTIF_ENABLED)) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(ams_c->conn, &ams_c->notif_params_eu);

	atomic_clear_bit(&ams_c->state, AMS_EU_NOTIF_ENABLED);

	return err;
}

static void bt_ams_rc_write_callback(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_write_params *params)
{
	struct bt_ams_client *ams_c;
	bt_ams_write_cb write_callback;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, write_params_rc);

	write_callback = ams_c->write_rc_cb;
	atomic_clear_bit(&ams_c->state, AMS_RC_WRITE_PENDING);
	if (write_callback) {
		write_callback(ams_c, err);
	}
}

int bt_ams_write_remote_command(struct bt_ams_client *ams_c,
				enum bt_ams_remote_command_id command,
				bt_ams_write_cb func)
{
	int err;

	if (!ams_c) {
		return -EINVAL;
	}
	if (!ams_c->conn) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ams_c->state, AMS_RC_WRITE_PENDING)) {
		return -EBUSY;
	}

	ams_c->data_rc[0] = command;

	ams_c->write_params_rc.func = bt_ams_rc_write_callback;
	ams_c->write_params_rc.handle = ams_c->handle_rc;
	ams_c->write_params_rc.offset = 0;
	ams_c->write_params_rc.data = ams_c->data_rc;
	ams_c->write_params_rc.length = 1;

	ams_c->write_rc_cb = func;

	err = bt_gatt_write(ams_c->conn, &ams_c->write_params_rc);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_RC_WRITE_PENDING);
	}

	return err;
}

static void bt_ams_eu_write_callback(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_write_params *params)
{
	struct bt_ams_client *ams_c;
	bt_ams_write_cb write_callback;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, write_params_eu);

	write_callback = ams_c->write_eu_cb;
	atomic_clear_bit(&ams_c->state, AMS_EU_WRITE_PENDING);
	if (write_callback) {
		write_callback(ams_c, err);
	}
}

int bt_ams_write_entity_update(struct bt_ams_client *ams_c,
			       const struct bt_ams_entity_attribute_list *ent_attr_list,
			       bt_ams_write_cb func)
{
	int err;
	size_t i;
	uint8_t attr_val;

	if (!ams_c) {
		return -EINVAL;
	}
	if (!ams_c->conn) {
		return -EINVAL;
	}
	if (ent_attr_list->attribute_count > BT_AMS_ATTRIBUTE_COUNT_MAX) {
		return -EINVAL;
	};

	if (atomic_test_and_set_bit(&ams_c->state, AMS_EU_WRITE_PENDING)) {
		return -EBUSY;
	}

	ams_c->data_eu[0] = ent_attr_list->entity;
	err = 0;
	for (i = 0; i < ent_attr_list->attribute_count; i++) {
		switch(ent_attr_list->entity) {
		case BT_AMS_ENTITY_ID_PLAYER:
			attr_val = *(ent_attr_list->attribute.player + i);
			break;
		case BT_AMS_ENTITY_ID_QUEUE:
			attr_val = *(ent_attr_list->attribute.queue + i);
			break;
		case BT_AMS_ENTITY_ID_TRACK:
			attr_val = *(ent_attr_list->attribute.track + i);
			break;
		default:
			err = -EINVAL;
			break;
		}

		if (err < 0) {
			break;
		} else {
			ams_c->data_eu[1 + i] = attr_val;
		}
	}
	if (err < 0) {
		atomic_clear_bit(&ams_c->state, AMS_EU_WRITE_PENDING);
		return err;
	}

	ams_c->write_params_eu.func = bt_ams_eu_write_callback;
	ams_c->write_params_eu.handle = ams_c->handle_eu;
	ams_c->write_params_eu.offset = 0;
	ams_c->write_params_eu.data = ams_c->data_eu;
	ams_c->write_params_eu.length = ent_attr_list->attribute_count + 1;

	ams_c->write_eu_cb = func;

	err = bt_gatt_write(ams_c->conn, &ams_c->write_params_eu);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_EU_WRITE_PENDING);
	}

	return err;
}

static void bt_ams_ea_write_callback(struct bt_conn *conn, uint8_t err,
				      struct bt_gatt_write_params *params)
{
	struct bt_ams_client *ams_c;
	bt_ams_write_cb write_callback;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, write_params_ea);

	write_callback = ams_c->write_ea_cb;
	atomic_clear_bit(&ams_c->state, AMS_EA_WRITE_PENDING);
	if (write_callback) {
		write_callback(ams_c, err);
	}
}

int bt_ams_write_entity_attribute(struct bt_ams_client *ams_c,
				  const struct bt_ams_entity_attribute *ent_attr,
				  bt_ams_write_cb func)
{
	int err;
	uint8_t attr_val;

	if (!ams_c) {
		return -EINVAL;
	}
	if (!ams_c->conn) {
		return -EINVAL;
	}

	switch(ent_attr->entity) {
	case BT_AMS_ENTITY_ID_PLAYER:
		attr_val = ent_attr->attribute.player;
		break;
	case BT_AMS_ENTITY_ID_QUEUE:
		attr_val = ent_attr->attribute.queue;
		break;
	case BT_AMS_ENTITY_ID_TRACK:
		attr_val = ent_attr->attribute.track;
		break;
	default:
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ams_c->state, AMS_EA_WRITE_PENDING)) {
		return -EBUSY;
	}

	ams_c->data_ea[0] = ent_attr->entity;
	ams_c->data_ea[1] = attr_val;

	ams_c->write_params_ea.func = bt_ams_ea_write_callback;
	ams_c->write_params_ea.handle = ams_c->handle_ea;
	ams_c->write_params_ea.offset = 0;
	ams_c->write_params_ea.data = ams_c->data_ea;
	ams_c->write_params_ea.length = 2;

	ams_c->write_ea_cb = func;

	err = bt_gatt_write(ams_c->conn, &ams_c->write_params_ea);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_EA_WRITE_PENDING);
	}

	return err;
}

static uint8_t bt_ams_ea_read_callback(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_read_params *params,
				       const void *data, uint16_t length)
{
	struct bt_ams_client *ams_c;
	bt_ams_read_cb read_callback;

	/* Retrieve AMS client module context. */
	ams_c = CONTAINER_OF(params, struct bt_ams_client, read_params_ea);

	read_callback = ams_c->read_ea_cb;
	atomic_clear_bit(&ams_c->state, AMS_EA_READ_PENDING);
	if (read_callback) {
		read_callback(ams_c, err, data, length);
	}

	return BT_GATT_ITER_STOP;
}

int bt_ams_read_entity_attribute(struct bt_ams_client *ams_c, bt_ams_read_cb func)
{
	int err;

	if (!ams_c || !func) {
		return -EINVAL;
	}
	if (!ams_c->conn) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&ams_c->state, AMS_EA_READ_PENDING)) {
		return -EBUSY;
	}

	ams_c->read_params_ea.func = bt_ams_ea_read_callback;
	ams_c->read_params_ea.handle_count = 1;
	ams_c->read_params_ea.single.handle = ams_c->handle_ea;
	ams_c->read_params_ea.single.offset = 0;

	ams_c->read_ea_cb = func;

	err = bt_gatt_read(ams_c->conn, &ams_c->read_params_ea);
	if (err) {
		atomic_clear_bit(&ams_c->state, AMS_EA_READ_PENDING);
	}

	return err;
}
