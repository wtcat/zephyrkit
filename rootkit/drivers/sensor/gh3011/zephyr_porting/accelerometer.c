#include "accelerometer.h"
#include "zephyr.h"
#include "device.h"
#include "drivers/sensor.h"
#include "sys/sem.h"
#define SAMPLE_NUM_PER_CH 50

static int16_t mems_data[SAMPLE_NUM_PER_CH * 3];
static uint8_t cnt = 0;
static struct k_timer acc_timer;
const struct device *acc_dev;

static struct sys_sem sem_lock;
#define SYNC_INIT() sys_sem_init(&sem_lock, 1, 1)
#define SYNC_LOCK() sys_sem_take(&sem_lock, K_FOREVER)
#define SYNC_UNLOCK() sys_sem_give(&sem_lock)

static void acc_timer_handler(struct k_timer *timer)
{
	SYNC_LOCK();
	if (cnt <= 49) {
		struct sensor_value value[3];
		sensor_channel_get(acc_dev, SENSOR_CHAN_ACCEL_XYZ, value);
		mems_data[cnt * 3] = value[0].val1;
		mems_data[cnt * 3 + 1] = value[1].val1;
		mems_data[cnt * 3 + 2] = value[2].val1;
		cnt++;
	}
	SYNC_UNLOCK();
}

bool accelerometer_init(void)
{
	SYNC_INIT();
	k_timer_init(&acc_timer, acc_timer_handler, NULL);
	acc_dev = device_get_binding("ICM42607");
	return true;
}

void accelerometer_get_fifo(int16_t **fifo, uint32_t *fifo_size)
{
	SYNC_LOCK();
	*fifo = mems_data;
	*fifo_size = cnt <= SAMPLE_NUM_PER_CH - 1 ? cnt + 1 : SAMPLE_NUM_PER_CH;
	cnt = 0;
	SYNC_UNLOCK();
}

void accelerometer_start(void)
{
	k_timer_start(&acc_timer, K_MSEC(20), K_MSEC(20));
	SYNC_LOCK();
	cnt = 0;
	SYNC_UNLOCK();
}
void accelerometer_stop(void)
{
	k_timer_stop(&acc_timer);
	SYNC_LOCK();
	cnt = 0;
	SYNC_UNLOCK();
}