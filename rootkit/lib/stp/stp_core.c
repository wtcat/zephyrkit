#include <assert.h>
#include <zephyr.h>
#include <init.h>
#include <net/buf.h>
#include <sys/crc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(stp_core);

#include "stp/stp_core.h"
#include "stp/stp.h"


/*
 * STX + STX_HEADER + CRC16 + ETX
 */
#define STP_MIN_LEN   (2 + sizeof(struct stp_header) + 2 + 2)

#define STP_TICK_MS    1000 
#define STP_MAX_RETRY  3


struct stp_rtn {
    sys_dnode_t node;
    struct net_buf_simple clone;
    struct stp_dev *dev;
    struct net_buf *p;
    int expire;
    uint8_t retry;
    uint8_t need_ack;
};

struct stp_dev {
    const struct stp_driver *drv;
    struct k_fifo rx_queue;
    struct k_fifo tx_queue;
    sys_dlist_t match_list;
    uint16_t req_seqno;
    uint32_t respond_timeout;
    unsigned long no_ack_count;
    unsigned long err_ack_count;
};

enum evt_tag {
    EVENT_RX,
    EVENT_TX
};

#define rtn_buffer(buf) (&stp_rtn_pool[net_buf_id(buf)])

static struct stp_rtn stp_rtn_pool[STP_MAX_TXBUF];
static K_THREAD_STACK_DEFINE(stp_thread_stack, 4096);
static struct k_thread stp_thread_data;
static struct stp_dev stp_device = {
    .rx_queue = Z_FIFO_INITIALIZER(stp_device.rx_queue),
    .tx_queue = Z_FIFO_INITIALIZER(stp_device.tx_queue),
    .match_list = SYS_DLIST_STATIC_INIT(&stp_device.match_list)
};

BUILD_ASSERT(CONFIG_NET_BUF_USER_DATA_SIZE >= 4, "");


static inline uint16_t stp_crc16(const uint8_t *src, size_t len)
{
    return crc16_itu_t(0, src, len);
}

static void remove_rtn(struct stp_rtn *meta)
{
    sys_dlist_remove(&meta->node);
    net_buf_unref(meta->p);
    meta->p = NULL;
}

static struct stp_rtn *add_rtn(struct stp_dev *dev, 
    struct net_buf *buf)
{
    struct stp_rtn *rtn = rtn_buffer(buf);
    net_buf_ref(buf);
    net_buf_simple_clone(&buf->b, &rtn->clone);
    rtn->p = buf;
    rtn->retry = STP_MAX_RETRY;
    rtn->expire = STP_TICK_MS;
    sys_dlist_append(&dev->match_list, &rtn->node);
    return rtn;
}

static inline int stp_send(struct stp_dev *dev, struct net_buf *buf)
{
    int ret;
    ret = dev->drv->output(dev->drv, buf);
    if (ret) {
        LOG_ERR("%s(): send failed with error %d\n", __func__, ret);
    }
    return ret;
}

static k_timeout_t run_rto_timer(struct stp_dev *dev, int elapsed)
{
    struct stp_rtn *curr, *next;
    int32_t next_expired = INT32_MAX;
    
    SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&dev->match_list, curr, next, node) {
        int32_t remain = curr->expire - elapsed;
        if (remain <= 0) {
            if (curr->retry > 0) {
                struct net_buf *buf = curr->p;
                if (buf->ref == 1) {
                    LOG_INF("Retransmit packet %d\n", 
                        ((struct stp_header *)buf->data)->seqno);
                    net_buf_simple_clone(&curr->clone, &buf->b);
                    net_buf_ref(buf);
                    stp_send(dev, buf);
                }
                if (--curr->retry == 0) {
                    remove_rtn(curr);
                    dev->no_ack_count++;
                    continue;
                }

                curr->expire = STP_TICK_MS;
            }
        } else  {
            curr->expire = remain;
        }
        next_expired = MIN(next_expired, curr->expire);
    } 
    
    return (next_expired == INT32_MAX)? 
            K_FOREVER: K_MSEC(next_expired);
}

static bool list_match(struct stp_dev *dev, struct stp_header *hdr)
{
    struct stp_rtn *curr;
    SYS_DLIST_FOR_EACH_CONTAINER(&dev->match_list, curr, node) {
        struct stp_header *list_hdr = (struct stp_header *)curr->p->data;
        if (list_hdr->seqno == hdr->seqno &&
            list_hdr->version == hdr->version) {
            remove_rtn(curr);
            return true;
        }
    }
    return false;
}

static void events_init(struct k_poll_event **ev, int *num)
{
    static struct k_poll_event events[] = {
        K_POLL_EVENT_STATIC_INITIALIZER(
            K_POLL_TYPE_FIFO_DATA_AVAILABLE,
            K_POLL_MODE_NOTIFY_ONLY,
            &stp_device.rx_queue,
            EVENT_RX),
        K_POLL_EVENT_STATIC_INITIALIZER(
            K_POLL_TYPE_FIFO_DATA_AVAILABLE,
            K_POLL_MODE_NOTIFY_ONLY,
            &stp_device.tx_queue,
            EVENT_TX),
    };

    *ev = events;
    *num = ARRAY_SIZE(events);
}

