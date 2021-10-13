#include <errno.h>
#include <kernel.h>
#include <init.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <sys/atomic.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include <soc.h>

#include "i2c/i2c-message.h"

#ifdef CONFIG_GUI_GPIO_KEY
#include "apollo3p_gpio_keys.h"
#endif

#ifdef CONFIG_GUIX
#include "gx_api.h"
#include "gx_system.h"
#endif

LOG_MODULE_REGISTER(tp);

//#define DEBUG_ITE7259

#define BUS_DEV "I2C_2"
#define GPIO_DEV "GPIO_1"

/*
 * Input event type
 */
#define INPUT_TP_EVENT   0x01
#define INPUT_KEY_EVENT 0x02

/* 
 * ITE7259 commands
 */
#define COMMAND_RESPONSE_BUFFER_INDEX 0xA0
#define ITE7259_CMD_ADDR      0x20
#define ITE7259_STATUS_ADDR   0x80
#define ITE7259_STATUS_BUSY   0x01
#define ITE7259_POINT_ADDR    0xE0
#define ITE7259_POINT_SIZE    14

#ifndef CONFIG_GUIX
#define GX_TOUCH_STATE_TOUCHED  1
#define GX_TOUCH_STATE_RELEASED 2

typedef struct _GX_POINT {
    int gx_point_x;
    int gx_point_y;
} GX_POINT;
#endif

struct input_evt {
	struct k_poll_event poll_event;
	struct k_poll_signal poll_signal;
	atomic_t event;
};

struct ite7259_private {
	struct input_evt evt;
    struct gpio_callback cb;
    const struct device *i2c;
    const struct device *gpio;
    struct k_thread *thread;
    uint16_t addr;
    uint16_t pin_rst;
    uint16_t pin_int;
    uint16_t reg_len;
    uint16_t chipid;
    GX_POINT current;
    GX_POINT last;
#ifdef CONFIG_GUIX
    int state;
    int drag_delta;
#endif
};

#define GPIO_PIN(n) ((n) & 31)

static K_KERNEL_STACK_DEFINE(tp_stack, 2048);
static struct k_thread tp_thread;


static struct ite7259_private k_touch = {
    .addr = 0x46,
    .pin_rst = GPIO_PIN(61),
    .pin_int = GPIO_PIN(62)
#ifdef CONFIG_GUIX
    ,
    .drag_delta = 8,
#endif
};

#ifdef CONFIG_GUIX
static void touch_down_event_send(GX_EVENT *event, 
    struct ite7259_private *touch)
{
    event->gx_event_type = GX_EVENT_PEN_DOWN;
    event->gx_event_payload.gx_event_pointdata = touch->current;

    _gx_system_event_send(event);
    touch->last = touch->current;
    touch->state = GX_TOUCH_STATE_TOUCHED;
#ifdef DEBUG_ITE7259
        printk("Touch Pannel(Down): X(%d) Y(%d)\n", touch->current.gx_point_x, 
            touch->current.gx_point_y);
#endif
}

static void touch_drag_event_send(GX_EVENT *event, 
    struct ite7259_private *touch)
{
    int x_delta = abs(touch->current.gx_point_x - touch->last.gx_point_x);
    int y_delta = abs(touch->current.gx_point_y - touch->last.gx_point_y);

    if (x_delta > touch->drag_delta ||
        y_delta > touch->drag_delta) {
        event->gx_event_type = GX_EVENT_PEN_DRAG;
        event->gx_event_payload.gx_event_pointdata = touch->current;
        touch->last = touch->current;
        _gx_system_event_fold(event);
#ifdef DEBUG_ITE7259
        printk("Touch Pannel(Drag): X(%d) Y(%d)\n", touch->current.gx_point_x, 
            touch->current.gx_point_y);
#endif
    }
}

static void touch_up_event_send(GX_EVENT *event, 
    struct ite7259_private *touch)
{
    event->gx_event_type = GX_EVENT_PEN_UP;
    event->gx_event_payload.gx_event_pointdata = touch->current;
    touch->last = touch->current;
    _gx_system_event_send(event);
    touch->state = GX_TOUCH_STATE_RELEASED;
#ifdef DEBUG_ITE7259
    printk("Touch Pannel(Up): X(%d) Y(%d)\n", touch->current.gx_point_x, 
        touch->current.gx_point_y);
#endif
}

static void touch_event_send(struct ite7259_private *touch, 
    GX_EVENT *event, int state)
{
    if (state == GX_TOUCH_STATE_TOUCHED) {
        if (touch->state == GX_TOUCH_STATE_TOUCHED)
            touch_drag_event_send(event, touch);
        else
            touch_down_event_send(event, touch);
    } else {
        touch_up_event_send(event, touch);
    }
}
#else /* !CONFIG_GUIX */
#define touch_event_send(...)
#endif /* CONFIG_GUIX */

