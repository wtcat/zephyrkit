/*
 * Copyright (c) 2018 Workaround GmbH
 * Copyright (c) 2018 Allterco Robotics
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Source file for the STM32 RTC driver
 *
 */
#include "irq.h"

#include <errno.h>
#include <time.h>

#include <version.h>
#include <kernel.h>
#include <soc.h>
#include <sys/util.h>

#include <sys/timeutil.h>

#include <logging/log.h>

#include "drivers_ext/rtc.h"

#include <syscall_handler.h>

LOG_MODULE_REGISTER(counter_rtc_apollo3p, CONFIG_COUNTER_APOLLO_LOG_LEVEL);

/* Seconds from 1970-01-01T00:00:00 to 2000-01-01T00:00:00 */
#define T_TIME_OFFSET 946684800

#if defined(CONFIG_MULTITHREADING)
/* semaphore for locking flash resources (tickers) */
static unsigned int key;
#define IRQ_INIT()
#define IRQ_LOCK() key = irq_lock();
#define IRQ_UNLOCK() irq_unlock(key);
#else
#define IRQ_INIT()
#define IRQ_LOCK()
#define IRQ_UNLOCK()
#endif

struct rtc_apollo3p_data {
	counter_alarm_callback_t callback;
	uint32_t ticks;
	void *user_data;
};

#define DEV_DATA(dev) ((struct rtc_apollo3p_data *)(dev)->data)
#define DEV_CFG(dev) ((const struct rtc_stm32_config *const)(dev)->config)

static void rtc_apoolo3p_irq_config(const struct device *dev);

static int rtc_apollo3p_start(const struct device *dev)
{
	ARG_UNUSED(dev);

#if 0
	am_hal_rtc_osc_enable();

	return 0;
#else
	return -ENOTSUP;
#endif
}

static int rtc_apollo3p_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

#if 0
	am_hal_rtc_osc_disable();
	return 0;
#else
	return -ENOTSUP;
#endif
}

static uint32_t rtc_apollo3p_read(const struct device *dev)
{
	struct tm now = {0};
	time_t ts;
	uint32_t ticks;

	ARG_UNUSED(dev);

	am_hal_rtc_time_t hal_time;
	am_hal_rtc_time_get(&hal_time);

	now.tm_year = 100 + hal_time.ui32Year;
	now.tm_mon = hal_time.ui32Month - 1; // rtc month: 1~12  posix month: 0~11
	now.tm_mday = hal_time.ui32DayOfMonth;
	now.tm_hour = hal_time.ui32Hour;
	now.tm_min = hal_time.ui32Minute;
	now.tm_sec = hal_time.ui32Second;

	ts = timeutil_timegm(&now);

	/* Return number of seconds since RTC init */

	__ASSERT(sizeof(time_t) == 8, "unexpected time_t definition");
	ticks = counter_us_to_ticks(dev, ts * USEC_PER_SEC);

	return ticks;
}

static int rtc_apollo3p_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = rtc_apollo3p_read(dev);
	return 0;
}

// TODO: need implement
static int rtc_apollo3p_set_value(const struct device *dev, uint32_t ticks)
{
	ARG_UNUSED(dev);

	IRQ_LOCK();

	uint64_t us_cnt = counter_ticks_to_us(dev, ticks);

	time_t now = us_cnt / 1000000UL;

	struct tm *tm_now = gmtime(&now);

	am_hal_rtc_time_t hal_time;

	if (tm_now->tm_year > 100) {
		hal_time.ui32Year = tm_now->tm_year - 100;
	} else {
		IRQ_UNLOCK();
		return -ENOTSUP;
	}

	hal_time.ui32Month =
		tm_now->tm_mon + 1; // rtc month: 1~12  posix month: 0~11
	hal_time.ui32DayOfMonth = tm_now->tm_mday;
	hal_time.ui32Hour = tm_now->tm_hour;
	hal_time.ui32Minute = tm_now->tm_min;
	hal_time.ui32Second = tm_now->tm_sec;
	hal_time.ui32Weekday = tm_now->tm_wday;
	hal_time.ui32Hundredths = 0;

	am_hal_rtc_time_set(&hal_time);

	IRQ_UNLOCK();

	return 0;
}

static int rtc_apollo3p_set_alarm(const struct device *dev, uint8_t chan_id,
								  const struct counter_alarm_cfg *alarm_cfg)
{
	time_t alarm_val;

	am_hal_rtc_time_t rtc_alarm;

	struct rtc_apollo3p_data *data = DEV_DATA(dev);

	uint32_t now = rtc_apollo3p_read(dev);
	uint32_t ticks = alarm_cfg->ticks;

	if (data->callback != NULL) {
		LOG_DBG("Alarm busy\n");
		return -EBUSY;
	}

	data->callback = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		/* Add +1 in order to compensate the partially started tick.
		 * Alarm will expire between requested ticks and ticks+1.
		 * In case only 1 tick is requested, it will avoid
		 * that tick+1 event occurs before alarm setting is finished.
		 */
		ticks += now + 1;
	}

	LOG_DBG("Set Alarm: %d\n", ticks);

	alarm_val = (time_t)(counter_ticks_to_us(dev, ticks) / USEC_PER_SEC);

	struct tm *alarm_tm = gmtime(&alarm_val);

	rtc_alarm.ui32Month = alarm_tm->tm_mon + 1;
	rtc_alarm.ui32DayOfMonth = alarm_tm->tm_mday;
	rtc_alarm.ui32Hour = alarm_tm->tm_hour;
	rtc_alarm.ui32Minute = alarm_tm->tm_min;
	rtc_alarm.ui32Second = alarm_tm->tm_sec;
	rtc_alarm.ui32Hundredths = 0;

	am_hal_rtc_alarm_set(&rtc_alarm, AM_HAL_RTC_ALM_RPT_YR);

	return 0;
}

