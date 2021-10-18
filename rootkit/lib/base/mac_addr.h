#ifndef BASE_MAC_ADDR_H_
#define BASE_MAC_ADDR_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C"{
#endif

/*
 * @return 6 is success, less than 0 is failed
 */
ssize_t get_mac_address(uint8_t addr[]);

#ifdef __cplusplus
}
#endif
#endif /* BASE_MAC_ADDR_H_ */