static void input_evt_init(struct input_evt *evt)
{
	k_poll_signal_init(&evt->poll_signal);
	k_poll_event_init(&evt->poll_event, K_POLL_TYPE_SIGNAL,
			  K_POLL_MODE_NOTIFY_ONLY, &evt->poll_signal);
	atomic_set(&evt->event, 0);
}

static void input_evt_post(struct input_evt *evt, int event)
{
	atomic_or(&evt->event, event);
	k_poll_signal_raise(&evt->poll_signal, 0);
}

static int input_evt_wait(struct input_evt *evt, uint32_t *event)
{
	int ret;
	__ASSERT(event != NULL, "Invalid input parameters");
	ret = k_poll(&evt->poll_event, 1, K_FOREVER);
	if (ret < 0)
		return ret;

	evt->poll_signal.signaled = 0;
	evt->poll_signal.result = 0;
	*event = (uint32_t)atomic_and(&evt->event, 0);
	return 0;
}

static inline void ite7259_set_regsize(struct ite7259_private *priv, 
    uint16_t size)
{
    priv->reg_len = size;
}

static int ite7259_read(struct ite7259_private *priv,  uint16_t reg, 
        uint8_t *data, uint16_t len)
{
    struct i2c_message msg = {
        .msg = {
            .flags = I2C_MSG_READ,
            .len = len,
            .buf = data
        },
        .reg_addr = reg,
        .reg_len  = priv->reg_len
    };
	
    return i2c_transfer(priv->i2c, &msg.msg, 1, priv->addr);
}

static int __unused ite7259_write(struct ite7259_private *priv,  uint16_t reg, 
        uint8_t *data, uint16_t len)
{
    struct i2c_message msg = {
        .msg = {
            .flags = I2C_MSG_WRITE,
            .len = len,
            .buf = data
        },
        .reg_addr = reg,
        .reg_len  = priv->reg_len
    };
    
    return i2c_transfer(priv->i2c, &msg.msg, 1, priv->addr);
}

static void ite7259_reset(struct ite7259_private *priv)
{
    gpio_pin_set(priv->gpio, priv->pin_rst, 0);
    k_busy_wait(20 * 1000);
    gpio_pin_set(priv->gpio, priv->pin_rst, 1);
    k_busy_wait(200 * 1000);
}

static void ite7259_gpio_isr(const struct device *dev, struct gpio_callback *cb,
    gpio_port_pins_t pins)
{
    struct ite7259_private *priv;
	
    ARG_UNUSED(dev);
    ARG_UNUSED(pins);
    priv = CONTAINER_OF(cb, struct ite7259_private, cb);
    input_evt_post(&priv->evt, INPUT_TP_EVENT);
}

static bool ite7259_data_ready(struct ite7259_private *priv)
{
    int retry = 5;
    
    do {
        uint8_t state = 0;
        ite7259_read(priv, ITE7259_STATUS_ADDR, &state, 1);
        if (state & 0x80)
            return true;

        if (state & ITE7259_STATUS_BUSY) {
            k_yield();
            if (--retry <= 0)
                break;
        } else {
            break;
        }
    } while (true);

    return false;
}

