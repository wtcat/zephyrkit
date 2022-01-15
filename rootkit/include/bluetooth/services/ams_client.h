/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_AMS_CLIENT_H_
#define BT_AMS_CLIENT_H_

/** @file
 *
 * @defgroup bt_ams_client Apple Media Service Client
 * @{
 *
 * @brief Apple Media Service Client module.
 */

#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Apple Media Service UUID. */
#define BT_UUID_AMS_VAL \
	BT_UUID_128_ENCODE(0x89d3502b, 0x0f36, 0x433a, 0x8ef4, 0xc502ad55f8dc)

/** @brief Remote Command Characteristic UUID. */
#define BT_UUID_AMS_REMOTE_COMMAND_VAL                                     \
	BT_UUID_128_ENCODE(0x9b3c81d8, 0x57b1, 0x4a8a, 0xb8df, 0x0e56f7ca51c2)

/** @brief Entity Update Characteristic UUID. */
#define BT_UUID_AMS_ENTITY_UPDATE_VAL                                           \
	BT_UUID_128_ENCODE(0x2f7cabce, 0x808d, 0x411f, 0x9a0c, 0xbb92ba96c102)

/** @brief Entity Attribute Characteristic UUID. */
#define BT_UUID_AMS_ENTITY_ATTRIBUTE_VAL                                             \
	BT_UUID_128_ENCODE(0xc6b2f38c, 0x23ab, 0x46d8, 0xa6ab, 0xa3a870bbd5d7)

#define BT_UUID_AMS BT_UUID_DECLARE_128(BT_UUID_AMS_VAL)
#define BT_UUID_AMS_REMOTE_COMMAND                                       \
	BT_UUID_DECLARE_128(BT_UUID_AMS_REMOTE_COMMAND_VAL)
#define BT_UUID_AMS_ENTITY_UPDATE                                             \
	BT_UUID_DECLARE_128(BT_UUID_AMS_ENTITY_UPDATE_VAL)
#define BT_UUID_AMS_ENTITY_ATTRIBUTE BT_UUID_DECLARE_128(BT_UUID_AMS_ENTITY_ATTRIBUTE_VAL)

/** @brief Remote Command ID values. */
enum bt_ams_remote_command_id {
	BT_AMS_REMOTE_COMMAND_ID_PLAY,
	BT_AMS_REMOTE_COMMAND_ID_PAUSE,
	BT_AMS_REMOTE_COMMAND_ID_TOGGLE_PLAY_PAUSE,
	BT_AMS_REMOTE_COMMAND_ID_NEXT_TRACK,
	BT_AMS_REMOTE_COMMAND_ID_PREVIOUS_TRACK,
	BT_AMS_REMOTE_COMMAND_ID_VOLUME_UP,
	BT_AMS_REMOTE_COMMAND_ID_VOLUME_DOWN,
	BT_AMS_REMOTE_COMMAND_ID_ADVANCE_REPEAT_MODE,
	BT_AMS_REMOTE_COMMAND_ID_ADVANCE_SHUFFLE_MODE,
	BT_AMS_REMOTE_COMMAND_ID_SKIP_FORWARD,
	BT_AMS_REMOTE_COMMAND_ID_SKIP_BACKWARD,
	BT_AMS_REMOTE_COMMAND_ID_LIKE_TRACK,
	BT_AMS_REMOTE_COMMAND_ID_DISLIKE_TRACK,
	BT_AMS_REMOTE_COMMAND_ID_BOOKMARK_TRACK
};

/** @brief Entity ID values. */
enum bt_ams_entity_id {
	BT_AMS_ENTITY_ID_PLAYER,
	BT_AMS_ENTITY_ID_QUEUE,
	BT_AMS_ENTITY_ID_TRACK
};

/** @brief Entity Update flag: truncated. */
#define BT_AMS_ENTITY_UPDATE_FLAG_TRUNCATED 0

/** @brief Player Attribute ID values. */
enum bt_ams_player_attribute_id {
	BT_AMS_PLAYER_ATTRIBUTE_ID_NAME,
	BT_AMS_PLAYER_ATTRIBUTE_ID_PLAYBACK_INFO,
	BT_AMS_PLAYER_ATTRIBUTE_ID_VOLUME
};

/** @brief Queue Attribute ID values. */
enum bt_ams_queue_attribute_id {
	BT_AMS_QUEUE_ATTRIBUTE_ID_INDEX,
	BT_AMS_QUEUE_ATTRIBUTE_ID_COUNT,
	BT_AMS_QUEUE_ATTRIBUTE_ID_SHUFFLE_MODE,
	BT_AMS_QUEUE_ATTRIBUTE_ID_REPEAT_MODE
};

