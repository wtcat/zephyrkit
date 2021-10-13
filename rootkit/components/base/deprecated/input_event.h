#ifndef BASE_INPUT_EVENT_H_
#define BASE_INPUT_EVENT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

struct input_struct;
struct input_event;

typedef void (*input_report_fn)(struct input_struct *idev, 
    struct input_event *evt, uint32_t event);
/*
 * Input event type
 */
#define INPUT_EVENT_TP   0x0001
#define INPUT_EVENT_KEY 0x0002

struct tp_event {
    uint16_t x;
    uint16_t y;
    int state;
#define INPUT_STATE_RELEASED 0
#define INPUT_STATE_PRESSED   1
};

struct key_event {
    uint16_t code;
    uint16_t value;
#define INPUT_KEY_PRESSED       0x01
#define INPUT_KEY_RELEASE       0x02
#define INPUT_KEY_CLICK         0x03
#define INPUT_KEY_PRESSED_LONG 0x04
};

struct input_event {
    union {
        struct tp_event tp;
        struct key_event key;
    };
};

struct input_report {
    struct input_report *next;
    void (*report)(struct input_event *, uint32_t);
};

struct input_device {
    int (*read)(void *private, struct input_struct *input,
                input_report_fn report);
    void *private_data;
};


int input_device_register(const struct input_device *idev,
    uint32_t event);
int input_report_register(struct input_report *report);
void input_event_post(uint32_t event);

#ifdef __cplusplus
}
#endif
#endif /* BASE_INPUT_EVENT_H_ */
