#ifndef __BOOT_EVENT_H__
#define __BOOT_EVENT_H__


#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#include <kernel.h>


/***************************************************************************
    msg[0] : status   * 0: boot start * 1: updating status * 3. update error
    msg[1] : para     *   error no    *  percent para      *    error no
 ****************************************************************************/
struct boot_event{
    uint8_t msg[4];
};

#define EVENT_STATUS_BOOT             0
#define EVENT_STATUS_UPDATING         1
#define EVENT_STATUS_UPDATE_ERROR     2

//boot error 
#define EVENT_BOOT_ERRNO_IMG_NOT_VALID 0
#define EVENT_BOOT_ERRNO_RESUME_FAIL   1
#define EVENT_BOOT_ERRNO_NONE          0xff
//update error
#define EVENT_ERRNO_UPDATE_ERROR_WRITE  0
#define EVENT_ERRNO_UPDATE_ERROR_IMG_NOT_VALID 1


int boot_event_event_pop(struct boot_event *put_event, bool wait);
int boot_event_event_push(struct boot_event *put_event);
void send_event_to_display(uint8_t evt_type, uint8_t evt_para);

#ifdef __cplusplus
}
#endif

#endif /* __BOOT_EVENT_H__ */