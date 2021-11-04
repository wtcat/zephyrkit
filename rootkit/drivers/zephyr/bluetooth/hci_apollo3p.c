#include <init.h>
#include <kernel.h>
#include <bluetooth/hci.h>
#include <drivers/bluetooth/hci_driver.h>
#include <net/buf.h>
#include <sys/byteorder.h>
#include <soc.h>

#include "common/log.h"

#ifdef CONFIG_BT_HCI_STAT
#include <shell/shell.h>
#endif

#ifdef CONFIG_BT_RECV_IS_RX_THREAD
#error "Not support this configure option!!"
#endif


#define HCI_DRIVER_LL_DEBUG

#define HCI_PACKET_SIZE (512 - sizeof(sys_snode_t) - sizeof(uint32_t))

#define HCI_CMD			0x01
#define HCI_ACL			0x02
#define HCI_SCO			0x03
#define HCI_EVT			0x04

#define PACKET_TYPE		0
#define EVT_HEADER_TYPE		0
#define EVT_HEADER_EVENT	1
#define EVT_HEADER_SIZE		2
#define EVT_VENDOR_CODE_LSB	3
#define EVT_VENDOR_CODE_MSB	4


struct hci_packet {
    sys_snode_t node;
    uint32_t len;
    union {
        uint32_t buf32[HCI_PACKET_SIZE / 4];
        uint8_t buf[HCI_PACKET_SIZE];
    };
};

struct slist_head {
    struct k_spinlock lock;
    sys_slist_t head;
};

struct hci_drv_statistics {
    unsigned int err_rx;
    unsigned int err_rx_nomem;
    unsigned int err_tx;
    unsigned int err_rx_packet;
    unsigned int err_tx_packet;
    unsigned int tx_interrupt;
    unsigned int rx_interrupt;
    unsigned int txcnt;
    unsigned int rxcnt;
    unsigned int tx_cmp_delayed;
    unsigned int rx_cmp_delayed;
};

struct xmit_monitor {
    uint32_t request;
    uint32_t ack;
};

struct drv_monitor {
    struct xmit_monitor rx;
    struct xmit_monitor tx;
};


#define BLE_DEVNO 0
#define BLE_INTR_MASK (\
        AM_HAL_BLE_INT_CMDCMP | \
        AM_HAL_BLE_INT_DCMP | \
        AM_HAL_BLE_INT_BLECIRQ | \
        AM_HAL_BLE_INT_BLECSSTAT)

/* BLE status flags */
#define BLE_STAT_SPI BIT(3)
#define BLE_STAT_IRQ BIT(7)


static K_SEM_DEFINE(bt_sem, 1, 1);
static K_FIFO_DEFINE(rx_queue);
static K_KERNEL_STACK_DEFINE(rx_thread_stack, CONFIG_HCI_DRIVER_STACK_SIZE);
static struct k_thread rx_thread_pcb;
static K_MEM_SLAB_DEFINE(rx_pool, sizeof(struct hci_packet), 4, 4);

static sys_slist_t tx_queue;
static void *ble_handle;
static bool radio_cold_boot;
static volatile struct net_buf *tx_buf_pending;

#ifdef HCI_DRIVER_LL_DEBUG
static struct drv_monitor drv_mon;
#define DRV_REQ_INC(_m) ({struct xmit_monitor *__p = &drv_mon._m; \
                        __p->request++; __p->request;})
#define DRV_ACK_INC(_m) ({struct xmit_monitor *__p = &drv_mon._m; \
                         __p->ack++; __p->ack;})
#else
#define DRV_REQ_INC(...)
#define DRV_ACK_INC(...)
#endif /* HCI_DRIVER_LL_DEBUG */

#ifdef CONFIG_BT_HCI_STAT
static struct hci_drv_statistics bt_stat;
#define BT_STAT_INC(field) bt_stat.field++
#else
#define BT_STAT_INC(...)
#endif

