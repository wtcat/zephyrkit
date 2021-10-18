/*
 * This file is part of the CmBacktrace Library.
 *
 * Copyright (c) 2016-2019, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Initialize function and other general function.
 * Created on: 2016-12-15
 */
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "cm_backtrace/cm_backtrace.h"

#if (CMB_OS_PLATFORM_TYPE != CMB_OS_PLATFORM_ZEPHYR)
#define __public_api 
#else
#define __public_api static
#endif

#if __STDC_VERSION__ < 199901L
#error "must be C99 or higher. try to add '-std=c99' to compile parameters"
#endif

#if defined(__CC_ARM) || defined(__CLANG_ARM)
#define SECTION_START(_name_) _name_##$$Base
#define SECTION_END(_name_) _name_##$$Limit
#define IMAGE_SECTION_START(_name_) Image$$##_name_##$$Base
#define IMAGE_SECTION_END(_name_) Image$$##_name_##$$Limit
#define CSTACK_BLOCK_START(_name_) SECTION_START(_name_)
#define CSTACK_BLOCK_END(_name_) SECTION_END(_name_)
#define CODE_SECTION_START(_name_) IMAGE_SECTION_START(_name_)
#define CODE_SECTION_END(_name_) IMAGE_SECTION_END(_name_)

extern const int CSTACK_BLOCK_START(CMB_CSTACK_BLOCK_NAME);
extern const int CSTACK_BLOCK_END(CMB_CSTACK_BLOCK_NAME);
extern const int CODE_SECTION_START(CMB_CODE_SECTION_NAME);
extern const int CODE_SECTION_END(CMB_CODE_SECTION_NAME);
#elif defined(__ICCARM__)
#pragma section = CMB_CSTACK_BLOCK_NAME
#pragma section = CMB_CODE_SECTION_NAME
#elif defined(__GNUC__)
extern const int CMB_CSTACK_BLOCK_START;
extern const int CMB_CSTACK_BLOCK_END;
extern const int CMB_CODE_SECTION_START;
extern const int CMB_CODE_SECTION_END;
#else
#error "not supported compiler"
#endif

enum {
	PRINT_MAIN_STACK_CFG_ERROR,
	PRINT_FIRMWARE_INFO,
	PRINT_ASSERT_ON_THREAD,
	PRINT_ASSERT_ON_HANDLER,
	PRINT_THREAD_STACK_INFO,
	PRINT_MAIN_STACK_INFO,
	PRINT_THREAD_STACK_OVERFLOW,
	PRINT_MAIN_STACK_OVERFLOW,
	PRINT_CALL_STACK_INFO,
	PRINT_CALL_STACK_ERR,
	PRINT_FAULT_ON_THREAD,
	PRINT_FAULT_ON_HANDLER,
	PRINT_REGS_TITLE,
	PRINT_HFSR_VECTBL,
	PRINT_MFSR_IACCVIOL,
	PRINT_MFSR_DACCVIOL,
	PRINT_MFSR_MUNSTKERR,
	PRINT_MFSR_MSTKERR,
	PRINT_MFSR_MLSPERR,
	PRINT_BFSR_IBUSERR,
	PRINT_BFSR_PRECISERR,
	PRINT_BFSR_IMPREISERR,
	PRINT_BFSR_UNSTKERR,
	PRINT_BFSR_STKERR,
	PRINT_BFSR_LSPERR,
	PRINT_UFSR_UNDEFINSTR,
	PRINT_UFSR_INVSTATE,
	PRINT_UFSR_INVPC,
	PRINT_UFSR_NOCP,
#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
	PRINT_UFSR_STKOF,
#endif
	PRINT_UFSR_UNALIGNED,
	PRINT_UFSR_DIVBYZERO0,
	PRINT_DFSR_HALTED,
	PRINT_DFSR_BKPT,
	PRINT_DFSR_DWTTRAP,
	PRINT_DFSR_VCATCH,
	PRINT_DFSR_EXTERNAL,
	PRINT_MMAR,
	PRINT_BFAR,
};

