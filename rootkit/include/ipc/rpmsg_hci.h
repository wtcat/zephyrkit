#ifndef IPC_RPMSG_HCI_H_
#define IPC_RPMSG_HCI_H_

#ifdef __cplusplus
extern "C"{
#endif

#ifdef CONFIG_RPMSG_HCI_RX_API
void bt_rpmsg_hci_rx_loop(void);
#endif
#ifdef __cplusplus
}
#endif
#endif /* IPC_RPMSG_HCI_H_ */