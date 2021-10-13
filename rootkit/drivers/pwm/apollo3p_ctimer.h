#ifndef DRIVER_APOLLO3P_CTIMER_H_
#define DRIVER_APOLLO3P_CTIMER_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*ctimer_fn_t)(void *arg, int offset);

struct ctimer_irqdesc {
    ctimer_fn_t isr;
    void *arg;
};


int ctimer_register_isr(int index, ctimer_fn_t isr, void *arg);
int ctimer_unregister_isr(int index);

#ifdef __cplusplus
}
#endif
#endif /* DRIVER_APOLLO3P_CTIMER_H_ */