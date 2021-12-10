#include <stdint.h>
#include <bluetooth/controller.h>
#include "ipc/rpmsg_hci.h"

int main(void) {
    uint8_t mac[] = {0xa0, 0xb1, 0xc2, 0x01, 0x02, 0x03};
    bt_ctlr_set_public_addr(mac);
#ifdef COFNIG_RPMSG_HCI_RX_API
    bt_rpmsg_hci_rx_loop();
#endif
    return 0;
}