static int stp_l2_input(struct stp_dev *dev, struct net_buf *buf)
{
    struct stpbuf_attr *attr;
    const struct stp_proto *proto;
    struct stp_header *hdr;

    hdr = net_buf_pull_mem(buf, sizeof(struct stp_header));
    if (!hdr->request) {
        if (!list_match(dev, hdr)) {
            dev->err_ack_count++;
            LOG_WRN("%s(): ACK not matched\n", __func__);
            return -EINVAL;
        }
    }

    proto = lookup_protocol(hdr->version);
    if (!proto) {
        net_buf_unref(buf);
        return -EINVAL;
    }

    attr = net_buf_user_data(buf);
    attr->require_ack = hdr->require_ack;
    attr->seqno = hdr->seqno;
    return proto->input(buf);
}

static int process_events(struct stp_dev *dev, 
    struct k_poll_event *ev, int n)
{
    int ret = 0;
    for ( ; n > 0; ev++, n--) {
        if (ev->state == K_POLL_STATE_FIFO_DATA_AVAILABLE) {
            struct net_buf *buf = net_buf_get(ev->fifo, K_NO_WAIT);
            if (!buf) {
                ev->state = K_POLL_STATE_NOT_READY;
                LOG_INF("Receive queue is empty!");
                ret = -EINVAL;
                goto _exit;
            }
            if (ev->tag == EVENT_RX) {
                ret = stp_l2_input(dev, buf);
            } else {
                struct stpbuf_attr *attr = net_buf_user_data(buf);
                if (attr->require_ack)
                    add_rtn(dev, buf);
                ret = stp_send(dev, buf);
                if (ret) {

                }
            }
        }
        ev->state = K_POLL_STATE_NOT_READY;
    }

_exit:
    return ret;
}

static void stp_thread(void *arg)
{
    k_timeout_t next_expire = K_MSEC(STP_TICK_MS);
    struct stp_dev *dev = arg;
    struct k_poll_event *events;
    int64_t last;
    int nevents;
    int delta;

    last = k_uptime_ticks();
    events_init(&events, &nevents);
    assert(events != NULL);
    assert(nevents > 0);

    for ( ; ; ) {
        int ret = k_poll(events, nevents, next_expire);
        if (ret) {
            if (ret == -EAGAIN)
                goto _run_timer;
            else
                continue;
        }

        process_events(dev, events, nevents);
_run_timer:
        delta = k_uptime_delta(&last);
        next_expire = run_rto_timer(dev, (int)delta);
    }
}

int stp_output(struct net_buf *buf, unsigned int flags)
{
    struct stpbuf_attr *attr = net_buf_user_data(buf);
    struct stp_dev *dev = &stp_device;
    struct stp_header *hdr;
    uint16_t crc;
    uint16_t etx;
    uint16_t len;

    hdr = net_buf_push(buf, sizeof(struct stp_header));
    hdr->stx = ltons(STP_STX);
    hdr->request = attr->request;
    hdr->require_ack = attr->require_ack;
    hdr->upload = 1;
    hdr->reserved = 0;
    hdr->version = attr->type & 0xF;
    if (flags & STP_RESP) {
        hdr->seqno = attr->seqno;
    } else {
        hdr->seqno = ltons(dev->req_seqno);
        dev->req_seqno++;
    }
    len = buf->len + sizeof(crc) + sizeof(etx);
    hdr->length = ltons(len);
    crc = stp_crc16(&buf->data[2], buf->len - 2);
    crc = ltons(crc);
    etx = ltons(STP_ETX);
    net_buf_add_mem(buf, &crc, sizeof(crc));
    net_buf_add_mem(buf, &etx, sizeof(etx));
    net_buf_put(&dev->tx_queue, buf);
    return 0;
}

int stp_driver_input(struct net_buf *buf)
{
    struct stp_dev *dev = &stp_device;
    struct stp_header *hdr;
    uint16_t length;
    uint16_t crc;

    assert(buf != NULL);
    hdr = (struct stp_header *)buf->data;
    length = ntols(hdr->length);
    if (length < STP_MIN_LEN)
        return -EINVAL;
    
    if (ntols(hdr->stx) != STP_STX || 
        btos(&buf->data[length - 2]) != STP_ETX)
        return -EINVAL;
    
    crc = btos(&buf->data[length - 4]);
    if (stp_crc16(&buf->data[2], length-6) != crc)
        return -EINVAL;

    if (hdr->upload)
        return -EINVAL;

    /* Remove CRC and tail */
    buf->len -= (2 + 2);
    net_buf_put(&dev->rx_queue, buf);
    return 0;
}

int stp_driver_register(const struct stp_driver *drv)
{
    if (!drv)
        return -EINVAL;
    
    if (stp_device.drv)
        return -EBUSY;

    stp_device.drv = drv;
    return 0;
}

static int stp_init(const struct device *dev_)
{
    struct stp_dev *dev = &stp_device;
    k_tid_t thr;

    ARG_UNUSED(dev_);
    if (!dev->drv)
        return -ENODEV;

    if (dev->drv->setup(dev->drv))
        return -EIO;
    
    thr = k_thread_create(&stp_thread_data, stp_thread_stack, 
            K_THREAD_STACK_SIZEOF(stp_thread_stack),
            (k_thread_entry_t)stp_thread, dev, NULL, NULL,
            8,
            0,
            K_FOREVER);
    k_thread_name_set(thr, "/protocol@stp");
    k_thread_start(thr);
    return 0;
}

SYS_INIT(stp_init, APPLICATION, 55);