#ifdef HCI_DRIVER_LL_DEBUG
static void dump_hci_xmit_infomation(void)
{
    printk("HCI tx-req(%d), tx-ack(%d)\n"
            "HCI rx-req(%d), rx-ack(%d)\n", 
            drv_mon.tx.request, drv_mon.tx.ack,
            drv_mon.rx.request, drv_mon.rx.ack);
}
#else
#define dump_hci_xmit_infomation()
#endif

#ifdef CONFIG_BT_HCI_DEBUG
static void dump_hci_buffer(struct net_buf *buf, bool tx)
{
    uint8_t *p = buf->data;
    int len = buf->len - 1;

    if (tx) {
        len = buf->len - 1;
        printk("Send: \n> %02x", *p);
        p++;
    } else {
        len = buf->len;
        printk("Recv: \n");
    }
    for (int i = 0; i < len; i++) {
        if (!(i & 0x7))
            printk("\n> ");
        printk("%02x ", p[i]);
    }
    printk("\n");
}
#endif /* CONFIG_BT_HCI_DEBUG */

static bool is_tx_queue_empty(sys_slist_t *q)
{
   unsigned int key;
   bool empty;

   key = irq_lock();
   empty =  sys_slist_is_empty(q) && !tx_buf_pending;
   irq_unlock(key);
   return empty;
}

static void bt_start_xmit(void)
{
    unsigned int key;

    key = irq_lock();
    if (!is_tx_queue_empty(&tx_queue) &&
        !(BLEIFn(BLE_DEVNO)->BSTATUS & (BLE_STAT_SPI|BLE_STAT_IRQ))) {
        am_hal_ble_wakeup_set(ble_handle, 1);

        /* If we've set wakeup, but IRQ came up at the same time, we should
         * just lower WAKE again.
         */
        if (BLEIFn(BLE_DEVNO)->BSTATUS_b.BLEIRQ)
            am_hal_ble_wakeup_set(ble_handle, 0);
    }
    irq_unlock(key);
}

static int hci_assert_status(uint32_t flags)
{
    int trycnt = 0;
    int ret = 0;
    
    while (BLEIFn(BLE_DEVNO)->BSTATUS & flags) {
        if (trycnt == 0) {
            if (flags & BLE_STAT_SPI)
                BT_STAT_INC(tx_cmp_delayed);
            else if (flags & BLE_STAT_IRQ)
                BT_STAT_INC(rx_cmp_delayed);
        }
        am_util_delay_us(5);
        if (trycnt++ < 4) {
            ret = BLEIFn(BLE_DEVNO)->BSTATUS & flags;
            break;
        }
    }
    return ret;
}

static void rx_completed(uint8_t *data, uint32_t len, void *ctx)
{
    struct hci_packet *hpkt = ctx;
    ARG_UNUSED(data);
    DRV_ACK_INC(rx);
    if (likely(len > 0)) {
        hpkt->len = len;
        k_fifo_put(&rx_queue, hpkt);
    }

    if (!hci_assert_status(BLE_STAT_IRQ))
        bt_start_xmit();
}

static void tx_completed(uint8_t *data, uint32_t len, void *ctx)
{
    struct net_buf *buf = ctx;
    ARG_UNUSED(data);
    ARG_UNUSED(len);

    tx_buf_pending = buf->frags;
    buf->frags = NULL;
    net_buf_unref(buf);
    DRV_ACK_INC(tx);
    if (hci_assert_status(BLE_STAT_SPI)) {
        BT_ERR("BLE controller SPI status error\n");
    }
}

static int bt_start_rx(void)
{
    struct hci_packet *hpkt = NULL;
    int err;

    err = k_mem_slab_alloc(&rx_pool, (void **)&hpkt, K_NO_WAIT);
    if (likely(!err)) {
        err = am_hal_ble_nonblocking_hci_read(ble_handle, 
            hpkt->buf32, rx_completed, hpkt);
        if (unlikely(err)) {
            BT_STAT_INC(err_rx);
            k_mem_slab_free(&rx_pool, (void **)&hpkt);
            BT_ERR("%s(): hci receive data failed with error 0x%x\n", __func__, err);
            dump_hci_xmit_infomation();
        } else {
            DRV_REQ_INC(rx);
            BT_STAT_INC(rxcnt);
        }
    } else {
        BT_STAT_INC(err_rx_nomem);
    }
    return err;
}