static void touch_daemon_thread(void *p1, void *p2, void *p3)
{
    struct ite7259_private *priv = (struct ite7259_private *)p1;
    GX_POINT *current = &priv->current;
    uint8_t buf[ITE7259_POINT_SIZE];
	uint32_t event_out;
    int state, ret;
#ifdef DEBUG_ITE7259
    const char *state_string;
#endif
#ifdef CONFIG_GUIX
    extern void *disp_driver_data;
    GX_EVENT event;

    event.gx_event_sender = 0;
    event.gx_event_target = 0;
    event.gx_event_display_handle = (ULONG)disp_driver_data;
    state = GX_TOUCH_STATE_RELEASED;
#endif

    for ( ; ; ) {
		ret = input_evt_wait(&priv->evt, &event_out);
		if (ret) {
			LOG_WRN("Wait event failed with error %d\n", ret);
			continue;
	    }
		//k_sem_take(&priv->sync, K_FOREVER);
#ifdef CONFIG_GUI_GPIO_KEY
        if (event_out & INPUT_KEY_EVENT) {
            struct key_value value;
            while (!dequeue_gpio_key(&value)) {
                if (value.value == KEY_ACTION_CLICK) {
                    event.gx_event_type = GX_SIGNAL(0xFFFF, GX_EVENT_CLICKED);
                    _gx_system_event_send(&event);
                }
            }
        }
#endif
		if (!(event_out & INPUT_TP_EVENT))
			continue;

        if (!ite7259_data_ready(priv)) {
#ifdef CONFIG_GUIX
            if (priv->state == GX_TOUCH_STATE_TOUCHED)
                touch_up_event_send(&event, priv);
#endif
            continue;
        }

        /* Read data from device */
        ret = ite7259_read(priv, ITE7259_POINT_ADDR, buf, ITE7259_POINT_SIZE);
        if (ret) {
            printk("%s: read ite7259_private failed witch error %d\n", 
                __func__, ret);
            continue;
        }
        
        if (buf[0] == 0x09) {
            current->gx_point_x = ((uint16_t)(buf[3] & 0x0F) << 8) | buf[2];
            current->gx_point_y = ((uint16_t)(buf[3] & 0xF0) << 4) | buf[4];
            state = GX_TOUCH_STATE_TOUCHED;
        } else if (buf[0] == 0x80 && buf[1] > 0x1f && buf[1] < 0x43) {
            switch (buf[1]) {
            case 0x20:
            case 0x21:
            case 0x23:
                current->gx_point_x = ((uint16_t)buf[3] << 8) | buf[2];
                current->gx_point_y = ((uint16_t)buf[5] << 8) | buf[4];
                state = GX_TOUCH_STATE_RELEASED;
                break;
            case 0x22:
                //last->gx_point_x = ((uint16_t)(buf[3] & 0x0F) << 8) | buf[2];
                //last->gx_point_y = ((uint16_t)(buf[3] & 0xF0) << 4) | buf[4];
                current->gx_point_x = (uint16_t)buf[6] + ((uint16_t)buf[7] << 8);
                current->gx_point_y = (uint16_t)buf[8] + ((uint16_t)buf[9] << 8);                 
                state = GX_TOUCH_STATE_TOUCHED;
                break;
            default:
                break;
            }
        } else {
            if (state == GX_TOUCH_STATE_TOUCHED) {
                state = GX_TOUCH_STATE_RELEASED;
            } else {
                continue;
            }
        }

        touch_event_send(priv, &event, state);

    }
}

static int ite7259_check_id(struct ite7259_private *priv)
{
    uint8_t  id[2];
    int ret;
    
    ite7259_set_regsize(priv, 2);
    ret = ite7259_read(priv, 0x7032, &id[0], 1);
    if (ret)
        goto out;

    ret = ite7259_read(priv, 0x7033, &id[1], 1);
    if (ret)
        goto out;

    priv->chipid = ((uint16_t)id[1] << 8) | id[0];
    if (priv->chipid != 0x7259)
        ret = -EINVAL;
    
out:
    ite7259_set_regsize(priv, 1);
    return ret;
}

static int ite7259_setup(struct ite7259_private *priv)
{
    int ret;

    priv->i2c = device_get_binding(BUS_DEV);
    if (!priv->i2c)
        return -ENODEV;
	
    priv->gpio = device_get_binding(GPIO_DEV);
    if (!priv->i2c)
        return -ENODEV;
	
    ret = gpio_pin_configure(priv->gpio, priv->pin_rst,
        GPIO_OUTPUT_LOW);
    if (ret)
        goto out;
	
    ite7259_reset(priv);
    ret = ite7259_check_id(priv);
    if (ret) {
        printk("Touch device id is invalid\n");
        goto out;
    }
    
    input_evt_init(&priv->evt);
    gpio_init_callback(&priv->cb, ite7259_gpio_isr, BIT(priv->pin_int));
    gpio_add_callback(priv->gpio, &priv->cb);
    ret = gpio_pin_interrupt_configure(priv->gpio, priv->pin_int, 
        GPIO_INT_EDGE_FALLING);
    if (ret)
        printk("GPIO interrupt install failed with error %d\n", ret);
out:
    return ret;
}

#ifdef CONFIG_GUI_GPIO_KEY
static void gpio_key_notifier(void *context)
{
    struct ite7259_private *priv = context;
    input_evt_post(&priv->evt, INPUT_KEY_EVENT);
}
#endif /* CONFIG_GUIX */

static int ite7259_driver_init(const struct device *dev __unused)
{
    struct ite7259_private *priv = &k_touch;
    int ret;

    ret = ite7259_setup(&k_touch);
    if (ret) {
        printk("%s touch pannel intialize failed\n", __func__);
        return -EINVAL;
    }
    priv->thread = k_thread_create(&tp_thread,
                              tp_stack,
                              K_KERNEL_STACK_SIZEOF(tp_stack),
                              touch_daemon_thread,
                              priv, NULL, NULL,
                              GX_SYSTEM_THREAD_PRIORITY - 1, 0,
                              K_NO_WAIT);
    k_thread_name_set(priv->thread, "touch-panel");						  
#ifdef CONFIG_GUI_GPIO_KEY
    register_keydev_notify(gpio_key_notifier, &k_touch);
#endif
    return 0;
}

SYS_INIT(ite7259_driver_init, POST_KERNEL, 
    CONFIG_KERNEL_INIT_PRIORITY_DEVICE+5);
 
