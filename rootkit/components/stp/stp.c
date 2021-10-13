#include <toolchain.h>
#include <errno.h>
#include <sys/__assert.h>

#include "stp/stp.h"

#define STP_PROTO_MAX 15

static int stp_null_input(struct net_buf *buf)
{
    ARG_UNUSED(buf);
    return -EINVAL;
}

static int stp_null_output(struct stp_chan *chan, 
    struct stp_bufv *v, size_t n)
{
    ARG_UNUSED(chan);
    ARG_UNUSED(v);
    ARG_UNUSED(n);
    return -EINVAL;
}

static const struct stp_proto null_proto = {
    .type = 0xFF,
    .input = stp_null_input,
    .output = stp_null_output
};


static const struct stp_proto *proto_slot[STP_PROTO_MAX] = {
    [0 ... STP_PROTO_MAX-1] = &null_proto
};


const struct stp_proto *lookup_protocol(uint16_t type)
{
    if (type <  STP_PROTO_MAX)
        return proto_slot[type];
    return NULL;
}

int stp_proto_register(const struct stp_proto *proto)
{
    if (!proto)
        return -EINVAL;
    
    if (proto->type > STP_PROTO_MAX-1)
        return -EINVAL;

    if (!proto->input)
        return -EINVAL;

    if (!proto->output)
        return -EINVAL;
    
    proto_slot[proto->type] = proto;
    return 0;
}

int stp_sendmsgs(struct stp_chan *chan, struct stp_bufv *v, size_t n)
{
    const struct stp_proto *proto;
    __ASSERT(chan != NULL, "invalid input parameter");
    
    if (!v)
        return -EINVAL;
    if (!n)
        return -EINVAL;
    
    proto = lookup_protocol(chan->type);
    if (proto)
        return proto->output(chan, v, n);
    return -EINVAL;    
}

