#include <init.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/nus.h>


#define LOG_BUFFER_SIZE 256

K_PIPE_DEFINE(bt_pipe, LOG_BUFFER_SIZE, 4);
static struct bt_conn *console_conn;
static uint8_t log_buffer[LOG_BUFFER_SIZE];
static uint16_t log_index;
static void *pr_hook;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
              0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
              0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
};

static int bt_console_out(int ch)
{
    size_t writen;

    log_buffer[log_index++] = ch & 0xFF;
    if (ch == '\n') {
        log_buffer[log_index++] = '\r';
        goto _send;
    }
    
    if (log_index > LOG_BUFFER_SIZE-2)
        goto _send;
        
    return ch;
_send: 
    k_pipe_put(&bt_pipe, log_buffer, log_index, &writen,
			 2, K_NO_WAIT);
    log_index = 0;
    ARG_UNUSED(writen);
    return ch;
}

static void on_exchange(struct bt_conn *conn, uint8_t err,
         struct bt_gatt_exchange_params *params)
{
    extern void __printk_hook_install(int (*)(int));
    if (err) {
        printk("%s(): Exchange failed\n", __func__);
        return;
    }
    __printk_hook_install(bt_console_out);
}

static void bt_console_connected(struct bt_conn *conn, uint8_t err)
{
    extern void *__printk_get_hook(void);
    static struct bt_gatt_exchange_params param;
    int ret;

    if (err) {
        printk("Connection failed (err 0x%02x)\n", err);
        return;
    }

    printk("Connected\n");
    pr_hook = __printk_get_hook();
    console_conn = bt_conn_ref(conn);  
    param.func = on_exchange;
	ret = bt_gatt_exchange_mtu(console_conn, &param);
    if (ret)
        printk("Exchange failed (err %d)", ret);
}

static void bt_console_disconnected(struct bt_conn *conn, uint8_t reason)
{
    extern void __printk_hook_install(int (*)(int));
    printk("Disconnected (reason 0x%02x)\n", reason);
    if (console_conn) {
        __printk_hook_install(pr_hook);
        bt_conn_unref(console_conn);
        console_conn = NULL;
    }
}

static struct bt_conn_cb conn_callbacks = {
    .connected = bt_console_connected,
    .disconnected = bt_console_disconnected,
};

static void bt_ready(int err)
{
    if (err) {
        printk("Bluetooth initialize failed: %d\n", err);
        return;
    }

    printk("Bluetooth initialized\n");
    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
    .cancel = auth_cancel,
};

static void bt_console_thread(void)
{
    uint8_t buffer[256];
    size_t len;
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_ready(0);

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	for ( ; ; ) {
        len = 0;
        err = k_pipe_get(&bt_pipe, buffer, 256, &len, 2, K_MSEC(500)); 
        if (!err && len > 0)
            bt_nus_send(console_conn, buffer, len);
	}
}

K_THREAD_DEFINE(btcon_thread, 2048, 
                bt_console_thread, NULL, NULL, NULL, 
                10, 0, 0);
                
