#include <version.h>
#include <kernel.h>
#include <init.h>
#include <net/buf.h>
#include <sys/atomic.h>
#include <sys/__assert.h>

#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(stp_bt);

#include "stp/stp_core.h"


#if KERNEL_VERSION_NUMBER < ZEPHYR_VERSION(2,6,0)
#define CONFIG_STP_BT_RX_BUFSZ  MIN(CONFIG_BT_L2CAP_TX_MTU, CONFIG_BT_L2CAP_RX_MTU) //280
#else
#define CONFIG_STP_BT_RX_BUFSZ CONFIG_BT_L2CAP_TX_MTU
#endif

#define CONFIG_STP_RX_BUFSZ_PRIV 4

/* UUID of the STP Service. **/
#define BT_UUID_STP_VAL \
	BT_UUID_128_ENCODE(0x50200001, 0xb5a3, 0x1000, 0x8000, 0xe50e24dcca9e)

/* UUID of the TX Characteristic. **/
#define BT_UUID_STP_TX_VAL \
	BT_UUID_128_ENCODE(0x50200002, 0xb5a3, 0x1000, 0x8000, 0xe50e24dcca9e)

/* UUID of the RX Characteristic. **/
#define BT_UUID_STP_RX_VAL \
	BT_UUID_128_ENCODE(0x50200003, 0xb5a3, 0x1000, 0x8000, 0xe50e24dcca9e)

#define BT_UUID_STP_SERVICE   BT_UUID_DECLARE_128(BT_UUID_STP_VAL)
#define BT_UUID_STP_RX        BT_UUID_DECLARE_128(BT_UUID_STP_RX_VAL)
#define BT_UUID_STP_TX        BT_UUID_DECLARE_128(BT_UUID_STP_TX_VAL)


struct stp_btpriv {
    struct k_fifo rx_queue;
    struct bt_conn_cb cb;
    struct bt_gatt_exchange_params params;
    struct bt_conn *conn;
};

NET_BUF_POOL_DEFINE(rx_pool, 4, CONFIG_STP_BT_RX_BUFSZ, 
    CONFIG_STP_RX_BUFSZ_PRIV, NULL);
static struct stp_btpriv stp_btdev = {
    .rx_queue = Z_FIFO_INITIALIZER(stp_btdev.rx_queue),
};    


static ssize_t stp_bt_receive_cb(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             const void *buf,
                             uint16_t len,
                             uint16_t offset,
                             uint8_t flags)
{
    struct net_buf *rx;
    __ASSERT(len < CONFIG_STP_UART_RX_BUFSZ, "Received packet is too large");
    ARG_UNUSED(conn);
    ARG_UNUSED(attr);
    ARG_UNUSED(flags);
    rx = net_buf_alloc(&rx_pool, K_NO_WAIT);
    if (!rx) {
        LOG_ERR("%s(): No more memory\n", __func__);
        return 0;
    }

    net_buf_add_mem(rx, (const char *)buf + offset, len);
    if (stp_driver_input(rx) < 0) {
        net_buf_unref(rx);
        LOG_ERR("%s(): Data packet error\n", __func__);
        return 0;
    }
	return len;
}

static void on_sent(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(conn);
    ARG_UNUSED(user_data);
}

/* STP Service Declaration */
BT_GATT_SERVICE_DEFINE(stp_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_STP_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_STP_TX,
			       BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       NULL, NULL, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_STP_RX,
			       BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       NULL, stp_bt_receive_cb, NULL),
);


static void on_mtu_exchange(struct bt_conn *conn, uint8_t err,
         struct bt_gatt_exchange_params *params)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(params);
    if (!err) {
        LOG_DBG("Exchange MTU successful\n");
    }
}

static void stp_bt_connected(struct bt_conn *conn, uint8_t err)
{
    if (!err) {
        stp_btdev.conn = bt_conn_ref(conn);
        bt_gatt_exchange_mtu(conn, &stp_btdev.params);
    }
}

static void stp_bt_disconnected(struct bt_conn *conn, uint8_t err)
{
    if (conn == stp_btdev.conn) {
        bt_conn_unref(conn);
        stp_btdev.conn = NULL;
    }
}

static int stp_bt_setup(const struct stp_driver *drv)
{
    ARG_UNUSED(drv);
    return 0;
}

static int stp_bt_send(const struct stp_driver *drv, 
    struct net_buf *buf)
{
    struct stp_btpriv *priv = drv->private;
    const struct bt_gatt_attr *attr = &stp_svc.attrs[2];
    struct bt_gatt_notify_params params;
    
    params.uuid = NULL;
    params.attr = attr;
    params.data = buf->data;
    params.len = buf->len;
    params.func = on_sent;
    params.user_data = priv;
    return bt_gatt_notify_cb(priv->conn, &params);
}

static const struct stp_driver stp_drv = {
    .name = "BT",
    .setup = stp_bt_setup,
    .output = stp_bt_send,
    .private = &stp_btdev
};

static int stp_bt_init(const struct device *dev)
{
    struct stp_btpriv *priv = stp_drv.private;

    ARG_UNUSED(dev);
    priv->cb.connected = stp_bt_connected;
    priv->cb.disconnected = stp_bt_disconnected;
    priv->params.func = on_mtu_exchange;
    bt_conn_cb_register(&priv->cb);
    return stp_driver_register(&stp_drv);
}

SYS_INIT(stp_bt_init, POST_KERNEL, 
    CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

