#include <kernel.h>
#include <init.h>
#include <errno.h>
#include <sys/atomic.h>
#include <sys/__assert.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(input);

#include "base/input_event.h"

#define INPUT_THREAD_PRIORITY 5
#define MAX_DEVICES 2

struct input_struct {
    struct k_poll_event poll_event;
    struct k_poll_signal poll_signal;
    atomic_t event;
    struct k_spinlock spin;
    struct input_report *report_list;
    const struct input_device *devices[MAX_DEVICES];
};

static K_KERNEL_STACK_DEFINE(input_stack, 2048);
static struct k_thread input_thread;
static struct input_struct input_handle;

int input_device_register(const struct input_device *idev, uint32_t event)
{
    if (idev == NULL)
        return -EINVAL;
    if (!event) {
        LOG_ERR("%s(): Event type(0x%x) is invalid\n", __func__, event);
        return -EINVAL;
    }

    int index = __builtin_ctz(event);
    if (index > MAX_DEVICES - 1) {
        LOG_ERR("%s(): Event type(0x%x) is not supported\n", __func__, event);
        return -EINVAL;
    }
    input_handle.devices[index] = idev;
    return 0;
}

int input_report_register(struct input_report *report)
{
    k_spinlock_key_t key;

    if (!report)
        return -EINVAL;
    if (!report->report)
        return -EINVAL;
    key = k_spin_lock(&input_handle.spin);
    report->next = input_handle.report_list;
    input_handle.report_list = report;
    k_spin_unlock(&input_handle.spin, key);
    return 0;
}

void input_event_post(uint32_t event)
{
    atomic_or(&input_handle.event, (atomic_val_t)event);
    k_poll_signal_raise(&input_handle.poll_signal, 0);
}

static void input_event_init(struct input_struct *evt)
{
    k_poll_signal_init(&evt->poll_signal);
    k_poll_event_init(&evt->poll_event, K_POLL_TYPE_SIGNAL,
                      K_POLL_MODE_NOTIFY_ONLY, &evt->poll_signal);
    atomic_set(&evt->event, 0);
}

static void input_event_report(struct input_struct *idev,
                               struct input_event *evt, uint32_t event)
{
    struct input_report *node = idev->report_list;
    while (node) {
        node->report(evt, event);
        node = node->next;
    }
}

static int input_event_wait(struct input_struct *evt, uint32_t *event)
{
    __ASSERT(event != NULL, "Invalid input parameters");
    int ret = k_poll(&evt->poll_event, 1, K_FOREVER);
    if (ret < 0)
        return ret;
    evt->poll_signal.signaled = 0;
    evt->poll_signal.result = 0;
    *event = (uint32_t)atomic_and(&evt->event, 0);
    return 0;
}

static void input_daemon_thread(void *arg)
{
    struct input_struct *input = arg;
    const struct input_device *idev;
    uint32_t events;

    for (; ;) {
        int ret = input_event_wait(input, &events);
        if (ret) {
            LOG_WRN("Wait event failed with error %d\n", ret);
            continue;
        }
        while (events) {
            int index = __builtin_ctz(events);
            idev = input->devices[index];
            if (idev)
                idev->read(idev->private_data, input, input_event_report);
            events &= ~BIT(index);
        }
    }
}

static int input_device_init(const struct device *dev __unused)
{
    k_tid_t handle;
    input_event_init(&input_handle);
    handle = k_thread_create(&input_thread,
                             input_stack,
                             K_KERNEL_STACK_SIZEOF(input_stack),
                             (k_thread_entry_t)input_daemon_thread,
                             &input_handle, NULL, NULL,
                             INPUT_THREAD_PRIORITY, 0,
                             K_NO_WAIT);
    k_thread_name_set(handle, "input-daemon");
    return 0;
}

SYS_INIT(input_device_init, POST_KERNEL,
         CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
