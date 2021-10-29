#include "ipc/rpmsg_hci.h"

int main(void) {
    bt_rpmsg_hci_rx_loop();
    return 0;
}