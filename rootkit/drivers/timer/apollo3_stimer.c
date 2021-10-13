/*
 ***********************************************
 * Copyright (c) 2020 HonBow
 ***********************************************
 */

#include <sys/printk.h>
#include <version.h>
#include <soc.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/timer/system_timer.h>
#include <stdint.h>
#include <sys_clock.h>

#include <spinlock.h>

#define CFG_STIMER_CLOCK_HZ                     32768

#if CFG_STIMER_CLOCK_HZ == 32768
// Default - for backward compatibility
#define CFG_STIMER_CLOCK      AM_HAL_STIMER_XTAL_32KHZ
#else
#error "CFG_STIMER_CLOCK not specified"
#endif

#define CFG_TICK_RATE_HZ      CONFIG_SYS_CLOCK_TICKS_PER_SEC

/* The Stimer is a 32-bit counter. */
#define PORT_MAX_32_BIT_NUMBER		( 0xffffffffUL )

const static uint32_t stimer_cnts_for_oneTick = ( CFG_STIMER_CLOCK_HZ / CFG_TICK_RATE_HZ );

#ifdef CONFIG_APOLLO3_STIMER_BACKUP
const static uint32_t max_possible_suppressed_ticks = PORT_MAX_32_BIT_NUMBER / stimer_cnts_for_oneTick - 1;
#else
const static uint32_t max_possible_suppressed_ticks = PORT_MAX_32_BIT_NUMBER / stimer_cnts_for_oneTick;
#endif

static volatile uint32_t g_last_stimer_cnt = 0;        //record last stimer value
static volatile uint32_t accum_stimer_cnt;       //record total stimer value

static struct k_spinlock lock;

#if (KERNEL_VERSION_NUMBER >= ZEPHYR_VERSION(2,6,0))
#define z_clock_driver_init  sys_clock_driver_init
#define z_clock_elapsed      sys_clock_elapsed
#define z_clock_set_timeout  sys_clock_set_timeout
#define z_timer_cycle_get_32 sys_clock_cycle_get_32 
#define z_clock_announce     sys_clock_announce
#endif

//*****************************************************************************
// compare0 and compare1 will call this function
// to announce ticks elapsed since last announce!
//*****************************************************************************
static void xPort_stimer_Handler(uint32_t delta)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t stimer_cur_cnt = am_hal_stimer_counter_get();
	k_spin_unlock(&lock, key);

	uint32_t timer_counts;    //for z_clock_announce

	timer_counts = stimer_cur_cnt - g_last_stimer_cnt;

	g_last_stimer_cnt = stimer_cur_cnt;                  		   //record last cnt value
	accum_stimer_cnt += timer_counts;                  			   //total cnt increase

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		if (0 == delta){
			am_hal_stimer_compare_delta_set(0, stimer_cnts_for_oneTick);
			#ifdef CONFIG_APOLLO3_STIMER_BACKUP
			am_hal_stimer_compare_delta_set(1, stimer_cnts_for_oneTick+1);
			#endif
		} else {
			am_hal_stimer_compare_delta_set(0, stimer_cnts_for_oneTick-1);
			#ifdef CONFIG_APOLLO3_STIMER_BACKUP
			am_hal_stimer_compare_delta_set(1, stimer_cnts_for_oneTick);
			#endif

		}
	}
	/* announce the elapsed time in ticks */
	uint32_t ticks = timer_counts / stimer_cnts_for_oneTick;

	g_last_stimer_cnt -= timer_counts % stimer_cnts_for_oneTick;

	z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL)	? ticks : (ticks > 0));
}

//*****************************************************************************
// Interrupt handler for the STIMER module Compare 0.
//*****************************************************************************
static void am_stimer_cmpr0_isr(const void *unused)
{
	ARG_UNUSED(unused);

	// Check the timer interrupt status.
	uint32_t ui32Status = am_hal_stimer_int_status_get(false);
	if (ui32Status & AM_HAL_STIMER_INT_COMPAREA)
	{
		am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREA);
		// Run handlers for the various possible timer events.
		xPort_stimer_Handler(0);
	}
}

#ifdef CONFIG_APOLLO3_STIMER_BACKUP
//*****************************************************************************
// Interrupt handler for the STIMER module Compare 0.
//*****************************************************************************
static void am_stimer_cmpr1_isr(const void *unused)
{
	ARG_UNUSED(unused);

	// Check the timer interrupt status.
	uint32_t ui32Status = am_hal_stimer_int_status_get(false);
	if (ui32Status & AM_HAL_STIMER_INT_COMPAREB)
	{
		am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREB);
		// Run handlers for the various possible timer events.
		xPort_stimer_Handler(1);
	}
}
#endif