static const char *const print_info[] = {
	[PRINT_MAIN_STACK_CFG_ERROR]  = "ERROR: Unable to get the main stack information, please check the configuration of the main stack\n",
	[PRINT_FIRMWARE_INFO]         = "Firmware name: %s, hardware version: %s, software version: %s\n",
	[PRINT_ASSERT_ON_THREAD]      = "Assert on thread %s\n",
	[PRINT_ASSERT_ON_HANDLER]     = "Assert on interrupt or bare metal(no OS) environment\n",
	[PRINT_THREAD_STACK_INFO]     = "===== Thread stack information =====\n",
	[PRINT_MAIN_STACK_INFO]       = "====== Main stack information ======\n",
	[PRINT_THREAD_STACK_OVERFLOW] = "Error: Thread stack(%08x) was overflow\n",
	[PRINT_MAIN_STACK_OVERFLOW]   = "Error: Main stack(%08x) was overflow\n",
	[PRINT_CALL_STACK_INFO]       = "Show more call stack info by run: addr2line -e %s%s -a -f %.*s\n",
	[PRINT_CALL_STACK_ERR]        = "Dump call stack has an error\n",
	[PRINT_FAULT_ON_THREAD]       = "Fault on thread %s\n",
	[PRINT_FAULT_ON_HANDLER]      = "Fault on interrupt or bare metal(no OS) environment\n",
	[PRINT_REGS_TITLE]            = "=================== Registers information ====================\n",
	[PRINT_HFSR_VECTBL]           = "Hard fault is caused by failed vector fetch\n",
	[PRINT_MFSR_IACCVIOL]         = "Memory management fault is caused by instruction access violation\n",
	[PRINT_MFSR_DACCVIOL]         = "Memory management fault is caused by data access violation\n",
	[PRINT_MFSR_MUNSTKERR]        = "Memory management fault is caused by unstacking error\n",
	[PRINT_MFSR_MSTKERR]          = "Memory management fault is caused by stacking error\n",
	[PRINT_MFSR_MLSPERR]          = "Memory management fault is caused by floating-point lazy state preservation\n",
	[PRINT_BFSR_IBUSERR]          = "Bus fault is caused by instruction access violation\n",
	[PRINT_BFSR_PRECISERR]        = "Bus fault is caused by precise data access violation\n",
	[PRINT_BFSR_IMPREISERR]       = "Bus fault is caused by imprecise data access violation\n",
	[PRINT_BFSR_UNSTKERR]         = "Bus fault is caused by unstacking error\n",
	[PRINT_BFSR_STKERR]           = "Bus fault is caused by stacking error\n",
	[PRINT_BFSR_LSPERR]           = "Bus fault is caused by floating-point lazy state preservation\n",
	[PRINT_UFSR_UNDEFINSTR]       = "Usage fault is caused by attempts to execute an undefined instruction\n",
	[PRINT_UFSR_INVSTATE]         = "Usage fault is caused by attempts to switch to an invalid state (e.g., ARM)\n",
	[PRINT_UFSR_INVPC]            = "Usage fault is caused by attempts to do an exception with a bad value in the EXC_RETURN number\n",
	[PRINT_UFSR_NOCP]             = "Usage fault is caused by attempts to execute a coprocessor instruction\n",
	#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
	[PRINT_UFSR_STKOF]        = "Usage fault is caused by indicates that a stack overflow (hardware check) has taken place\n",
	#endif
	[PRINT_UFSR_UNALIGNED]        = "Usage fault is caused by indicates that an unaligned access fault has taken place\n",
	[PRINT_UFSR_DIVBYZERO0]       = "Usage fault is caused by Indicates a divide by zero has taken place (can be set only if DIV_0_TRP is set)\n",
	[PRINT_DFSR_HALTED]           = "Debug fault is caused by halt requested in NVIC\n",
	[PRINT_DFSR_BKPT]             = "Debug fault is caused by BKPT instruction executed\n",
	[PRINT_DFSR_DWTTRAP]          = "Debug fault is caused by DWT match occurred\n",
	[PRINT_DFSR_VCATCH]           = "Debug fault is caused by Vector fetch occurred\n",
	[PRINT_DFSR_EXTERNAL]         = "Debug fault is caused by EDBGRQ signal asserted\n",
	[PRINT_MMAR]                  = "The memory management fault occurred address is %08x\n",
	[PRINT_BFAR]                  = "The bus fault occurred address is %08x\n",
};

