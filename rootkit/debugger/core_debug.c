/*
 * CopyRight(C) 2021-12
 * Author: wtcat
 */
#include <stddef.h>
#include <init.h>
#include <arch/cpu.h>

#include "base/fatal.h"
#include "debugger/core_debug.h"


#define _CPU_MUTEX_LOCK(_lock) \
    (_lock) = arch_irq_lock()
#define _CPU_MUTEX_UNLOCK(_lock) \
    arch_irq_unlock(_lock)

#if defined(__CORE_CM3_H_GENERIC) || \
	defined(__CORE_CM4_H_GENERIC) || \
	defined(__CORE_CM7_H_GENERIC)
struct watchpoint_regs {
	volatile uint32_t comp;
	volatile uint32_t mask;
	volatile uint32_t function;
	uint32_t reserved;
};

#elif defined(__CORE_CM33_H_GENERIC) || \
	  defined(__CORE_CM23_H_GENERIC)
struct watchpoint_regs {
	volatile uint32_t comp;
	uint32_t reserved_1;
	volatile uint32_t function;
	uint32_t reserved_2;
};

#else
#error "Unknown CPU architecture"

#endif /* __CORE_CM_H_GENERIC */

struct breakpoint_info {
	const char *file;
	int line;
};


#define WATCHPOINT_REGADDR(member) \
	(struct watchpoint_regs *)(DWT_BASE + offsetof(DWT_Type, member));
#define WATCHPOINT_MAX_NR \
	((DWT->CTRL & DWT_CTRL_NUMCOMP_Msk) >> DWT_CTRL_NUMCOMP_Pos)
#define WATCHPOINT_REG() WATCHPOINT_REGADDR(COMP0)

#if !defined(__ZEPHYR__)
static cpu_debug_handler_t _debug_cb_handler;
#endif
static int _watchpoint_nums;
static struct breakpoint_info _breakpoint[MAX_BREAKPOINT_NR];

static int core_watchpoint_match(void) {
	struct watchpoint_regs *wp_regs = WATCHPOINT_REG();
	for (int nr = 0; nr < _watchpoint_nums; nr++) {
		if (wp_regs[nr].function & DWT_FUNCTION_MATCHED_Pos)
			return nr;
	}
	return -1;
}

#if !defined(__ZEPHYR__)
void _core_debug_exception_handler(uintptr_t msp, uintptr_t psp, 
	uintptr_t exec_return) {
	if (_debug_cb_handler) {
		uintptr_t sp = (exec_return & 0x4)? msp: psp;
		struct _cpu_exception *info = (struct _cpu_exception *)sp;
		struct breakpoint_info bkpt = {NULL, 0};
		int nr = core_watchpoint_match();
		if (nr >= 0)
			bkpt = _breakpoint[nr];
		_debug_cb_handler(info, sp, bkpt.file, bkpt.line, sp == psp);
	}
}

#if defined(__GNUC__)
void __attribute__((naked)) _debug_mointor_exception(void) {
    __asm volatile (
            " mrs r0, msp       \n"
            " mrs r1, psp       \n"
            " mov r2, lr        \n"
            " b _core_debug_exception_handler \n"
    );
}

#elif defined(__CC_ARM)
void __attribute__((naked)) _debug_mointor_exception(void) {

}
#else
#error "Unknown compiler"
#endif /* __GNUC__ */
#endif /* !__ZEPHYR__ */

static void __core_debug_access(int ena) {
#if defined(__CORE_CM7_H_GENERIC)
	uint32_t lsr = DWT->LSR;
	if ((lsr & DWT_LSR_Present_Msk) != 0) {
		if (!!ena) {
			if ((lsr & DWT_LSR_Access_Msk) != 0) 
				DWT->LAR = 0xC5ACCE55; /* unlock it */
		} else {
			if ((lsr & DWT_LSR_Access_Msk) == 0) 
				DWT->LAR = 0; /* Lock it */
		}
	}
#endif
}

static int __core_debug_init(void) {
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	__core_debug_access(1);
	return 0;
}