static inline int bt_start_tx_queue(sys_slist_t *txq)
{
    struct net_buf *buf;
    int err;

    if (unlikely(tx_buf_pending)) {
        buf = (struct net_buf *)tx_buf_pending;
        tx_buf_pending = NULL;
    } else {
        buf = net_buf_slist_get(txq);
    }
    if (buf) {
        err = am_hal_ble_nonblocking_hci_write(ble_handle, AM_HAL_BLE_RAW,
            (uint32_t *)buf->data, buf->len, tx_completed, buf);
        if (likely(!err)) {
            DRV_REQ_INC(tx);
            BT_STAT_INC(txcnt);
        } else {
            tx_buf_pending = buf;
            BT_STAT_INC(err_tx);
            BT_ERR("%s(): hci send data failed with error 0x%x\n", __func__, err);
            dump_hci_xmit_infomation();
        }
    }
    return err;
}

static void bt_apollo3p_isr(const void *param)
{
    uint32_t intmask = BLEIFn(BLE_DEVNO)->INTEN;
    uint32_t status = BLEIFn(BLE_DEVNO)->INTSTAT & intmask;

    BLEIFn(0)->INTCLR = status;
    am_hal_ble_int_service(ble_handle, status);

    /* 
    * If this was a BLEIRQ interrupt, attempt to start a read operation. If it
    * was a STATUS interrupt, start a write operation.
    */
    if (status & AM_HAL_BLE_INT_BLECIRQ) {
        BT_STAT_INC(rx_interrupt);
        am_hal_ble_wakeup_set(ble_handle, 0);
        bt_start_rx();
    } else if (status & AM_HAL_BLE_INT_BLECSSTAT) {
        BT_STAT_INC(tx_interrupt);
        bt_start_tx_queue(&tx_queue);
    }
}

static int bt_apollo3p_start_radio(void)
{
    am_hal_ble_config_t cfg;
    uint32_t intmask;
    int trycnt = 0;
    int err;

_retry:
    err = am_hal_ble_initialize(BLE_DEVNO, &ble_handle);
    if (err)
        goto _exit;

    err = am_hal_ble_power_control(ble_handle, AM_HAL_BLE_POWER_ACTIVE);
    if (err)
        goto _deinit;

    /* Configure the HCI interface clock for 6 MHz */
    cfg.ui32SpiClkCfg = AM_HAL_BLE_HCI_CLK_DIV8;
    
    /* Set HCI read and write thresholds to 32 bytes each */
    cfg.ui32ReadThreshold = 32;
    cfg.ui32WriteThreshold = 32;

    /* The MCU will supply the clock to the BLE core */
    cfg.ui32BleClockConfig = AM_HAL_BLE_CORE_MCU_CLK;

    /* Apply the default patches when am_hal_ble_boot() is called */
    cfg.bUseDefaultPatches = true;
    err = am_hal_ble_config(ble_handle, &cfg);
    if (err) {
        BT_ERR("Configure BLE failed with error %d\n", err);
        goto _dis_pwr;
    }

    if (radio_cold_boot)
        k_msleep(1000);

    /* Attempt to boot the radio */
    err = am_hal_ble_boot(ble_handle);
    if (!err)
        goto _next;
    if (err == AM_HAL_BLE_32K_CLOCK_UNSTABLE) {
        am_hal_ble_power_control(ble_handle, AM_HAL_BLE_POWER_OFF);
        am_hal_ble_deinitialize(ble_handle);
        k_msleep(1000);
        if (++trycnt < 10) {
            BT_ERR("BLE clock source is unstable and attempt reboot ble\n");
            goto _retry;
        }
    }

    BT_ERR("Attempt boot ble failed\n");
    goto _dis_pwr;

_next:
    /* Setting 32M trim value, this value is got from actual debug */
    am_util_ble_crystal_trim_set(ble_handle, 600);
        
    /* Set the BLE TX Output power to 0dBm. */
    am_hal_ble_tx_power_set(ble_handle, CONFIG_BLE_TX_OUTPUT_POWER);

    irq_disable(BLE_IRQn);
    intmask = BLE_INTR_MASK;
    if (APOLLO3_GE_B0)
        intmask |= (AM_HAL_BLE_INT_BLECIRQN | AM_HAL_BLE_INT_BLECSSTATN);
    am_hal_ble_int_clear(ble_handle, intmask);
    am_hal_ble_int_enable(ble_handle, intmask);
    irq_connect_dynamic(BLE_IRQn, 0, bt_apollo3p_isr, NULL, 0);
    irq_enable(BLE_IRQn);
    radio_cold_boot = false;
    return 0;

_dis_pwr:
    am_hal_ble_power_control(ble_handle, AM_HAL_BLE_POWER_OFF);
_deinit:
    am_hal_ble_deinitialize(ble_handle);
_exit:
    return err;
}

