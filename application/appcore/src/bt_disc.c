#include <errno.h>

#include <zephyr.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/ancs_client.h>
#include <bluetooth/services/gattp.h>

#include <logging/log.h>
#include "bt_disc.h"

LOG_MODULE_REGISTER(bt_disc);

#define DC_LIST_FOREACH(dc, head) \
    for ((dc) = (head); (dc) != NULL; (dc) = (dc)->next)

enum disc_state {
    DISC_SERVICE_CHANGED,
    DISC_SERVICE_BUSY
};

static struct bt_disc_node *dc_array;
static struct bt_disc_node *dc_pointer;
static atomic_t dc_flags;

void bt_discovery_reinit(void) {
    __ASSERT_NO_MSG(dc_array != NULL);
    struct bt_disc_node *dc;
    dc_pointer = dc_array;
    dc_flags = ATOMIC_INIT(0);
    DC_LIST_FOREACH(dc, dc_array)
        dc->found = false;
}

int bt_discovery_start(struct bt_conn *conn) {
    __ASSERT_NO_MSG(dc_array != NULL);
    if (atomic_test_and_set_bit(&dc_flags, DISC_SERVICE_BUSY))
        return 0;
    if (atomic_test_and_clear_bit(&dc_flags, DISC_SERVICE_CHANGED)) 
        dc_pointer = dc_array;
    struct bt_disc_node *dc;
    DC_LIST_FOREACH(dc, dc_pointer) {
        if (!dc->found) {
            int ret = dc->discover(conn);
            if (ret) {
                LOG_ERR("Failed to start %s discovery (err %d)\n", 
                    dc->name, ret);
            }
            dc_pointer = dc;
            return ret;
        }
    }
    return 0;
}

int bt_discovery_finish(struct bt_conn *conn, bool found) {
    if (atomic_test_and_clear_bit(&dc_flags, DISC_SERVICE_BUSY)) {
        __ASSERT_NO_MSG(dc_pointer != NULL);
        dc_pointer->found = found;
        dc_pointer = dc_pointer->next;
        return bt_discovery_start(conn);
    }
    return -EINVAL;
}

static void bt_gattp_service_changed(struct bt_gattp *gattp,
	const struct bt_gattp_handle_range *handle_range, int err) {
	if (!err) {
        atomic_set_bit(&dc_flags, DISC_SERVICE_CHANGED);
        bt_discovery_start(gattp->conn);
	}
}

static void bt_gattp_discovery_completed(struct bt_gatt_dm *dm, void *ctx) {
	struct bt_gattp *gattp = (struct bt_gattp *)ctx;
	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);
	LOG_DBG("The discovery procedure for GATT Service succeeded\n");
	bt_gatt_dm_data_print(dm);
	int err = bt_gattp_handles_assign(dm, gattp);
	if (err) {
		LOG_ERR("Could not init GATT Service client object, error: %d\n", err);
	} else {
		err = bt_gattp_subscribe_service_changed(gattp, bt_gattp_service_changed);
        if (err)
            LOG_ERR("Cannot subscribe to Service Changed indication (err %d)\n", err);
	}
	err = bt_gatt_dm_data_release(dm);
	if (err) {
		LOG_ERR("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
	bt_discovery_start(conn);
}

void bt_gattp_discovery_not_found(struct bt_conn *conn, void *ctx) {
	LOG_WRN("GATT Service could not be found during the discovery\n");
    bt_discovery_finish(conn, false);
}

void bt_gattp_discovery_error_found(struct bt_conn *conn, 
	int err, void *ctx) {
	LOG_WRN("The discovery procedure for GATT Service failed, err %d\n",
        err);
    bt_discovery_finish(conn, false);
}

int bt_gattp_discovery_start(struct bt_conn *conn) {
    static struct bt_gattp gattp;
    static const struct bt_gatt_dm_cb gattp_disc = {
        .completed = bt_gattp_discovery_completed,
        .service_not_found = bt_gattp_discovery_not_found,
        .error_found = bt_gattp_discovery_error_found,
    };
    bt_discovery_reinit();
    bt_gattp_init(&gattp);
    int err = bt_gatt_dm_start(conn, BT_UUID_GATT, &gattp_disc, &gattp);
	if (err) 
		LOG_ERR("Failed to start discovery for GATT Service (err %d)\n", err);
    return err;
}

int bt_gattp_discovery_register(struct bt_disc_node *node) {
    if (node == NULL)
        return -EINVAL;
    struct bt_disc_node **ptr = &dc_array;
    while (*ptr != NULL)
        ptr = &(*ptr)->next;
    node->next = NULL;
    *ptr = node;
    return 0;
}