static const struct cm_printplugin *cm_iostream;
static char fw_name[CMB_NAME_MAX] = {0};
static char hw_ver[CMB_NAME_MAX] = {0};
static char sw_ver[CMB_NAME_MAX] = {0};
static uint32_t main_stack_start_addr = 0;
static size_t main_stack_size = 0;
static uint32_t code_start_addr = 0;
static size_t code_size = 0;
static bool init_ok = false;
static char call_stack_info[CMB_CALL_STACK_MAX_DEPTH * (8 + 1)] = {0};
static bool on_fault = false;
static bool stack_is_overflow = false;
static struct cmb_hard_fault_regs regs;
#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M4) || \
	(CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M7) || \
	(CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
static bool statck_has_fpu_regs = false;
#endif
static bool on_thread_before_fault = false;

static int cmb_println(const char *fmt, ...) {
	va_list ap;
	int ret;
	va_start(ap, fmt);
	ret = cm_iostream->printer(cm_iostream->context, fmt, ap);
	va_end(ap);
	return ret;
}

static void cm_backtrace_firmware_info(void)
{
	cmb_println(print_info[PRINT_FIRMWARE_INFO], 
		fw_name, hw_ver, sw_ver);
}

#ifdef CMB_USING_OS_PLATFORM
static void get_cur_thread_stack_info(uint32_t sp, uint32_t *start_addr, 
	size_t *size)
{
	CMB_ASSERT(start_addr);
	CMB_ASSERT(size);
#if (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_RTT)
	*start_addr = (uint32_t)rt_thread_self()->stack_addr;
	*size = rt_thread_self()->stack_size;
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_UCOSII)
	extern OS_TCB *OSTCBCur;

	*start_addr = (uint32_t)OSTCBCur->OSTCBStkBottom;
	*size = OSTCBCur->OSTCBStkSize * sizeof(OS_STK);
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_UCOSIII)
	extern OS_TCB *OSTCBCurPtr;

	*start_addr = (uint32_t)OSTCBCurPtr->StkBasePtr;
	*size = OSTCBCurPtr->StkSize * sizeof(CPU_STK_SIZE);
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_FREERTOS)
	*start_addr = (uint32_t)vTaskStackAddr();
	*size = vTaskStackSize() * sizeof(StackType_t);
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_ZEPHYR)
	k_tid_t tid = k_current_get();
	*start_addr = (uint32_t)tid->stack_info.start;
	*size = (uint32_t)tid->stack_info.size;
#endif
}

static const char *get_cur_thread_name(void)
{
#if (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_RTT)
	return rt_thread_self()->name;
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_UCOSII)
	extern OS_TCB *OSTCBCur;

#if OS_TASK_NAME_SIZE > 0 || OS_TASK_NAME_EN > 0
	return (const char *)OSTCBCur->OSTCBTaskName;
#else
	return NULL;
#endif /* OS_TASK_NAME_SIZE > 0 || OS_TASK_NAME_EN > 0 */

#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_UCOSIII)
	extern OS_TCB *OSTCBCurPtr;

	return (const char *)OSTCBCurPtr->NamePtr;
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_FREERTOS)
	return vTaskName();
#elif (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_ZEPHYR)
#if defined(CONFIG_THREAD_NAME)
	return k_current_get()->name;
#else
	return "***";
#endif /* CONFIG_THREAD_NAME */
#endif
}

#endif /* CMB_USING_OS_PLATFORM */

