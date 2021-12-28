/*
 * CopyRight(C) 2021-12
 * Author: wtcat
 */
#ifndef ARM_CORE_DEBUG_H_
#define ARM_CORE_DEBUG_H_

#include <errno.h>
#include <stdint.h>

#include <arch/arm/aarch32/cortex_m/cmsis.h>

#ifdef __cplusplus
extern "C"{
#endif

/*
 * User configure options
 */
#define MAX_BREAKPOINT_NR 8


struct _cpu_exception;
typedef void (*cpu_debug_handler_t)(const struct _cpu_exception *info, 
	uint32_t sp, const char *file , int line, int is_thread_mode);

struct _cpu_exception {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t psr;
};

/* 
 * Watchpoint mode 
 */
#define MEM_READ  0x0100  /* */
#define MEM_WRITE 0x0200
#define MEM_RW_MASK (MEM_READ | MEM_WRITE)
	
#define MEM_SIZE_1 0  /* 1 Byte */
#define MEM_SIZE_2 1  /* 2 Bytes */
#define MEM_SIZE_4 2  /* 4 Bytes */
#define MEM_SIZE_MASK 0xF

/*
 * Public interface
 */
#define core_watchpoint_install(_nr, _addr, _mode) \
	_core_debug_watchpoint_enable(_nr, (void *)_addr, _mode, __FILE__, __LINE__)
	
#define core_watchpoint_uninstall(_nr) \
	_core_debug_watchpoint_disable(_nr)

#if !defined(__ZEPHYR__)
#define core_watchpoint_init(_cb) \
	_core_debug_setup((cpu_debug_handler_t)_cb)
#endif

/*
 * Protected interface
 */
int _core_debug_watchpoint_enable(int nr, void *addr, unsigned int mode, 
	const char *file, int line);
int _core_debug_watchpoint_disable(int nr);
int _core_debug_watchpoint_busy(int nr);

#if !defined(__ZEPHYR__)
int _core_debug_setup(cpu_debug_handler_t cb);
void _core_debug_exception_handler(uintptr_t msp, uintptr_t psp, 
	uintptr_t exec_return);
#endif

/*
 * Private interface
 */
static inline int _core_debug_init_cycle_counter(void) {
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	return (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) != 0;
}

static inline uint32_t _core_debug_get_cycles(void) {
	return DWT->CYCCNT;
}

static inline void _core_debug_cycle_count_start(void) {
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

#ifdef __cplusplus
}
#endif
#endif /* ARM_CORTEXM_DEBUG_H_ */

