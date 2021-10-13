
#include <toolchain.h>
#include <irq.h>
#include <init.h>
#include <soc.h>

#include "apollo3p_ctimer.h"


static struct ctimer_irqdesc ctimer_irqdesc_table[32];

int ctimer_register_isr(int index, ctimer_fn_t isr, void *arg)
{
    if (index > 31)
        return -EINVAL;
    if (isr == NULL)
        return -EINVAL;

    unsigned int key = irq_lock();
    ctimer_irqdesc_table[index].isr = isr;
    ctimer_irqdesc_table[index].arg = arg;
    irq_unlock(key);
    CTIMERn(0)->INTCLR = BIT(index);
    CTIMERn(0)->INTEN |= BIT(index);
    return 0;
}

int ctimer_unregister_isr(int index)
{
    if (index > 31)
        return -EINVAL;

    unsigned int key = irq_lock();
    CTIMERn(0)->INTEN &= ~BIT(index);
    ctimer_irqdesc_table[index].isr = NULL;
    ctimer_irqdesc_table[index].arg = NULL;
    irq_unlock(key);
    return 0;
}

static void ctimer_isr(const void *unused)
{
    uint32_t status = CTIMERn(0)->INTSTAT;
    struct ctimer_irqdesc *desc;

    status &= CTIMERn(0)->INTEN;
    CTIMERn(0)->INTCLR = status;
    while (status) {
        uint32_t no = 31 - __builtin_clz(status);
        status &= ~BIT(no);
        desc = &ctimer_irqdesc_table[no];
        if (desc->isr)
            desc->isr(desc->arg, no);
    }
}

static int ctimer_common_init(const struct device *dev)
{
    ARG_UNUSED(dev);
    irq_connect_dynamic(CTIMER_IRQn, 0, ctimer_isr, NULL, 0);
    irq_enable(CTIMER_IRQn);
    return 0;
}

SYS_INIT(ctimer_common_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