/** @brief Shuffle Mode constants. */
enum bt_ams_shuffle_mode {
	BT_AMS_SHUFFLE_MODE_OFF,
	BT_AMS_SHUFFLE_MODE_ONE,
	BT_AMS_SHUFFLE_MODE_ALL
};

/** @brief Repeat Mode constants. */
enum bt_ams_repeat_mode {
	BT_AMS_REPEAT_MODE_OFF,
	BT_AMS_REPEAT_MODE_ONE,
	BT_AMS_REPEAT_MODE_ALL
};

/** @brief Track Attribute ID values. */
enum bt_ams_track_attribute_id {
	BT_AMS_TRACK_ATTRIBUTE_ID_ARTIST,
	BT_AMS_TRACK_ATTRIBUTE_ID_ALBUM,
	BT_AMS_TRACK_ATTRIBUTE_ID_TITLE,
	BT_AMS_TRACK_ATTRIBUTE_ID_DURATION
};

/** @brief Structure for Entity Attribute pair. */
struct bt_ams_entity_attribute {
	/** Entity ID. */
	enum bt_ams_entity_id entity;

	/** Attribute ID. */
	union {
		enum bt_ams_player_attribute_id player;
		enum bt_ams_queue_attribute_id queue;
		enum bt_ams_track_attribute_id track;
	} attribute;
};

/** @brief Structure for Entity Attribute pair list. */
struct bt_ams_entity_attribute_list {
	/** Entity ID. */
	enum bt_ams_entity_id entity;

	/** Attribute ID pointer. */
	union {
		const enum bt_ams_player_attribute_id *player;
		const enum bt_ams_queue_attribute_id *queue;
		const enum bt_ams_track_attribute_id *track;
	} attribute;

	/** Attribute ID count. */
	size_t attribute_count;
};

/** @brief Structure for Entity Update notification. */
struct bt_ams_entity_update_notif {
	/** Entity Attribute pair. */
	struct bt_ams_entity_attribute ent_attr;

	/** Flags bitmask. */
	uint8_t flags;

	/** Value data pointer. */
	const uint8_t *data;

	/** Value data size. */
	size_t len;
};

/** Maximum value of Attribute ID count. */
#define BT_AMS_ATTRIBUTE_COUNT_MAX 4

/** AMS error code: MS is not properly set up. */
#define BT_ATT_ERR_AMS_MS_INVALID_STATE 0xA0
/** AMS error code: The command format is invalid. */
#define BT_ATT_ERR_AMS_MS_INVALID_COMMAND 0xA1
/** AMS error code: The entity attribute is empty. */
#define BT_ATT_ERR_AMS_MS_ABSENT_ATTRIBUTE 0xA2

struct bt_ams_client;

/**@brief Notification callback function.
 *
 * @param[in] ams_c     AMS client instance.
 * @param[in] data      Data pointer.
 * @param[in] len       Data length.
 */
typedef void (*bt_ams_notify_cb)(struct bt_ams_client *ams_c,
				 const uint8_t *data, size_t len);

/**@brief Entity Update notification callback function.
 *
 * @param[in] ams_c     AMS client instance.
 * @param[in] notif     Notification data.
 * @param[in] err       0 if the notification is valid.
 *                      Otherwise, contains a (negative) error code.
 */
typedef void (*bt_ams_entity_update_notify_cb)(
				struct bt_ams_client *ams_c,
				const struct bt_ams_entity_update_notif *notif,
				int err);

/**@brief Write response callback function.
 *
 * @param[in] ams_c     AMS client instance.
 * @param[in] err       ATT error code.
 */
typedef void (*bt_ams_write_cb)(struct bt_ams_client *ams_c,
				uint8_t err);

/**@brief Read response callback function.
 *
 * @param[in] ams_c     AMS client instance.
 * @param[in] err       ATT error code.
 * @param[in] data      Data pointer.
 * @param[in] len       Data length.
 */
typedef void (*bt_ams_read_cb)(struct bt_ams_client *ams_c,
			       uint8_t err, const uint8_t *data, size_t len);

struct bt_ams_client {
	/** Connection object. */
	struct bt_conn *conn;

	/** Handle of the Remote Command Characteristic. */
	uint16_t handle_rc;

	/** Handle of the CCCD of the Remote Command Characteristic. */
	uint16_t handle_rc_ccc;

	/** Handle of the Entity Update Characteristic. */
	uint16_t handle_eu;

	/** Handle of the CCCD of the Entity Update Characteristic. */
	uint16_t handle_eu_ccc;

	/** Handle of the Entity Attribute Characteristic. */
	uint16_t handle_ea;

	/** GATT subscribe parameters for the Remote Command Characteristic. */
	struct bt_gatt_subscribe_params notif_params_rc;

