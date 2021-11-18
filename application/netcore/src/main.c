#include "ipc/rpmsg_hci.h"

int main(void) {
#ifdef COFNIG_RPMSG_HCI_RX_API
    bt_rpmsg_hci_rx_loop();
#endif
    return 0;
}