#ifdef CMB_USING_DUMP_STACK_INFO
static void dump_stack(uint32_t stack_start_addr, size_t stack_size, 
	uint32_t *stack_pointer)
{
	if (stack_is_overflow) {
		if (on_thread_before_fault) {
			cmb_println(print_info[PRINT_THREAD_STACK_OVERFLOW], 
				stack_pointer);
		} else {
			cmb_println(print_info[PRINT_MAIN_STACK_OVERFLOW], 
				stack_pointer);
		}
		if ((uint32_t)stack_pointer < stack_start_addr) {
			stack_pointer = (uint32_t *)stack_start_addr;
		} else if ((uint32_t)stack_pointer > stack_start_addr + stack_size) {
			stack_pointer = (uint32_t *)(stack_start_addr + stack_size);
		}
	}
	cmb_println(print_info[PRINT_THREAD_STACK_INFO], NULL);
	for (; (uint32_t)stack_pointer < stack_start_addr + stack_size; 
		stack_pointer++) {
		cmb_println("  addr: %08x    data: %08x\n", 
			stack_pointer, *stack_pointer);
	}
	cmb_println("====================================\n");
}
#endif /* CMB_USING_DUMP_STACK_INFO */

static bool disassembly_ins_is_bl_blx(uint32_t addr)
{
	uint16_t ins1 = *((uint16_t *)addr);
	uint16_t ins2 = *((uint16_t *)(addr + 2));

#define BL_INS_MASK 0xF800
#define BL_INS_HIGH 0xF800
#define BL_INS_LOW 0xF000
#define BLX_INX_MASK 0xFF00
#define BLX_INX 0x4700
	if ((ins2 & BL_INS_MASK) == BL_INS_HIGH && 
		(ins1 & BL_INS_MASK) == BL_INS_LOW) {
		return true;
	} else if ((ins2 & BLX_INX_MASK) == BLX_INX) {
		return true;
	} else {
		return false;
	}
}

size_t cm_backtrace_call_stack(uint32_t *buffer, size_t size, uint32_t sp)
{
	uint32_t stack_start_addr = main_stack_start_addr, pc;
	size_t depth = 0, stack_size = main_stack_size;
	bool regs_saved_lr_is_valid = false;

	if (on_fault) {
		if (!stack_is_overflow) {
			/* first depth is PC */
			buffer[depth++] = regs.saved.pc;
			/* fix the LR address in thumb mode */
			pc = regs.saved.lr - 1;
			if ((pc >= code_start_addr) && 
				(pc <= code_start_addr + code_size) && 
				(depth < CMB_CALL_STACK_MAX_DEPTH) &&
				(depth < size)) {
				buffer[depth++] = pc;
				regs_saved_lr_is_valid = true;
			}
		}

#ifdef CMB_USING_OS_PLATFORM
		/* program is running on thread before fault */
		if (on_thread_before_fault) {
			get_cur_thread_stack_info(sp, &stack_start_addr, &stack_size);
		}
	} else {
		/* OS environment */
		if (cmb_get_sp() == cmb_get_psp()) {
			get_cur_thread_stack_info(sp, &stack_start_addr, &stack_size);
		}
#endif /* CMB_USING_OS_PLATFORM */
	}

	if (stack_is_overflow) {
		if (sp < stack_start_addr) {
			sp = stack_start_addr;
		} else if (sp > stack_start_addr + stack_size) {
			sp = stack_start_addr + stack_size;
		}
	}

	/* copy called function address */
	for (; sp < stack_start_addr + stack_size; sp += sizeof(size_t)) {
		/* the *sp value may be LR, so need decrease a word to PC */
		pc = *((uint32_t *)sp) - sizeof(size_t);
		/* the Cortex-M using thumb instruction, so the pc must be an odd number */
		if (pc % 2 == 0) {
			continue;
		}
		/* fix the PC address in thumb mode */
		pc = *((uint32_t *)sp) - 1;
		if ((pc >= code_start_addr + sizeof(size_t)) && 
			(pc <= code_start_addr + code_size) &&
			(depth < CMB_CALL_STACK_MAX_DEPTH) && 
			/* check the the instruction before PC address is 'BL' or 'BLX' */
			disassembly_ins_is_bl_blx(pc - sizeof(size_t)) && 
			(depth < size)) {
			/* the second depth function may be already saved, so need ignore repeat */
			if ((depth == 2) && regs_saved_lr_is_valid && (pc == buffer[1])) {
				continue;
			}
			buffer[depth++] = pc;
		}
	}

	return depth;
}

