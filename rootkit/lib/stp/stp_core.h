#ifndef STP_CORE_H_
#define STP_CORE_H_

#include <sys/byteorder.h>

#ifdef __cplusplus
extern "C"{
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __CPU_LITTLE_ENDIAN__ 
#else
#define __CPU_BIG_ENDIAN__
#endif


#define STP_MAX_PAYLOAD 256


/*
 * @brief: Protocol byteorder 
 *
 */
#define ltons(v) sys_cpu_to_be16(v)
#define ltonl(v) sys_cpu_to_be32(v)
#define ntols(v) sys_be16_to_cpu(v)
#define ntoll(v) sys_be32_to_cpu(v)

#define btos(buf) sys_get_be16(buf)
#define btol(buf) sys_get_be32(buf)

/*
 * Boundary mark
 */
#define STP_STX (uint16_t)0xDEEF
#define STP_ETX (uint16_t)0xBCCD

#define STP_MAX_TXBUF 8


#define STP_CRC_LEN 2
#define STP_EXT_LEN 2
#define STP_TAIL_SIZE (STP_CRC_LEN + STP_EXT_LEN)


struct net_buf;

struct stp_header {
    uint16_t stx;
#ifndef __CPU_LITTLE_ENDIAN__
    uint8_t upload:1;
    uint8_t request:1;
    uint8_t require_ack:1;
    uint8_t reserved:1;
    uint8_t version:4;
#else /* !__CPU_LITTLE_ENDIAN__ */
    uint8_t version:4;
    uint8_t reserved:1;
    uint8_t require_ack:1;
    uint8_t request:1;
    uint8_t upload:1;
#endif /* __CPU_LITTLE_ENDIAN__ */
    uint16_t length;
    uint16_t seqno;
} __packed;


struct stpbuf_attr {
    uint16_t seqno;
    uint8_t type;
    uint8_t request:1;
    uint8_t require_ack:1;
};

struct stp_driver {
    const char *name;
    int (*setup)(const struct stp_driver *drv);
    int (*output)(const struct stp_driver *drv, struct net_buf *buf);
    void *private;
};



#define STP_RESP 0x0001

int stp_output(struct net_buf *buf, unsigned int flags);

int stp_driver_register(const struct stp_driver *drv);
int stp_driver_input(struct net_buf *buf);

#ifdef __cplusplus
}
#endif
#endif /* STP_CORE_H_ */