static int rtc_apollo3p_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	am_hal_rtc_alarm_interval_set(AM_HAL_RTC_ALM_RPT_DIS);

	uint32_t ui32Status = am_hal_rtc_int_status_get(false);
	if (ui32Status & AM_HAL_RTC_INT_ALM) {
		am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);
	}

	DEV_DATA(dev)->callback = NULL;

	return 0;
}

static uint32_t rtc_apollo3p_get_pending_int(const struct device *dev)
{
	return am_hal_rtc_int_status_get(true) != 0;
}

static uint32_t rtc_apollo3p_get_top_value(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;

	return info->max_top_value;
}

static int rtc_apollo3p_set_top_value(const struct device *dev,
									  const struct counter_top_cfg *cfg)
{
	const struct counter_config_info *info = dev->config;

	if ((cfg->ticks != info->max_top_value) ||
		!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		return -ENOTSUP;
	} else {
		return 0;
	}
}

static uint32_t __unused rtc_apollo3p_get_max_relative_alarm(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;

	return info->max_top_value;
}

void rtc_apollo3p_isr(const void *device)
{
	struct device *dev = (struct device *)device;

	struct rtc_apollo3p_data *data = dev->data;

	counter_alarm_callback_t alarm_callback = data->callback;

	uint32_t now = rtc_apollo3p_read(dev);

	uint32_t ui32Status = am_hal_rtc_int_status_get(false);
	if (ui32Status & AM_HAL_RTC_INT_ALM) {
		am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

		if (alarm_callback != NULL) {
			data->callback = NULL;
			alarm_callback(dev, 0, now, data->user_data);
		}
	}
}

static int rtc_apollo3p_set_alarm_wk_rpt(
	const struct device *dev, uint8_t chan_id,
	const struct counter_alarm_wk_rpt_cfg *alarm_cfg)
{
	am_hal_rtc_time_t rtc_alarm = {
		0,
	};
	struct rtc_apollo3p_data *data = DEV_DATA(dev);

	if (data->callback != NULL) {
		LOG_DBG("Alarm busy\n");
		return -EBUSY;
	}

	data->callback = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	LOG_DBG("Set Alarm: weekday: %d hour: %d min: %d\n", alarm_cfg->weekday,
			alarm_cfg->hour, alarm_cfg->min);

	rtc_alarm.ui32Weekday = alarm_cfg->weekday;
	rtc_alarm.ui32Hour = alarm_cfg->hour;
	rtc_alarm.ui32Minute = alarm_cfg->min;
	rtc_alarm.ui32Second = 0;
	rtc_alarm.ui32Hundredths = 0;

	am_hal_rtc_alarm_set(&rtc_alarm, AM_HAL_RTC_ALM_RPT_WK);

	am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

	am_hal_rtc_int_enable(AM_HAL_RTC_INT_ALM);

	return 0;
}

static int rtc_apollo3p_init(const struct device *dev)
{
	IRQ_INIT();

	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_START, 0);

	am_hal_rtc_osc_select(AM_HAL_RTC_OSC_XT);

	am_hal_rtc_osc_enable();

	DEV_DATA(dev)->callback = NULL;

	am_hal_rtc_alarm_interval_set(AM_HAL_RTC_ALM_RPT_DIS);

	am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

	am_hal_rtc_int_disable(AM_HAL_RTC_INT_ALM);

	rtc_apoolo3p_irq_config(dev);

	return 0;
}

static struct rtc_apollo3p_data rtc_data;

static struct counter_config_info counter_config = {
	.max_top_value = UINT32_MAX,
	.freq = 1,
	.flags = COUNTER_CONFIG_INFO_COUNT_UP,
	.channels = 1,
};

static const struct rtc_driver_api rtc_apollo3p_driver_api = {
	.base =
		{
			.start = rtc_apollo3p_start,
			.stop = rtc_apollo3p_stop,
			.get_value = rtc_apollo3p_get_value,
			.set_alarm = rtc_apollo3p_set_alarm,
			.cancel_alarm = rtc_apollo3p_cancel_alarm,
			.set_top_value = rtc_apollo3p_set_top_value,
			.get_pending_int = rtc_apollo3p_get_pending_int,
			.get_top_value = rtc_apollo3p_get_top_value,
#if (KERNEL_VERSION_NUMBER < ZEPHYR_VERSION(2,6,0))
			.get_max_relative_alarm = rtc_apollo3p_get_max_relative_alarm,
#endif
		},
	// extend api
	.set_value = rtc_apollo3p_set_value,
	.set_alarm_by_week_rpt = rtc_apollo3p_set_alarm_wk_rpt,
};

DEVICE_DEFINE(apollo3p_rtc, "apollo3p_rtc", &rtc_apollo3p_init,
			  NULL, &rtc_data, &counter_config, PRE_KERNEL_1,
			  CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			  &rtc_apollo3p_driver_api.base);

static void rtc_apoolo3p_irq_config(const struct device *dev)
{
	irq_connect_dynamic(RTC_IRQn, 0, rtc_apollo3p_isr, dev, 0);
	irq_enable(RTC_IRQn);
}