static void __unused bt_stop_radio(void)
{
    irq_disable(BLE_IRQn);
    am_hal_ble_power_control(ble_handle, AM_HAL_BLE_POWER_OFF);
    while ( PWRCTRL->DEVPWREN_b.PWRBLEL );
    PWRCTRL->SUPPLYSRC_b.BLEBUCKEN = 0;
    am_hal_ble_deinitialize(ble_handle);
}

static void tryfix_event(uint8_t *pkghdr)
{
    uint16_t opcode = ((uint16_t)pkghdr[4] << 8) | pkghdr[3];
    if (unlikely(opcode == BT_HCI_OP_READ_LOCAL_FEATURES)) {
        /*
         * @bit_5: Not support EDR
         * @bit_6: Support LE
         */
        pkghdr[10] |= BIT(5) | BIT(6);
    }
}

static void bt_drv_rx_thread(void)
{
    struct bt_hci_acl_hdr acl_hdr;
    struct hci_packet *hpkt;
    struct net_buf *buf;

    for ( ; ; ) {
        hpkt = k_fifo_get(&rx_queue, K_FOREVER);
        switch (hpkt->buf[PACKET_TYPE]) {
        case HCI_EVT:
            switch (hpkt->buf[EVT_HEADER_EVENT]) {
            case BT_HCI_EVT_VENDOR:
                /* Vendor events are currently unsupported */
                BT_ERR("Unknown evtcode type 0x%02x", hpkt->buf[EVT_HEADER_EVENT]);
                goto _free;
            default:
                buf = bt_buf_get_evt(hpkt->buf[EVT_HEADER_EVENT], 
                    false, K_FOREVER);
                break;
            }

            tryfix_event(&hpkt->buf[1]);
            net_buf_add_mem(buf, &hpkt->buf[1], hpkt->buf[EVT_HEADER_SIZE] + 2);
            break;
        case HCI_ACL:
            buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
            memcpy(&acl_hdr, &hpkt->buf[1], sizeof(acl_hdr));
            net_buf_add_mem(buf, &acl_hdr, sizeof(acl_hdr));
            net_buf_add_mem(buf, &hpkt->buf[5], sys_le16_to_cpu(acl_hdr.len));
            break;
        default:
            BT_STAT_INC(err_rx_packet);
            BT_ERR("Unknown BT buf type %d", hpkt->buf[PACKET_TYPE]);
            goto _free;
        }

#ifdef CONFIG_BT_HCI_DEBUG
        dump_hci_buffer(buf, false);
#endif
        bt_recv(buf);
    _free:
        k_mem_slab_free(&rx_pool, (void **)&hpkt);
    }
}

static int bt_apollo3p_send(struct net_buf *buf)
{
    int ret = 0;
    k_sem_take(&bt_sem, K_FOREVER);
    switch (bt_buf_get_type(buf)) {
    case BT_BUF_ACL_OUT:
        net_buf_push_u8(buf, HCI_ACL);
        break;
    case BT_BUF_CMD:
        net_buf_push_u8(buf, HCI_CMD);
        break;
    default:
        BT_STAT_INC(err_tx_packet);
        BT_ERR("Unsupported type");
        k_sem_give(&bt_sem);
        return -EINVAL;
    }

#ifdef CONFIG_BT_HCI_DEBUG
    dump_hci_buffer(buf, true);
#endif
    net_buf_slist_put(&tx_queue, buf);
    bt_start_xmit();
    k_sem_give(&bt_sem);
    return ret;
}

