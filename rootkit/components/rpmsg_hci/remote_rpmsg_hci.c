#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <arch/cpu.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <drivers/ipm.h>

#include <openamp/open_amp.h>
#include <metal/sys.h>
#include <metal/device.h>
#include <metal/alloc.h>

#include <ipc/rpmsg_service.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/buf.h>
#include <bluetooth/hci_raw.h>

#define HCI_RPMSG_CMD 0x01
#define HCI_RPMSG_ACL 0x02
#define HCI_RPMSG_SCO 0x03
#define HCI_RPMSG_EVT 0x04
#define HCI_RPMSG_ISO 0x05

#define BT_DBG_ENABLED 0
#define LOG_MODULE_NAME hci_rpmsg
#include "common/log.h"


#ifndef CONFIG_RPMSG_HCI_SMALL_FOOTPRINT
static K_THREAD_STACK_DEFINE(tx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread tx_thread_data;
static K_FIFO_DEFINE(tx_queue);
#endif /* CONFIG_RPMSG_HCI_SMALL_FOOTPRINT */
static K_THREAD_STACK_DEFINE(rx_thread_stack, 512);
static struct k_thread rx_thread_data;
static K_FIFO_DEFINE(rx_queue);

static int bt_hci_endpoint_id;

static struct net_buf *hci_rpmsg_cmd_recv(uint8_t *data, size_t remaining) {
	struct bt_hci_cmd_hdr *hdr = (void *)data;
	struct net_buf *buf;
	if (remaining < sizeof(*hdr)) {
		LOG_ERR("Not enought data for command header");
		return NULL;
	}
	buf = bt_buf_get_tx(BT_BUF_CMD, K_NO_WAIT, hdr, sizeof(*hdr));
	if (buf) {
		data += sizeof(*hdr);
		remaining -= sizeof(*hdr);
	} else {
		LOG_ERR("No available command buffers!");
		return NULL;
	}
	if (remaining != hdr->param_len) {
		LOG_ERR("Command payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}
	LOG_DBG("len %u", hdr->param_len);
	net_buf_add_mem(buf, data, remaining);
	return buf;
}

static struct net_buf *hci_rpmsg_acl_recv(uint8_t *data, size_t remaining) {
	struct bt_hci_acl_hdr *hdr = (void *)data;
	struct net_buf *buf;
	if (remaining < sizeof(*hdr)) {
		LOG_ERR("Not enought data for ACL header");
		return NULL;
	}
	buf = bt_buf_get_tx(BT_BUF_ACL_OUT, K_NO_WAIT, hdr, sizeof(*hdr));
	if (buf) {
		data += sizeof(*hdr);
		remaining -= sizeof(*hdr);
	} else {
		LOG_ERR("No available ACL buffers!");
		return NULL;
	}
	if (remaining != sys_le16_to_cpu(hdr->len)) {
		LOG_ERR("ACL payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}
	LOG_DBG("len %u", remaining);
	net_buf_add_mem(buf, data, remaining);
	return buf;
}

static struct net_buf *hci_rpmsg_iso_recv(uint8_t *data, size_t remaining) {
	struct bt_hci_iso_hdr *hdr = (void *)data;
	struct net_buf *buf;
	if (remaining < sizeof(*hdr)) {
		LOG_ERR("Not enough data for ISO header");
		return NULL;
	}
	buf = bt_buf_get_tx(BT_BUF_ISO_OUT, K_NO_WAIT, hdr, sizeof(*hdr));
	if (buf) {
		data += sizeof(*hdr);
		remaining -= sizeof(*hdr);
	} else {
		LOG_ERR("No available ISO buffers!");
		return NULL;
	}
	if (remaining != sys_le16_to_cpu(hdr->len)) {
		LOG_ERR("ISO payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}
	LOG_DBG("len %zu", remaining);
	net_buf_add_mem(buf, data, remaining);
	return buf;
}

static void hci_rpmsg_rx(uint8_t *data, size_t len) {
	uint8_t pkt_indicator;
	struct net_buf *buf = NULL;
	size_t remaining = len;
	LOG_HEXDUMP_DBG(data, len, "RPMSG data:");
	pkt_indicator = *data++;
	remaining -= sizeof(pkt_indicator);
	switch (pkt_indicator) {
	case HCI_RPMSG_CMD:
		buf = hci_rpmsg_cmd_recv(data, remaining);
		break;
	case HCI_RPMSG_ACL:
		buf = hci_rpmsg_acl_recv(data, remaining);
		break;
	case HCI_RPMSG_ISO:
		buf = hci_rpmsg_iso_recv(data, remaining);
		break;
	default:
		LOG_ERR("Unknown HCI type %u", pkt_indicator);
		return;
	}
	if (buf) {
#ifdef CONFIG_RPMSG_HCI_SMALL_FOOTPRINT
		int err = bt_send(buf);
		if (err) {
			LOG_ERR("Unable to send (err %d)", err);
			net_buf_unref(buf);
		}
#else  /* !CONFIG_RPMSG_HCI_SMALL_FOOTPRINT */
		net_buf_put(&tx_queue, buf);
#endif /* CONFIG_RPMSG_HCI_SMALL_FOOTPRINT */
		LOG_HEXDUMP_DBG(buf->data, buf->len, "Final net buffer:");
	}
}

static int hci_rpmsg_send(struct net_buf *buf) {
	uint8_t pkt_indicator;
	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf),
		buf->len);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "Controller buffer:");
	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_IN:
		pkt_indicator = HCI_RPMSG_ACL;
		break;
	case BT_BUF_EVT:
		pkt_indicator = HCI_RPMSG_EVT;
		break;
	case BT_BUF_ISO_IN:
		pkt_indicator = HCI_RPMSG_ISO;
		break;
	default:
		LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
		net_buf_unref(buf);
		return -EINVAL;
	}
	net_buf_push_u8(buf, pkt_indicator);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "Final HCI buffer:");
	rpmsg_service_send(bt_hci_endpoint_id, buf->data, buf->len);
	net_buf_unref(buf);
	return 0;
}

#if defined(CONFIG_BT_CTLR_ASSERT_HANDLER)
void bt_ctlr_assert_handle(char *file, uint32_t line)
{
	BT_ASSERT_MSG(false, "Controller assert in: %s at %d", file, line);
}
#endif /* CONFIG_BT_CTLR_ASSERT_HANDLER */

#ifndef CONFIG_RPMSG_HCI_SMALL_FOOTPRINT
static void bt_rpmsg_hci_tx_thread(void *p1, void *p2, void *p3) {
	struct net_buf *buf;
	int err;

	for ( ; ; ) {
		buf = net_buf_get(&tx_queue, K_FOREVER);
		err = bt_send(buf);
		if (err) {
			LOG_ERR("Unable to send (err %d)", err);
			net_buf_unref(buf);
		}
		k_yield();
	}
}
#endif /* CONFIG_RPMSG_HCI_SMALL_FOOTPRINT */

static void bt_rpmsg_hci_rx_thread(void *p1, void *p2, void *p3) {
	struct net_buf *buf;
	int err;

	for ( ; ; ) {
		buf = net_buf_get(&rx_queue, K_FOREVER);
		err = hci_rpmsg_send(buf);
		if (err) {
			LOG_ERR("Failed to send (err %d)", err);
		}
	}
}

void bt_remote_rpmsg_hci_setup(void) {
	bt_enable_raw(&rx_queue);
#ifndef CONFIG_RPMSG_HCI_SMALL_FOOTPRINT
	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_THREAD_STACK_SIZEOF(tx_thread_stack), bt_rpmsg_hci_tx_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	k_thread_name_set(&tx_thread_data, "HCI rpmsg TX");
#endif /* CONFIG_RPMSG_HCI_SMALL_FOOTPRINT */
	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_THREAD_STACK_SIZEOF(rx_thread_stack), bt_rpmsg_hci_rx_thread,
			NULL, NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);
	k_thread_name_set(&rx_thread_data, "HCI rpmsg RX");
}

static int hci_endpoint_cb(struct rpmsg_endpoint *ept, void *data, 
	size_t len, uint32_t src, void *priv) {
	(void) ept;
	(void) src;
	(void) priv;
	LOG_INF("Received message of %u bytes.", len);
	hci_rpmsg_rx((uint8_t *) data, len);
	return RPMSG_SUCCESS;
}

static int bt_remote_register_endpoint(const struct device *arg __unused) {
	int status;
	status = rpmsg_service_register_endpoint(CONFIG_RPMSG_HCI_ENDPOINT_NAME, 
		hci_endpoint_cb);
	if (status < 0) {
		LOG_ERR("Registering endpoint failed with %d", status);
		return status;
	}
	bt_hci_endpoint_id = status;
	return 0;
}

SYS_INIT(bt_remote_register_endpoint, POST_KERNEL, 
	CONFIG_RPMSG_SERVICE_EP_REG_PRIORITY);
