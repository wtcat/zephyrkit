/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_GATTS_CLIENT_H_
#define BT_GATTS_CLIENT_H_

/** @file
 *
 * @defgroup bt_gatts_client Bluetooth LE Generic Attribute Profile Service Client API
 * @{
 *
 * @brief API for the Bluetooth LE Generic Attribute Profile (GATT) Service Client.
 */

#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Structure for Service Changed handle range. */
struct bt_gatts_handle_range {
	/** Start Handle. */
	uint16_t start_handle;

	/** End Handle. */
	uint16_t end_handle;
};

struct bt_gatts_client;

/**@brief Service Changed Indication callback function.
 *
 * @param[in] gatts_c      GATT Service client instance.
 * @param[in] handle_range Affected handle range.
 * @param[in] err          0 if the indication is valid.
 *                         Otherwise, contains a (negative) error code.
 */
typedef void (*bt_gatts_indicate_cb)(
				struct bt_gatts_client *gatts_c,
				const struct bt_gatts_handle_range *handle_range,
				int err);

struct bt_gatts_client {
	/** Connection object. */
	struct bt_conn *conn;

	/** Handle of the Remote Command Characteristic. */
	uint16_t handle_sc;

	/** Handle of the CCCD of the Remote Command Characteristic. */
	uint16_t handle_sc_ccc;

	/** GATT subscribe parameters for the Service Changed Characteristic. */
	struct bt_gatt_subscribe_params indicate_params;

	/** Callback function for the Service Changed indication. */
	bt_gatts_indicate_cb indicate_cb;

	/** Internal state. */
	atomic_t state;
};

/**@brief Initialize the GATT Service client instance.
 *
 * @param[out] gatts_c     GATT Service client instance.
 *
 * @retval 0 If the client is initialized successfully.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gatts_client_init(struct bt_gatts_client *gatts_c);

/**@brief Assign handles to the GATT Service client instance.
 *
 * @details Call this function when a link has been established with a peer to
 *          associate the link to this instance of the module. This makes it
 *          possible to handle several links and associate each link to a particular
 *          instance of this module.
 *
 * @param[in]     dm       Discovery object.
 * @param[in,out] gatts_c  GATT Service client instance.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gatts_handles_assign(struct bt_gatt_dm *dm,
			    struct bt_gatts_client *gatts_c);

/**@brief Subscribe to the Service Changed indication.
 *
 * @param[in] gatts_c      GATT Service client instance.
 * @param[in] func         Indication callback function handler.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gatts_subscribe_service_changed(struct bt_gatts_client *gatts_c,
				       bt_gatts_indicate_cb func);

/**@brief Unsubscribe from the Service Changed indication.
 *
 * @param[in] gatts_c      GATT Service client instance.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gatts_unsubscribe_service_changed(struct bt_gatts_client *gatts_c);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_GATTS_CLIENT_H_ */
