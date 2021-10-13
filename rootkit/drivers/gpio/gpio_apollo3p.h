#ifndef DRIVER_APOLLO3P_GPIO_H_
#define DRIVER_APOLLO3P_GPIO_H_

#include <drivers/gpio.h>

#ifdef __cplusplus
extern "C"{
#endif

struct device;

int gpio_apollo3p_request_irq(const struct device *dev, int pin, 
    void (*handler)(int, void *), void *arg,
    enum gpio_int_trig trig, bool auto_en);

#ifdef __cplusplus
}
#endif
#endif /* DRIVER_APOLLO3P_GPIO_H_*/
