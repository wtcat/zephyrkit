#ifndef ZEPHYR_INCLUDE_DRIVERS_EXT_RTC_H_
#define ZEPHYR_INCLUDE_DRIVERS_EXT_RTC_H_

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>
#include <drivers/counter.h>

struct counter_alarm_wk_rpt_cfg {
	counter_alarm_callback_t callback;
	uint8_t weekday;
	uint8_t hour;
	uint8_t min;
	void *user_data;
	uint32_t flags;
};

typedef int (*counter_api_set_value)(const struct device *dev, uint32_t ticks);

// rtc extend api
typedef int (*counter_api_set_alarm_wk_rpt)(const struct device *dev, uint8_t chan_id,
											const struct counter_alarm_wk_rpt_cfg *alarm_cfg);

__subsystem struct rtc_driver_api {
	struct counter_driver_api base;
	// extend api
	counter_api_set_value set_value;
	counter_api_set_alarm_wk_rpt set_alarm_by_week_rpt;
};

/**
 * @brief Set current counter value.
 * @param dev Pointer to the device structure for the driver instance.
 * @param ticks Pointer to where to store the current counter value
 *
 * @retval 0 If successful.
 * @retval Negative error code on failure getting the counter value
 */
__syscall int counter_set_value(const struct device *dev, uint32_t ticks);

static inline int z_impl_counter_set_value(const struct device *dev, uint32_t ticks)
{
	const struct rtc_driver_api *api = (struct rtc_driver_api *)dev->api;

	return api->set_value(dev, ticks);
}

/**
 * @brief Set alarm by weekday
 * @param dev Pointer to the device structure for the driver instance.
 * @param ticks Pointer to where to store the current counter value
 *
 * @retval 0 If successful.
 * @retval Negative error code on failure setting alarm
 */
__syscall int counter_set_alarm_wk_rpt(const struct device *dev, uint8_t chan_id,
									   const struct counter_alarm_wk_rpt_cfg *alarm_cfg);

static inline int z_impl_counter_set_alarm_wk_rpt(const struct device *dev, uint8_t chan_id,
												  const struct counter_alarm_wk_rpt_cfg *alarm_cfg)
{
	const struct rtc_driver_api *api = (struct rtc_driver_api *)dev->api;

	if (chan_id >= counter_get_num_of_channels(dev)) {
		return -ENOTSUP;
	}

	return api->set_alarm_by_week_rpt(dev, chan_id, alarm_cfg);
}

#ifdef __cplusplus
}
#endif

#include <syscalls/rtc.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_COUNTER_H_ */
