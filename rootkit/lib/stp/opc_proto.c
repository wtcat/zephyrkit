#include <stdlib.h>

#include <zephyr.h>
#include <net/buf.h>
#include <init.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(stp_opc);

#include "stp/opc_proto.h"
#include "stp/stp_core.h"

extern struct opc_service_node __stp_node_start[];
extern struct opc_service_node __stp_node_end[];


#define OPC_BUF_MAX STP_MAX_PAYLOAD
#define OPC_RESERVE sizeof(struct stp_header)
 
#define OPC_TXBUF_SIZE(n) (n + OPC_RESERVE)

NET_BUF_POOL_DEFINE(stp_pool, STP_MAX_TXBUF, OPC_TXBUF_SIZE(OPC_BUF_MAX), 
    sizeof(void *), NULL);
static K_MUTEX_DEFINE(tx_lock);

#ifdef CONFIG_STP_RBTREE
static bool opc_lessthan(struct rbnode *a, struct rbnode *b)
{
    struct opc_service_node *opc_l = 
        CONTAINER_OF(a, struct opc_service_node, node);
    struct opc_service_node *opc_r = 
        CONTAINER_OF(b, struct opc_service_node, node);
    return opc_l->opcode < opc_r->opcode;
}

static struct rbtree root = {
    .root = NULL,
    .lessthan_fn = opc_lessthan
};

static struct opc_service_node *opc_find_handle(struct rbtree *tree, 
    uint16_t opcode)
{
    struct rbnode *n = tree->root;
    struct opc_service_node node = {
        .opcode = opcode
    };

    while (n) {
        struct opc_service_node *handle = CONTAINER_OF(n, 
                struct opc_service_node, node);
        if (handle->opcode == opcode)
            return handle;
            
        if (tree->lessthan_fn(n, &node.node)) {
            n = n->children[1];
        } else {
            uintptr_t l = ((uintptr_t)n->children[0]) & ~1UL;
            n = (struct rbnode *)l;
        }
    }
    return NULL;
}

#else /* !CONFIG_STP_RBTREE  */

struct static_btree {
    void *start;
    size_t count;
    size_t size;
};

static struct static_btree root;

static void static_tree_init(struct static_btree *tree)
{
    uintptr_t size = (uintptr_t)__stp_node_end - 
                     (uintptr_t)__stp_node_start;
    tree->start = __stp_node_start;
    tree->size = sizeof(struct opc_service_node);
    tree->count = size / tree->size;
}

static bool static_tree_check(void)
{
    struct opc_service_node *previous = __stp_node_start;
    struct opc_service_node *current = __stp_node_start + 1;
    while (current < __stp_node_end) {
        if (current->opcode == previous->opcode) {
            LOG_ERR("%s(): protocol tree conflicted(major: %d, minor: %d)\n", 
                __func__, OPC_MAJOR(current->opcode), OPC_MINOR(current->opcode));
            return false;
        }
        previous = current;
        current++;
    }
    return true;
}

static int compare(const void *a, const void *b)
{ 
    const struct opc_service_node *left = 
        (const struct opc_service_node *)a;
    const struct opc_service_node *right = 
        (const struct opc_service_node *)b;
    return (int)left->opcode - (int)right->opcode;
}

static const struct opc_service_node *opc_find_handle(
    struct static_btree *tree, uint16_t opcode)
{
    struct opc_service_node key;
    key.opcode = opcode;
    return bsearch(&key, tree->start, tree->count, 
        tree->size, compare);
        
}
#endif /* CONFIG_STP_RBTREE */

static struct net_buf *opc_alloc_buf(struct net_buf_pool *pool,
    size_t offset)
{
    struct net_buf *buf;
    buf = net_buf_alloc(pool, K_FOREVER);
    if (buf) 
        net_buf_reserve(buf, OPC_RESERVE + offset);
    return buf;
}

