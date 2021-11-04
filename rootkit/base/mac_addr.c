#include <bluetooth/bluetooth.h>
#include <errno.h>

#include "base/mac_addr.h"

ssize_t get_mac_address(uint8_t *addr)
{
    bt_addr_le_t le_addr;
    size_t count;
	
    if (!addr)
        return -EINVAL;
    count = 1;
    bt_id_get(&le_addr, &count);
    if (count > 0 && le_addr.type == BT_ADDR_LE_PUBLIC) {
        memcpy(addr, le_addr.a.val, sizeof(bt_addr_le_t));
        return sizeof(bt_addr_le_t);
    }
    return -EINVAL;
}