static void print_call_stack(uint32_t sp)
{
	size_t i, cur_depth = 0;
	uint32_t call_stack_buf[CMB_CALL_STACK_MAX_DEPTH] = {0};

	cur_depth = cm_backtrace_call_stack(call_stack_buf, 
		CMB_CALL_STACK_MAX_DEPTH, sp);

	for (i = 0; i < cur_depth; i++) {
		sprintf(call_stack_info + i * (8 + 1), "%08lx", 
			(unsigned long)call_stack_buf[i]);
		call_stack_info[i * (8 + 1) + 8] = ' ';
	}

	if (cur_depth) {
		cmb_println(print_info[PRINT_CALL_STACK_INFO], 
			fw_name, 
			CMB_ELF_FILE_EXTENSION_NAME, 
			cur_depth * (8 + 1),
			call_stack_info);
	} else {
		cmb_println(print_info[PRINT_CALL_STACK_ERR], NULL);
	}
}

#ifdef CONFIG_CM_BACKTRACE_ASSERT
void cm_backtrace_assert(uint32_t sp)
{
	CMB_ASSERT(init_ok);

#ifdef CMB_USING_OS_PLATFORM
	uint32_t cur_stack_pointer = cmb_get_sp();
#endif

	cmb_println("\n");
	cm_backtrace_firmware_info();

#ifdef CMB_USING_OS_PLATFORM
	if (cur_stack_pointer == cmb_get_msp()) {
		cmb_println(print_info[PRINT_ASSERT_ON_HANDLER], NULL);

#ifdef CMB_USING_DUMP_STACK_INFO
		dump_stack(main_stack_start_addr, main_stack_size, (uint32_t *)sp);
#endif /* CMB_USING_DUMP_STACK_INFO */

	} else if (cur_stack_pointer == cmb_get_psp()) {
		cmb_println(print_info[PRINT_ASSERT_ON_THREAD], get_cur_thread_name());

#ifdef CMB_USING_DUMP_STACK_INFO
		uint32_t stack_start_addr;
		size_t stack_size;
		get_cur_thread_stack_info(sp, &stack_start_addr, &stack_size);
		dump_stack(stack_start_addr, stack_size, (uint32_t *)sp);
#endif /* CMB_USING_DUMP_STACK_INFO */
	}

#else

	/* bare metal(no OS) environment */
#ifdef CMB_USING_DUMP_STACK_INFO
	dump_stack(main_stack_start_addr, main_stack_size, (uint32_t *)sp);
#endif /* CMB_USING_DUMP_STACK_INFO */

#endif /* CMB_USING_OS_PLATFORM */

	print_call_stack(sp);
}
#endif /* CONFIG_CM_BACKTRACE_ASSERT */

