#ifndef BSP_GPIO_KEYS_H_
#define BSP_GPIO_KEYS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

struct key_value {
    uint16_t value;
    uint16_t code;
};

/* Key action */
#define KEY_ACTION_PRESSED    0x01
#define KEY_ACTION_RELEASE    0x02
#define KEY_ACTION_CLICK      0x03
#define KEY_ACTION_LONG_PRESS 0x04

void flush_gpio_keys(void);
int  dequeue_gpio_key(struct key_value *val);
void iterate_gpio_key(void (*iter)(struct key_value *, void *), 
    void *arg);
int  register_keydev_notify(void (*notify)(void *context), 
    void *context);

#ifdef __cplusplus
}
#endif
#endif /* BSP_GPIO_KEYS_H_ */