static int opc_output(struct stp_chan *chan, struct stp_bufv *v, 
    size_t n)
{
    struct stpbuf_attr *attr;
    struct opc_chan *opc;
    struct net_buf *buf;
    int ret = 0;
    
    k_mutex_lock(&tx_lock, K_FOREVER);
    buf = opc_alloc_buf(&stp_pool, 0);
    if (!buf) {
        ret = -ENOMEM;
        goto _unlock;
    }

    opc = CONTAINER_OF(chan, struct opc_chan, base);
    net_buf_add_u8(buf, opc->major);
    
    for (int i = 0; i < n ;i++, v++) {
        if (!v->buf || !v->len)
            continue;

        if (v->len + STP_TAIL_SIZE > net_buf_tailroom(buf)) {
            ret = -EFBIG;
            goto _freebuf;
        }

        net_buf_add_mem(buf, v->buf, v->len);
    }
    if (buf->len > 0) {
        attr = net_buf_user_data(buf);
        attr->request = (opc->flags & OPCF_REQ)? 1: 0;
        attr->require_ack = (opc->flags & OPCF_NOACK)? 0: 1;
        attr->type = opc->base.type;
        ret = stp_output(buf, 0);
        goto _unlock;
    }

_freebuf:
    net_buf_unref(buf);
_unlock:
    k_mutex_unlock(&tx_lock);
    return ret;
}

static int opc_output_buf(struct net_buf *buf, uint8_t major)
{
    int ret;
    net_buf_push_u8(buf, major);
    k_mutex_lock(&tx_lock, K_FOREVER);
    ret = stp_output(buf, STP_RESP);
    k_mutex_unlock(&tx_lock);
    return ret;
}

static int opc_input(struct net_buf *buf)
{
    struct stpbuf_attr *attr = net_buf_user_data(buf);
    const struct opc_service_node *handle;
    struct opc_hdr *hdr;
    struct opc_subhdr *sub;
    struct net_buf *resp_buf;
    uint16_t len;
    uint16_t dlen;

    if (attr->require_ack) {
        resp_buf = opc_alloc_buf(&stp_pool, sizeof(struct opc_hdr));
        if (!buf) 
            LOG_ERR("%s(): No more memory\n", __func__);
    } else {
        resp_buf = NULL;
    }

    hdr = net_buf_pull_mem(buf, sizeof(struct opc_hdr));
    sub = net_buf_pull_mem(buf, sizeof(struct opc_subhdr));
    len = ntols(sub->len);
    dlen = OPC_LEN(len);
    while (buf->len >= dlen) {
        uint16_t opcode =   ((uint16_t)hdr->major << 8) | sub->minor;
        handle = opc_find_handle(&root, opcode);
        if (handle)
            handle->action(sub->data, len, resp_buf);
        sub = net_buf_pull(buf, dlen);
        if (!buf->len)
            break;
        len = ntols(sub->len);
        dlen = OPC_LEN(len);
    }
    net_buf_unref(buf);
    if (resp_buf && resp_buf->len) {
        struct stpbuf_attr *resp_attr = net_buf_user_data(resp_buf);
        resp_attr->require_ack = 0;
        resp_attr->request = 0;
        resp_attr->type = STP_OPC_PROTO;
        resp_attr->seqno = attr->seqno;
        return opc_output_buf(resp_buf, hdr->major);
    }
    return 0;
}

static const struct stp_proto opc_protocol = {
    .type   = STP_OPC_PROTO,
    .input  = opc_input,
    .output = opc_output,
};

#ifdef CONFIG_STP_RBTREE
static int opc_node_register(struct opc_service_node *node)
{
    if (!node)
        return -EINVAL;
    
    if (!node->action)
        return -EINVAL;

    if (rb_contains(&root, &node->node))
        return -EBUSY;
        
    rb_insert(&root, &node->node);
    return 0;
}
#endif

static int opc_init(const struct device *dev)
{    
    ARG_UNUSED(dev);
    stp_proto_register(&opc_protocol);
#ifdef CONFIG_STP_RBTREE
    for (struct opc_service_node *node = __stp_node_start;
          node < __stp_node_end;
          node++) {
        opc_node_register(node);
    }
#else
    if (!static_tree_check()) 
        return -EINVAL;
    static_tree_init(&root);
#endif /* CONFIG_STP_RBTREE */
    return 0;
}

SYS_INIT(opc_init, APPLICATION, 
    CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

