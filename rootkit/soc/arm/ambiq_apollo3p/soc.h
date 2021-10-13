#ifndef _APOLLO3P_SOC_H_
#define _APOLLO3P_SOC_H_

#include <sys/util.h>
#include <section_tags.h>

#ifndef _ASMLANGUAGE

#include <apollo3p.h>
#include <am_mcu_apollo.h>
#include <am_util_delay.h>

#if defined(CONFIG_BT) || defined(CONFIG_BLE_COMPONENT)
#include <am_util_ble.h>
#endif

/* Add include for DTS generated information */
#include <devicetree.h>


#define pin2gpio(n) ((n) & 31)
#define pin2name(n) ({ \
        const char *__io_name[] = { \
            "GPIO_0", "GPIO_1", "GPIO_2" \
        }; \
        __io_name[n >> 5]; \
    })


int soc_get_pin(const char *name);

#endif /* !_ASMLANGUAGE */

#endif /* _STM32F4_SOC_H_ */
