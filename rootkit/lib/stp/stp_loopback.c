#include <kernel.h>
#include <init.h>
#include <net/buf.h>
#include <sys/atomic.h>
#include <sys/crc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(stp_loopback);

#include "stp/stp_core.h"


enum {
    RX_STATE_FIRST,
    RX_STATE_CONT
};

struct stp_drvpriv {
    struct k_fifo tx_queue;
};


#define CONFIG_STP_RX_PRIO 2
#define CONFIG_STP_RX_BUFSZ       280
#define CONFIG_STP_RX_BUFSZ_PRIV 4


NET_BUF_POOL_DEFINE(rx_pool, 4, CONFIG_STP_RX_BUFSZ, 
    CONFIG_STP_RX_BUFSZ_PRIV, NULL);
static struct stp_drvpriv stp_lldev = {
    .tx_queue = Z_FIFO_INITIALIZER(stp_lldev.tx_queue),
};    

static K_KERNEL_STACK_DEFINE(rx_thread_stack, 2048);
static struct k_thread rx_thread_data;

static inline uint16_t stp_crc16(const uint8_t *src, size_t len)
{
    return crc16_itu_t(0, src, len);
}

static void stp_loopback_thread(void *arg)
{
    struct stp_drvpriv *priv = arg;
    struct net_buf *buf, *rx;
    uint16_t crc;
    uint16_t etx;

    for ( ; ; ) {
        buf = net_buf_get(&priv->tx_queue, K_FOREVER);
        if (!buf) {
            LOG_ERR("%s(): Get tx buffer failed\n", __func__);
            continue;
        }

        rx = net_buf_alloc(&rx_pool, K_NO_WAIT);
        if (!rx) {
            LOG_ERR("%s(): No more buffers\n", __func__);
            net_buf_unref(buf);
            continue;
        }

        struct stp_header *hdr = (struct stp_header *)buf->data;
        hdr->require_ack = 0;
        hdr->request = 0;
        hdr->upload = 0;
        buf->len = buf->len - sizeof(crc) - sizeof(etx);
        crc = stp_crc16(&buf->data[2], buf->len - 2);
        crc = ltons(crc);
        etx = ltons(STP_ETX);
        net_buf_add_mem(rx, buf->data, buf->len);
        net_buf_add_mem(rx, &crc, sizeof(crc));
        net_buf_add_mem(rx, &etx, sizeof(etx));
        stp_driver_input(rx);
    }
}

static int stp_loopback_setup(const struct stp_driver *drv)
{
    k_tid_t rx;
    rx = k_thread_create(&rx_thread_data, rx_thread_stack,
                    K_KERNEL_STACK_SIZEOF(rx_thread_stack),
                    (k_thread_entry_t)stp_loopback_thread, 
                    &stp_lldev, NULL, NULL,
                    K_PRIO_COOP(CONFIG_STP_RX_PRIO),
                    0, K_FOREVER);
    k_thread_name_set(rx, "/protocol@stp-loopback");
    k_thread_start(rx);
    return 0;
}

static int stp_loopback_send(const struct stp_driver *drv, 
    struct net_buf *buf)
{
    struct stp_drvpriv *priv = drv->private;
    net_buf_put(&priv->tx_queue, buf);
    return 0;
}

static const struct stp_driver stp_drv = {
    .name = "UART",
    .setup = stp_loopback_setup,
    .output = stp_loopback_send,
    .private = &stp_lldev
};

static int stp_loopback_init(const struct device *dev)
{
    ARG_UNUSED(dev);
    return stp_driver_register(&stp_drv);
}

SYS_INIT(stp_loopback_init, POST_KERNEL, 
    CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