#if (CMB_CPU_PLATFORM_TYPE != CMB_CPU_ARM_CORTEX_M0)
static void fault_diagnosis(void)
{
	if (regs.hfsr.bits.VECTBL) {
		cmb_println(print_info[PRINT_HFSR_VECTBL], NULL);
	}
	if (regs.hfsr.bits.FORCED) {
		/* Memory Management Fault */
		if (regs.mfsr.value) {
			if (regs.mfsr.bits.IACCVIOL) {
				cmb_println(print_info[PRINT_MFSR_IACCVIOL], NULL);
			}
			if (regs.mfsr.bits.DACCVIOL) {
				cmb_println(print_info[PRINT_MFSR_DACCVIOL], NULL);
			}
			if (regs.mfsr.bits.MUNSTKERR) {
				cmb_println(print_info[PRINT_MFSR_MUNSTKERR], NULL);
			}
			if (regs.mfsr.bits.MSTKERR) {
				cmb_println(print_info[PRINT_MFSR_MSTKERR], NULL);
			}

#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M4) || \
	(CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M7) || \
	(CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
			if (regs.mfsr.bits.MLSPERR) {
				cmb_println(print_info[PRINT_MFSR_MLSPERR], NULL);
			}
#endif

			if (regs.mfsr.bits.MMARVALID) {
				if (regs.mfsr.bits.IACCVIOL || regs.mfsr.bits.DACCVIOL) {
					cmb_println(print_info[PRINT_MMAR], regs.mmar);
				}
			}
		}
		/* Bus Fault */
		if (regs.bfsr.value) {
			if (regs.bfsr.bits.IBUSERR) {
				cmb_println(print_info[PRINT_BFSR_IBUSERR], NULL);
			}
			if (regs.bfsr.bits.PRECISERR) {
				cmb_println(print_info[PRINT_BFSR_PRECISERR], NULL);
			}
			if (regs.bfsr.bits.IMPREISERR) {
				cmb_println(print_info[PRINT_BFSR_IMPREISERR], NULL);
			}
			if (regs.bfsr.bits.UNSTKERR) {
				cmb_println(print_info[PRINT_BFSR_UNSTKERR], NULL);
			}
			if (regs.bfsr.bits.STKERR) {
				cmb_println(print_info[PRINT_BFSR_STKERR], NULL);
			}

#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M4) || \
	(CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M7) || \
	(CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
			if (regs.bfsr.bits.LSPERR) {
				cmb_println(print_info[PRINT_BFSR_LSPERR], NULL);
			}
#endif

			if (regs.bfsr.bits.BFARVALID) {
				if (regs.bfsr.bits.PRECISERR) {
					cmb_println(print_info[PRINT_BFAR], regs.bfar);
				}
			}
		}
		/* Usage Fault */
		if (regs.ufsr.value) {
			if (regs.ufsr.bits.UNDEFINSTR) {
				cmb_println(print_info[PRINT_UFSR_UNDEFINSTR], NULL);
			}
			if (regs.ufsr.bits.INVSTATE) {
				cmb_println(print_info[PRINT_UFSR_INVSTATE], NULL);
			}
			if (regs.ufsr.bits.INVPC) {
				cmb_println(print_info[PRINT_UFSR_INVPC], NULL);
			}
			if (regs.ufsr.bits.NOCP) {
				cmb_println(print_info[PRINT_UFSR_NOCP], NULL);
			}
#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
			if (regs.ufsr.bits.STKOF) {
				cmb_println(print_info[PRINT_UFSR_STKOF]);
			}
#endif
			if (regs.ufsr.bits.UNALIGNED) {
				cmb_println(print_info[PRINT_UFSR_UNALIGNED], NULL);
			}
			if (regs.ufsr.bits.DIVBYZERO0) {
				cmb_println(print_info[PRINT_UFSR_DIVBYZERO0], NULL);
			}
		}
	}
	/* Debug Fault */
	if (regs.hfsr.bits.DEBUGEVT) {
		if (regs.dfsr.value) {
			if (regs.dfsr.bits.HALTED) {
				cmb_println(print_info[PRINT_DFSR_HALTED], NULL);
			}
			if (regs.dfsr.bits.BKPT) {
				cmb_println(print_info[PRINT_DFSR_BKPT], NULL);
			}
			if (regs.dfsr.bits.DWTTRAP) {
				cmb_println(print_info[PRINT_DFSR_DWTTRAP], NULL);
			}
			if (regs.dfsr.bits.VCATCH) {
				cmb_println(print_info[PRINT_DFSR_VCATCH], NULL);
			}
			if (regs.dfsr.bits.EXTERNAL) {
				cmb_println(print_info[PRINT_DFSR_EXTERNAL], NULL);
			}
		}
	}
}
#endif /* (CMB_CPU_PLATFORM_TYPE != CMB_CPU_ARM_CORTEX_M0) */

#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M4) || \
	(CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M7) || \
	(CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
static uint32_t statck_del_fpu_regs(uint32_t fault_handler_lr, uint32_t sp)
{
	statck_has_fpu_regs = (fault_handler_lr & (1UL << 4)) == 0 ? 
		true : false;
	/* 
	 * the stack has S0~S15 and FPSCR registers when 
	 * statck_has_fpu_regs is true, double word align 
	 */
	return statck_has_fpu_regs == true ? 
		sp + sizeof(size_t) * 18 : sp;
}
#endif

__public_api
void cm_backtrace_fault(uint32_t fault_handler_lr, uint32_t fault_handler_sp)
{
	uint32_t stack_pointer = fault_handler_sp;
	uint32_t saved_regs_addr = stack_pointer;
	const char *regs_name[] = {
		"R0 ", "R1 ", "R2 ", "R3 ", "R12", "LR ", "PC ", "PSR"
	};

#ifdef CMB_USING_DUMP_STACK_INFO
	uint32_t stack_start_addr = main_stack_start_addr;
	size_t stack_size = main_stack_size;
#endif
	CMB_ASSERT(init_ok);
	CMB_ASSERT(!on_fault);

	on_fault = true;
	cmb_println("\n");
	cm_backtrace_firmware_info();
#ifdef CMB_USING_OS_PLATFORM
	on_thread_before_fault = fault_handler_lr & (1UL << 2);
	/* check which stack was used before (MSP or PSP) */
	if (on_thread_before_fault) {
		cmb_println(print_info[PRINT_FAULT_ON_THREAD],
					get_cur_thread_name() != NULL ? 
					get_cur_thread_name() : 
					"NO_NAME");
		saved_regs_addr = stack_pointer = cmb_get_psp();

#ifdef CMB_USING_DUMP_STACK_INFO
		get_cur_thread_stack_info(stack_pointer, 
			&stack_start_addr, &stack_size);
#endif /* CMB_USING_DUMP_STACK_INFO */

	} else {
		cmb_println((char *)print_info[PRINT_FAULT_ON_HANDLER]);
	}
#else
	cmb_println(print_info[PRINT_FAULT_ON_HANDLER]);
#endif /* CMB_USING_OS_PLATFORM */

	/* delete saved R0~R3, R12, LR,PC,xPSR registers space */
	stack_pointer += sizeof(size_t) * 8;
#if (CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M4) || \
	(CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M7) ||            \
	(CMB_CPU_PLATFORM_TYPE == CMB_CPU_ARM_CORTEX_M33)
	stack_pointer = statck_del_fpu_regs(fault_handler_lr, stack_pointer);
#endif

#ifdef CMB_USING_DUMP_STACK_INFO
	if (stack_pointer < stack_start_addr || 
		stack_pointer > stack_start_addr + stack_size) {
		stack_is_overflow = true;
	}
	dump_stack(stack_start_addr, stack_size, (uint32_t *)stack_pointer);
#endif /* CMB_USING_DUMP_STACK_INFO */
	if (!stack_is_overflow) {
		/* dump register */
		cmb_println(print_info[PRINT_REGS_TITLE]);

		regs.saved.r0 = ((uint32_t *)saved_regs_addr)[0];		 // R0
		regs.saved.r1 = ((uint32_t *)saved_regs_addr)[1];		 // R1
		regs.saved.r2 = ((uint32_t *)saved_regs_addr)[2];		 // R2
		regs.saved.r3 = ((uint32_t *)saved_regs_addr)[3];		 // R3
		regs.saved.r12 = ((uint32_t *)saved_regs_addr)[4];		 // R12
		regs.saved.lr = ((uint32_t *)saved_regs_addr)[5];		 // LR
		regs.saved.pc = ((uint32_t *)saved_regs_addr)[6];		 // PC
		regs.saved.psr.value = ((uint32_t *)saved_regs_addr)[7]; // PSR

		cmb_println("  %s: %08x  %s: %08x  %s: %08x  %s: %08x\n", 
			regs_name[0], regs.saved.r0, regs_name[1],
			regs.saved.r1, regs_name[2], regs.saved.r2, 
			regs_name[3], regs.saved.r3);
		cmb_println("  %s: %08x  %s: %08x  %s: %08x  %s: %08x\n", 
			regs_name[4], regs.saved.r12, regs_name[5],
			regs.saved.lr, regs_name[6], regs.saved.pc, 
			regs_name[7], regs.saved.psr.value);
		cmb_println(
			"==============================================================\n");
	}

	/* the Cortex-M0 is not support fault diagnosis */
#if (CMB_CPU_PLATFORM_TYPE != CMB_CPU_ARM_CORTEX_M0)
	regs.syshndctrl.value = CMB_SYSHND_CTRL; // System Handler Control and State Register
	regs.mfsr.value = CMB_NVIC_MFSR;		 // Memory Fault Status Register
	regs.mmar = CMB_NVIC_MMAR;				 // Memory Management Fault Address Register
	regs.bfsr.value = CMB_NVIC_BFSR;		 // Bus Fault Status Register
	regs.bfar = CMB_NVIC_BFAR;				 // Bus Fault Manage Address Register
	regs.ufsr.value = CMB_NVIC_UFSR;		 // Usage Fault Status Register
	regs.hfsr.value = CMB_NVIC_HFSR;		 // Hard Fault Status Register
	regs.dfsr.value = CMB_NVIC_DFSR;		 // Debug Fault Status Register
	regs.afsr = CMB_NVIC_AFSR;				 // Auxiliary Fault Status Register
	fault_diagnosis();
#endif
	print_call_stack(stack_pointer);
}

__public_api
int cm_backtrace_init(const struct cm_printplugin *printer, 
	const char *firmware_name, 
	const char *hardware_ver, 
	const char *software_ver)
{
	if (printer == NULL)
		return -EINVAL;
	cm_iostream = printer;
	strncpy(fw_name, firmware_name, CMB_NAME_MAX);
	strncpy(hw_ver, hardware_ver, CMB_NAME_MAX);
	strncpy(sw_ver, software_ver, CMB_NAME_MAX);

#if defined(__CC_ARM) || defined(__CLANG_ARM)
	main_stack_start_addr = 
		(uint32_t)&CSTACK_BLOCK_START(CMB_CSTACK_BLOCK_NAME);
	main_stack_size = 
		(uint32_t)&CSTACK_BLOCK_END(CMB_CSTACK_BLOCK_NAME) - 
		main_stack_start_addr;
	code_start_addr = 
		(uint32_t)&CODE_SECTION_START(CMB_CODE_SECTION_NAME);
	code_size = 
		(uint32_t)&CODE_SECTION_END(CMB_CODE_SECTION_NAME) - 
		code_start_addr;
#elif defined(__ICCARM__)
	main_stack_start_addr = 
		(uint32_t)__section_begin(CMB_CSTACK_BLOCK_NAME);
	main_stack_size = 
		(uint32_t)__section_end(CMB_CSTACK_BLOCK_NAME) - 
		main_stack_start_addr;
	code_start_addr = 
		(uint32_t)__section_begin(CMB_CODE_SECTION_NAME);
	code_size = 
		(uint32_t)__section_end(CMB_CODE_SECTION_NAME) - 
		code_start_addr;
#elif defined(__GNUC__)
#if (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_ZEPHYR)
	extern char z_interrupt_stacks[];
    main_stack_start_addr = (uint32_t)z_interrupt_stacks;
    main_stack_size = CONFIG_ISR_STACK_SIZE;
#else
    main_stack_start_addr = 
		(uint32_t)(&CMB_CSTACK_BLOCK_START);
    main_stack_size = 
		(uint32_t)(&CMB_CSTACK_BLOCK_END) - main_stack_start_addr;
#endif
    code_start_addr = 
		(uint32_t)(&CMB_CODE_SECTION_START);
    code_size = 
		(uint32_t)(&CMB_CODE_SECTION_END) - code_start_addr;
#else
#error "not supported compiler"
#endif
	if (main_stack_size == 0) {
		cmb_println(print_info[PRINT_MAIN_STACK_CFG_ERROR]);
		return -EINVAL;
	}
	init_ok = true;
	return 0;
}

#if (CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_ZEPHYR)
void k_sys_fatal_error_handler(unsigned int reason,
	const z_arch_esf_t *esf) {
	extern void arch_system_halt(unsigned int);
	cm_backtrace_fault(esf->extra_info.exc_return, esf->extra_info.msp);
	arch_system_halt(reason); 
}

static int zephyr_iostream_out(void *ctx, const char *fmt, va_list ap) {
	(void) ctx;
	vprintk(fmt, ap);
	return 0;
}

static const struct cm_printplugin zephyr_printer = {
	.printer = zephyr_iostream_out
};

static int zephyr_cm_backtrace_init(const struct device *dev __unused) {
	cm_backtrace_init(&zephyr_printer, "zephyr/zephyr", " ", " ");
	return 0;
}

SYS_INIT(zephyr_cm_backtrace_init, 
	PRE_KERNEL_2, 0);
#endif /* CMB_OS_PLATFORM_TYPE == CMB_OS_PLATFORM_ZEPHYR */
