/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sys/byteorder.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/gatts_client.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(gatts_c, CONFIG_BT_GATTS_CLIENT_LOG_LEVEL);

enum {
	GATTS_SC_INDICATE_ENABLED
};

int bt_gatts_client_init(struct bt_gatts_client *gatts_c)
{
	if (!gatts_c) {
		return -EINVAL;
	}

	memset(gatts_c, 0, sizeof(struct bt_gatts_client));

	return 0;
}

static void gatts_reinit(struct bt_gatts_client *gatts_c)
{
	gatts_c->conn = NULL;
	gatts_c->handle_sc = 0;
	gatts_c->handle_sc_ccc = 0;
	gatts_c->state = ATOMIC_INIT(0);
}

int bt_gatts_handles_assign(struct bt_gatt_dm *dm,
			    struct bt_gatts_client *gatts_c)
{
	const struct bt_gatt_dm_attr *gatt_service_attr =
		bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
		bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_GATT)) {
		return -ENOTSUP;
	}

	LOG_DBG("GATT Service found");

	gatts_reinit(gatts_c);

	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_GATT_SC);
	if (!gatt_chrc) {
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
					    BT_UUID_GATT_SC);
	if (!gatt_desc) {
		return -EINVAL;
	}
	gatts_c->handle_sc = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		return -EINVAL;
	}
	gatts_c->handle_sc_ccc = gatt_desc->handle;

	/* Finally - save connection object */
	gatts_c->conn = bt_gatt_dm_conn_get(dm);

	return 0;
}

static uint8_t bt_gatts_sc_indicate_callback(struct bt_conn *conn,
					     struct bt_gatt_subscribe_params *params,
					     const void *data, uint16_t length)
{
	struct bt_gatts_client *gatts_c;
	struct bt_gatts_handle_range handle_range;
	int err;
	const uint8_t *handles_data = (const uint8_t *)data;

	/* Retrieve GATT Service client module context. */
	gatts_c = CONTAINER_OF(params, struct bt_gatts_client, indicate_params);

	if (gatts_c->indicate_cb) {
		err = (length == 4) ? 0: -EINVAL;

		if (!err) {
			handle_range.start_handle = sys_get_le16(handles_data);
			handle_range.end_handle = sys_get_le16(handles_data + 2);
		}

		gatts_c->indicate_cb(gatts_c, &handle_range, err);
	}

	return BT_GATT_ITER_CONTINUE;
}

int bt_gatts_subscribe_service_changed(struct bt_gatts_client *gatts_c,
				       bt_gatts_indicate_cb func)
{
	int err;

	if (!gatts_c || !func) {
		return -EINVAL;
	}
	if (!gatts_c->conn) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&gatts_c->state, GATTS_SC_INDICATE_ENABLED)) {
		return -EALREADY;
	}

	gatts_c->indicate_cb = func;

	gatts_c->indicate_params.notify = bt_gatts_sc_indicate_callback;
	gatts_c->indicate_params.value = BT_GATT_CCC_INDICATE;
	gatts_c->indicate_params.value_handle = gatts_c->handle_sc;
	gatts_c->indicate_params.ccc_handle = gatts_c->handle_sc_ccc;
	atomic_set_bit(gatts_c->indicate_params.flags,
		       BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(gatts_c->conn, &gatts_c->indicate_params);
	if (err) {
		atomic_clear_bit(&gatts_c->state, GATTS_SC_INDICATE_ENABLED);
	}

	return err;
}

int bt_gatts_unsubscribe_service_changed(struct bt_gatts_client *gatts_c)
{
	int err;

	if (!gatts_c) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&gatts_c->state, GATTS_SC_INDICATE_ENABLED)) {
		return -EFAULT;
	}

	err = bt_gatt_unsubscribe(gatts_c->conn, &gatts_c->indicate_params);

	atomic_clear_bit(&gatts_c->state, GATTS_SC_INDICATE_ENABLED);

	return err;
}