#ifdef CONFIG_HCI_SET_MAC_ADDR
static int bt_apollo3p_set_addr(void)
{
    uint8_t mac_addr[6] = {0x01, 0x02, 0x03};
    am_hal_mcuctrl_device_t devinfo;
    struct net_buf *buf;
    
    am_hal_mcuctrl_info_get(AM_HAL_MCUCTRL_INFO_DEVICEID, &devinfo);
    mac_addr[3] = devinfo.ui32ChipID0;
    mac_addr[4] = devinfo.ui32ChipID0 >> 8;
    mac_addr[5] = devinfo.ui32ChipID0 >> 16;
    buf = bt_hci_cmd_create(0xFC32, 6);
    net_buf_add_mem(buf, mac_addr, 6);    
    return bt_hci_cmd_send_sync(0xFC32, buf, NULL);
}
#endif

static int bt_apollo3p_open(void)
{
    k_tid_t thr;
    int err;
    
    err = bt_apollo3p_start_radio();
    if (err)
        return err;

    /* Start RX thread */
    thr = k_thread_create(&rx_thread_pcb, rx_thread_stack,
            K_KERNEL_STACK_SIZEOF(rx_thread_stack),
            (k_thread_entry_t)bt_drv_rx_thread, NULL, NULL, NULL,
            K_PRIO_COOP(CONFIG_BT_DRIVER_RX_PRIO),
            0, K_NO_WAIT);
    k_thread_name_set(thr, "/bt@hci-driver");

#ifdef CONFIG_HCI_SET_MAC_ADDR
    err = bt_hci_cmd_send_sync(BT_HCI_OP_RESET, NULL, NULL);
    if (err)
        return err;
    err = bt_apollo3p_set_addr();
#endif
	return err;
}

static const struct bt_hci_driver drv = {
    .name           = "BT VIR",
    .bus            = BT_HCI_DRIVER_BUS_VIRTUAL,
#ifdef CONFIG_HCI_SET_MAC_ADDR
    .quirks         = BT_QUIRK_NO_RESET,
#else
    .quirks         = 0,
#endif
    .open           = bt_apollo3p_open,
    .send           = bt_apollo3p_send,
};

static int bt_apollo3p_init(const struct device *unused)
{
    ARG_UNUSED(unused);
    radio_cold_boot = true;
    ble_handle = NULL;
    sys_slist_init(&tx_queue);
    return bt_hci_driver_register(&drv);
}

#if defined(CONFIG_BT_HCI_STAT) && defined(CONFIG_SHELL)
static int bt_hci_driver_info(const struct shell *shell,
			      size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
    shell_fprintf(shell, SHELL_NORMAL, 
                    "HCI interface statistics infomation:\n");
    shell_fprintf(shell, SHELL_NORMAL,
                    "RX errors:                %u\n"
                    "RX buffer errors:         %u\n"
                    "TX errors:                %u\n"
                    "RX packet type errors:    %u\n"
                    "TX Packet type errors:    %u\n"
                    "TX interrupt counts:      %u\n"
                    "RX interrupt counts:      %u\n"
                    "TX packets:               %u\n"
                    "RX packets:               %u\n",
                    bt_stat.err_rx,
                    bt_stat.err_rx_nomem,
                    bt_stat.err_tx,
                    bt_stat.err_rx_packet,
                    bt_stat.err_tx_packet,
                    bt_stat.tx_interrupt,
                    bt_stat.rx_interrupt,
                    bt_stat.txcnt,
                    bt_stat.rxcnt);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_command,
    SHELL_CMD(stat, NULL, "HCI driver statistics information", bt_hci_driver_info),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(hci, &sub_command, "HCI driver commands", NULL);
#endif /* CONFIG_BT_HCI_STAT */

SYS_INIT(bt_apollo3p_init, 
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
   