int z_clock_driver_init(const struct device *device)
{
	ARG_UNUSED(device);

	//clock setup
	uint32_t oldCfg;

	#ifdef CONFIG_APOLLO3_STIMER_BACKUP
		am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREA | AM_HAL_STIMER_INT_COMPAREB);
	#else
		am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREA);
	#endif

	//stimer cmpr0 enable
	irq_connect_dynamic(STIMER_CMPR0_IRQn, 0, am_stimer_cmpr0_isr, NULL, 0);
	irq_enable(STIMER_CMPR0_IRQn);

	#ifdef CONFIG_APOLLO3_STIMER_BACKUP  //enable cmpr1 if need
		irq_connect_dynamic(STIMER_CMPR1_IRQn, 0, am_stimer_cmpr1_isr, NULL, 0);
		irq_enable(STIMER_CMPR1_IRQn);
	#endif

	accum_stimer_cnt = 0;
	g_last_stimer_cnt = am_hal_stimer_counter_get();

	oldCfg = am_hal_stimer_config(AM_HAL_STIMER_CFG_FREEZE);

	/* Set the Delta value once the timer is enabled */
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* LPTIM1 is triggered on a LPTIM_TIMEBASE period */
		am_hal_stimer_compare_delta_set(0, stimer_cnts_for_oneTick);
		#ifdef CONFIG_APOLLO3_STIMER_BACKUP
		am_hal_stimer_compare_delta_set(1, stimer_cnts_for_oneTick + 1);
		#endif
	} else {
		uint32_t delta_cnts =  stimer_cnts_for_oneTick * ( max_possible_suppressed_ticks - 1 );
		am_hal_stimer_compare_delta_set(0, delta_cnts);
		#ifdef CONFIG_APOLLO3_STIMER_BACKUP
		am_hal_stimer_compare_delta_set(1, delta_cnts + 1);
		#endif
	}

	#ifdef CONFIG_APOLLO3_STIMER_BACKUP
		am_hal_stimer_config((oldCfg & ~(AM_HAL_STIMER_CFG_FREEZE | CTIMER_STCFG_CLKSEL_Msk)) \
								| CFG_STIMER_CLOCK | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE | AM_HAL_STIMER_CFG_COMPARE_B_ENABLE);
	#else
		am_hal_stimer_config((oldCfg & ~(AM_HAL_STIMER_CFG_FREEZE | CTIMER_STCFG_CLKSEL_Msk)) \
								| CFG_STIMER_CLOCK | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE);
	#endif

	return 0;
}

static inline uint32_t z_clock_stimer_getcounter(void)
{
	uint32_t stimer_cnt;
	uint32_t stimer_cnt_prev_read;

	stimer_cnt = am_hal_stimer_counter_get();
	do {
		stimer_cnt_prev_read = stimer_cnt;
		stimer_cnt = am_hal_stimer_counter_get();
	} while (stimer_cnt != stimer_cnt_prev_read);
	return stimer_cnt;
}

void z_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (ticks == K_TICKS_FOREVER) {
		am_hal_stimer_config(AM_HAL_STIMER_CFG_FREEZE);
		return;
	} else if (CTIMER->STCFG & AM_HAL_STIMER_CFG_FREEZE) { //TODO: if anything go wrong,break point here for a test!
		#ifdef CONFIG_APOLLO3_STIMER_BACKUP
		am_hal_stimer_config((CTIMER->STCFG & ~(AM_HAL_STIMER_CFG_FREEZE | CTIMER_STCFG_CLKSEL_Msk)) \
						| CFG_STIMER_CLOCK | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE | AM_HAL_STIMER_CFG_COMPARE_B_ENABLE);
		#else
		am_hal_stimer_config((CTIMER->STCFG & ~(AM_HAL_STIMER_CFG_FREEZE | CTIMER_STCFG_CLKSEL_Msk)) \
				| CFG_STIMER_CLOCK | AM_HAL_STIMER_CFG_COMPARE_A_ENABLE);
		#endif
	}

	#if 1
	uint32_t cur_cnt = am_hal_stimer_counter_get();
	uint32_t elapsed_cnts = cur_cnt - g_last_stimer_cnt;

	if (ticks * stimer_cnts_for_oneTick >  (PORT_MAX_32_BIT_NUMBER - elapsed_cnts)){
		ticks = (PORT_MAX_32_BIT_NUMBER - elapsed_cnts)/stimer_cnts_for_oneTick;
	} else if (ticks <= 1) {
		ticks = 2;
	}
	uint32_t delta_cnts =  stimer_cnts_for_oneTick * ticks;
	#endif

	k_spinlock_key_t key = k_spin_lock(&lock);

	am_hal_stimer_compare_delta_set(0, delta_cnts);
	#ifdef CONFIG_APOLLO3_STIMER_BACKUP
	am_hal_stimer_compare_delta_set(1, delta_cnts + 1);
	#endif

	k_spin_unlock(&lock, key);
}

uint32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t curr_cnt = z_clock_stimer_getcounter();
	k_spin_unlock(&lock, key);

	uint32_t cnts_elapsed = curr_cnt - g_last_stimer_cnt;
	uint64_t ret = (cnts_elapsed * CONFIG_SYS_CLOCK_TICKS_PER_SEC) / CFG_STIMER_CLOCK_HZ;

	return (uint32_t)(ret);
}

uint32_t z_timer_cycle_get_32(void)
{
	/* just gives the accumulated count in a number of hw cycles */
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t curr_cnt = z_clock_stimer_getcounter();
	k_spin_unlock(&lock, key);

	uint32_t cnts_total = curr_cnt - g_last_stimer_cnt;
	cnts_total += accum_stimer_cnt;

	/* convert lptim count in a nb of hw cycles with precision */
	uint64_t ret = cnts_total * (sys_clock_hw_cycles_per_sec() / CFG_STIMER_CLOCK_HZ);
	return (uint32_t)(ret);/* convert in hw cycles (keeping 32bit value) */
}
