#ifndef BT_DISC_H_
#define BT_DISC_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C"{
#endif

struct bt_conn;
struct bt_disc_node {
    struct bt_disc_node *next;
    int (*discover)(struct bt_conn *conn);
    char *name;
    bool found;
};

void bt_discovery_reinit(void);
int  bt_discovery_start(struct bt_conn *conn);
int  bt_discovery_finish(struct bt_conn *conn, bool found);

void bt_gattp_discovery_not_found(struct bt_conn *conn, void *ctx) ;
void bt_gattp_discovery_error_found(struct bt_conn *conn, 
	int err, void *ctx);
    
int bt_gattp_discovery_start(struct bt_conn *conn);
int bt_gattp_discovery_register(struct bt_disc_node *node);
#ifdef __cplusplus
}
#endif
#endif /* BT_DISC_H_ */