	/** Callback function for the Remote Command notification. */
	bt_ams_notify_cb notify_rc_cb;

	/** GATT write parameters for the Remote Command Characteristic. */
	struct bt_gatt_write_params write_params_rc;

	/** Callback function for the Remote Command write. */
	bt_ams_write_cb write_rc_cb;

	/** Data buffer for the Remote Command write. */
	uint8_t data_rc[1];

	/** GATT subscribe parameters for the Entity Update Characteristic. */
	struct bt_gatt_subscribe_params notif_params_eu;

	/** Callback function for the Entity Update notification. */
	bt_ams_entity_update_notify_cb notify_eu_cb;

	/** GATT write parameters for the Entity Update Characteristic. */
	struct bt_gatt_write_params write_params_eu;

	/** Callback function for the Entity Update write. */
	bt_ams_write_cb write_eu_cb;

	/** Data buffer for the Entity Update write. */
	uint8_t data_eu[BT_AMS_ATTRIBUTE_COUNT_MAX + 1];

	/** GATT write parameters for the Entity Attribute Characteristic. */
	struct bt_gatt_write_params write_params_ea;

	/** Callback function for the Entity Attribute write. */
	bt_ams_write_cb write_ea_cb;

	/** Data buffer for the Entity Attribute write. */
	uint8_t data_ea[2];

	/** GATT read parameters for the Entity Attribute Characteristic. */
	struct bt_gatt_read_params read_params_ea;

	/** Callback function for the Entity Attribute read. */
	bt_ams_read_cb read_ea_cb;

	/** Internal state. */
	atomic_t state;
};

/**@brief Initialize the AMS Client instance.
 *
 * @param[out] ams_c       AMS client instance.
 *
 * @retval 0 If the client was initialized successfully.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ams_client_init(struct bt_ams_client *ams_c);

/**@brief Assign handles to the AMS Client instance.
 *
 * @details Call this function when a link has been established with a peer to
 *          associate the link to this instance of the module. This makes it
 *          possible to handle several links and associate each link to a particular
 *          instance of this module.
 *
 * @param[in]     dm     Discovery object.
 * @param[in,out] ams_c  AMS client instance.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ams_handles_assign(struct bt_gatt_dm *dm,
			  struct bt_ams_client *ams_c);

/**@brief Subscribe to the Remote Command notification.
 *
 * @param[in] ams_c       AMS client instance.
 * @param[in] func        Notification callback function handler.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ams_subscribe_remote_command(struct bt_ams_client *ams_c,
				    bt_ams_notify_cb func);

/**@brief Unsubscribe from the Remote Command notification.
 *
 * @param[in] ams_c       AMS client instance.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ams_unsubscribe_remote_command(struct bt_ams_client *ams_c);

/**@brief Subscribe to the Entity Update notification.
 *
 * @param[in] ams_c       AMS client instance.
 * @param[in] func        Notification callback function handler.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ams_subscribe_entity_update(struct bt_ams_client *ams_c,
				   bt_ams_entity_update_notify_cb func);

/**@brief Unsubscribe from the Entity Update notification.
 *
 * @param[in] ams_c       AMS client instance.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ams_unsubscribe_entity_update(struct bt_ams_client *ams_c);

/**@brief Write data to the Remote Command characteristic.
 *
 * @param[in] ams_c       AMS client instance.
 * @param[in] command     Remote Command ID.
 * @param[in] func        Write callback function handler.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ams_write_remote_command(struct bt_ams_client *ams_c,
				enum bt_ams_remote_command_id command,
				bt_ams_write_cb func);

/**@brief Write data to the Entity Update characteristic.
 *
 * @param[in] ams_c          AMS client instance.
 * @param[in] ent_attr_list  Entity Attribute list.
 * @param[in] func           Write callback function handler.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ams_write_entity_update(struct bt_ams_client *ams_c,
			       const struct bt_ams_entity_attribute_list *ent_attr_list,
			       bt_ams_write_cb func);

/**@brief Write data to the Entity Attribute characteristic.
 *
 * @param[in] ams_c       AMS client instance.
 * @param[in] ent_attr    Entity Attribute pair.
 * @param[in] func        Write callback function handler.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ams_write_entity_attribute(struct bt_ams_client *ams_c,
				  const struct bt_ams_entity_attribute *ent_attr,
				  bt_ams_write_cb func);

/**@brief Read data from the Entity Attribute characteristic.
 *
 * @param[in] ams_c       AMS client instance.
 * @param[in] func        Read callback function handler.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_ams_read_entity_attribute(struct bt_ams_client *ams_c,
				 bt_ams_read_cb func);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_AMS_CLIENT_H_ */
