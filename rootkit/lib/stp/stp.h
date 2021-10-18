#ifndef STP_H_
#define STP_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"{
#endif


/*
 * Simple transfer protocol version
 */
#define STP_OPC_PROTO 0
#define STP_PROTO_LAST STP_OPC_PROTO


struct net_buf;

struct stp_bufv {
    void *buf;
    size_t len;
};

struct stp_chan {
    uint16_t type;
};

struct stp_proto {
    int type;
    int (*input)(struct net_buf *buf);
    int (*output)(struct stp_chan *chan, struct stp_bufv *v, size_t n);
};


const struct stp_proto *lookup_protocol(uint16_t type);
int stp_proto_register(const struct stp_proto *proto);

/*
 * @bref send data by simple transfer protocol
 *
 */
int stp_sendmsgs(struct stp_chan *chan, struct stp_bufv *v, size_t n);


#ifdef __cplusplus
}
#endif
#endif /* STP_H_ */