static int __core_debug_enable_monitor_exception(void) {
	 if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
	 	return -EBUSY;
#if defined(CONFIG_ARMV8_M_SE) && !defined(CONFIG_ARM_NONSECURE_FIRMWARE)
	if (!(CoreDebug->DEMCR & DCB_DEMCR_SDME_Msk))
		return -EBUSY;
#endif
	CoreDebug->DEMCR |= CoreDebug_DEMCR_MON_EN_Msk;
	return 0;
}

int _core_debug_watchpoint_enable(int nr, void *addr, unsigned int mode, 
	const char *file, int line) {
	if (nr > _watchpoint_nums)
		return -EINVAL;
	struct watchpoint_regs *wp_regs = WATCHPOINT_REG();
	uint32_t func = (mode & 0xF) << DWT_FUNCTION_DATAVSIZE_Pos;
    uint32_t level;
	_CPU_MUTEX_LOCK(level);
	_breakpoint[nr].file = file;
	_breakpoint[nr].line = line;
#if defined(__CORE_CM3_H_GENERIC) || \
	defined(__CORE_CM4_H_GENERIC) || \
	defined(__CORE_CM7_H_GENERIC)
	if (mode & MEM_READ)
		func |= (0x6 << DWT_FUNCTION_FUNCTION_Pos);
	if (mode & MEM_WRITE)
		func |= (0x5 << DWT_FUNCTION_FUNCTION_Pos);	
	wp_regs[nr].comp = (uint32_t)addr;
	wp_regs[nr].mask = 0;
	wp_regs[nr].function = func;
	
#elif defined(__CORE_CM33_H_GENERIC) || \
	  defined(__CORE_CM23_H_GENERIC)
	if (mode & MEM_READ)
		func |= (0x5 << DWT_FUNCTION_MATCH_Pos);
	if (mode & MEM_WRITE)
		func |= (0x6 << DWT_FUNCTION_MATCH_Pos);	
	func |= (0x1 << DWT_FUNCTION_ACTION_Pos);
	wp_regs[nr].comp = (uint32_t)addr;
	wp_regs[nr].function = func;
#endif
	_CPU_MUTEX_UNLOCK(level);
    return 0;
}

int _core_debug_watchpoint_disable(int nr) {
	if (nr > _watchpoint_nums)
		return -EINVAL;
	struct watchpoint_regs *wp_regs = WATCHPOINT_REG();
    uint32_t level;
	_CPU_MUTEX_LOCK(level);
	wp_regs[nr].function = 0;
	_breakpoint[nr].file = NULL;
	_CPU_MUTEX_UNLOCK(level);
	return 0;
}

int _core_debug_watchpoint_busy(int nr) {
	struct watchpoint_regs *wp_regs = WATCHPOINT_REG();
	uint32_t func = wp_regs[nr].function;
	return func != 0;
}

#if !defined(__ZEPHYR__)
int _core_debug_setup(cpu_debug_handler_t cb) {
	_watchpoint_nums = WATCHPOINT_MAX_NR;
	__core_debug_init();
	for (int i = 0; i < _watchpoint_nums; i++)
		_core_debug_watchpoint_disable(i);
	if (cb) {
		_debug_cb_handler = cb;
		return __core_debug_enable_monitor_exception();
	}
	return 0;
}
#else /* __ZEPHYR__ */

static int _core_debugger_setup(const struct device *dev __unused) {
	_watchpoint_nums = WATCHPOINT_MAX_NR;
	__core_debug_init();
	for (int i = 0; i < _watchpoint_nums; i++)
		_core_debug_watchpoint_disable(i);
	__core_debug_enable_monitor_exception();
	return 0;
}

static void _core_debugger_exception_entry(const struct printer *printer, 
	unsigned int reason, const z_arch_esf_t *esf) {
	int nr = core_watchpoint_match();
	if (nr >= 0) {
		struct breakpoint_info *bkpt = &_breakpoint[nr];
		virt_print(printer, "Hit Breakpoint: %s : %d\n", bkpt->file, bkpt->line);
	}
}

SYS_FATAL_DEFINE(watchpoint) = {
	.fatal = _core_debugger_exception_entry
};
SYS_INIT(_core_debugger_setup, 
	PRE_KERNEL_2, 0);
#endif /* __ZEPHYR__ */
