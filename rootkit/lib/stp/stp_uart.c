#include <kernel.h>
#include <init.h>
#include <net/buf.h>
#include <drivers/uart.h>
#include <sys/atomic.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(stp_uart);

#include "stp/stp_core.h"


enum {
    RX_STATE_FIRST,
    RX_STATE_CONT
};

struct stp_uartpriv {
    const struct device *dev;
    struct k_fifo rx_queue;
    sys_slist_t tx_queue;
    struct net_buf *curr_tx;
    struct net_buf *curr_rx;
    atomic_t tx_actived;
    int64_t time;
};


#define CONFIG_STP_RX_PRIO 2
#define CONFIG_STP_UART_RX_TIMEOUT     50 /* 25ms */
#define CONFIG_STP_UART_RX_BUFSZ       280
#define CONFIG_STP_UART_RX_BUFSZ_PRIV 4

NET_BUF_POOL_DEFINE(rx_pool, 4, CONFIG_STP_UART_RX_BUFSZ, 
    CONFIG_STP_UART_RX_BUFSZ_PRIV, NULL);
static struct stp_uartpriv stp_lldev = {
    .rx_queue = Z_FIFO_INITIALIZER(stp_lldev.rx_queue),
    .tx_queue = SYS_SLIST_STATIC_INIT(&stp_lldev.tx_queue)
};    

static K_KERNEL_STACK_DEFINE(rx_thread_stack, 1024);
static struct k_thread rx_thread_data;


static void fifo_dummy_read(const struct device *dev)
{
    uint8_t dummy[4];
    while (uart_fifo_read(dev, dummy, sizeof(dummy)));
}

static void uart_tx_process(const struct device *dev, 
    struct stp_uartpriv *priv)
{
    struct net_buf *tx = priv->curr_tx;
    int len;

    if (!tx) {
        tx = net_buf_slist_get(&priv->tx_queue);
        if (!tx) {
            uart_irq_tx_disable(dev);
            atomic_set(&priv->tx_actived, 0);
            return;
        }
        priv->curr_tx = tx;
    }

    len = uart_fifo_fill(dev, tx->data, tx->len);
    net_buf_pull_mem(tx, len);
    if (!tx->len) {
        net_buf_unref(tx);
        priv->curr_tx = NULL;
    }
}

static void uart_rx_process(const struct device *dev, 
    struct stp_uartpriv *priv)
{
    struct net_buf *rx = priv->curr_rx;
    int req_len;
    int len;
    
    if (!rx) {
        rx = net_buf_alloc(&rx_pool, K_NO_WAIT);
        if (!rx) {
            LOG_ERR("%s(): No more rx-buf and flush read fifo\n", __func__);
            fifo_dummy_read(dev);
            return;
        }
        priv->curr_rx = rx;
        priv->time = k_uptime_get();
    } else {
        if (k_uptime_delta(&priv->time) >= CONFIG_STP_UART_RX_TIMEOUT)
            net_buf_reset(rx);
    }

    req_len = net_buf_tailroom(rx);
    if (!req_len) {
        LOG_ERR("%s(): Receive buffer is no enough\n", __func__);
        goto _drop_rx;
    }

    len = (uint16_t)uart_fifo_read(dev, net_buf_tail(rx), req_len);
    net_buf_add(rx, len);
    if (rx->len >= sizeof(struct stp_header)) {
        struct stp_header *hdr = (struct stp_header *)rx->data;
        uint16_t length = ntols(hdr->length);
        if (rx->len == length) {
            net_buf_put(&priv->rx_queue, rx);
            priv->curr_rx = NULL;
        } else if (rx->len > length) {
            LOG_ERR("%s(): Rx-packet length is invalid\n", __func__);
            goto _drop_rx;
        }
    }
    return;
_drop_rx:
    fifo_dummy_read(dev);
    LOG_ERR("%s(): receive frame error\n", __func__);
}

static void stp_uart_isr(const struct device *dev,
    void *user_data)
{
    struct stp_uartpriv *priv = user_data;
    ARG_UNUSED(user_data);

    uart_irq_update(dev);             
    if (uart_irq_rx_ready(dev)) 
        uart_rx_process(dev, priv);

    if (uart_irq_tx_ready(dev))
        uart_tx_process(dev, priv);
}

static void stp_rx_thread(void *arg)
{
    struct stp_uartpriv *priv = arg;

    for ( ; ; ) {
        struct net_buf *buf = net_buf_get(&priv->rx_queue, K_FOREVER);
        stp_driver_input(buf);
    }
}

static int stp_uart_setup(const struct stp_driver *drv)
{
    struct stp_uartpriv *priv = drv->private;
    const struct device *dev = priv->dev;
    struct uart_config cfg;
    k_tid_t rx;
    int ret;

    ARG_UNUSED(dev);
    cfg.baudrate = 115200;
    cfg.data_bits = UART_CFG_DATA_BITS_8;
    cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
    cfg.parity = UART_CFG_PARITY_NONE;
    cfg.stop_bits = UART_CFG_STOP_BITS_1;
    ret = uart_configure(dev, &cfg);
    if (ret) {
        LOG_ERR("%s(): configure uart failed: %d\n", __func__, ret);
        return ret;
    }

    rx = k_thread_create(&rx_thread_data, rx_thread_stack,
                    K_KERNEL_STACK_SIZEOF(rx_thread_stack),
                    (k_thread_entry_t)stp_rx_thread, 
                    &stp_lldev, NULL, NULL,
                    K_PRIO_COOP(CONFIG_STP_RX_PRIO),
                    0, K_FOREVER);
    k_thread_name_set(rx, "/protocol@stp-uart");
    k_thread_start(rx);

    fifo_dummy_read(dev);
    uart_irq_callback_user_data_set(dev, stp_uart_isr, priv);
    uart_irq_rx_enable(dev);
    return 0;
}

static int stp_uart_send(const struct stp_driver *drv, 
    struct net_buf *buf)
{
    struct stp_uartpriv *priv = drv->private;
    ARG_UNUSED(drv);
    
    net_buf_slist_put(&priv->tx_queue, buf);
    if (!atomic_set(&priv->tx_actived, 1)) {
        unsigned int key = irq_lock();
        if (uart_irq_tx_ready(priv->dev))
            uart_tx_process(priv->dev, priv);
        irq_unlock(key);
        uart_irq_tx_enable(priv->dev);
    }
    return 0;
}

static const struct stp_driver stp_drv = {
    .name = "UART",
    .setup = stp_uart_setup,
    .output = stp_uart_send,
    .private = &stp_lldev
};

static int stp_uart_init(const struct device *dev)
{
    struct stp_uartpriv *priv = stp_drv.private;

    priv->dev = device_get_binding(CONFIG_STP_UART_DEV);
    if (!priv->dev)
        return -EINVAL;

    //uart_irq_rx_disable(priv->dev);
    //uart_irq_tx_disable(priv->dev);
    return stp_driver_register(&stp_drv);
}

SYS_INIT(stp_uart_init, POST_KERNEL, 
    CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
