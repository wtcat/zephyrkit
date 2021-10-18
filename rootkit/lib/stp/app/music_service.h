#ifndef STP_APP_MUSIC_SERVICE_H_
#define STP_APP_MUSIC_SERVICE_H_

#include "base/observer_class.h"
#ifdef CONFIG_PROTOBUF
#include "stp/proto/music.pb-c.h"
#endif

#ifdef __cplusplus
extern "C"{
#endif

/*
 * method:
 *  int remind_notify(unsigned long action, void *ptr)
 *  int remind_add_observer(struct observer_base *obs)
 *  int remind_remove_observer(struct observer_base *obs)
 */
OBSERVER_CLASS_DECLARE(music)


/*
 * Minor command list
 */
#define MUSIC_REQ_PREV  0x02
#define MUSIC_REQ_NEXT  0x03
#define MUSIC_REQ_PLAY  0x04
#define MUSIC_REQ_PAUSE 0x05


/*
 * Remote music control
 */ 
int remote_music_player_request(uint32_t req);
int remote_music_volume_sync_request(int volume);

#ifdef __cplusplus
}
#endif
#endif /* STP_APP_MUSIC_SERVICE_H_ */
