#include "pt_mcu_port.h"
#include "device.h"
#include "gpio/gpio_apollo3p.h"
#include "i2c/i2c-message.h"
#include "string.h"
#include "zephyr.h"

#define BUS_DEV "I2C_4"
#define CYTTSP5_I2C_IRQ_GPIO 46
#define CYTTSP5_I2C_RST_GPIO 61

#define PT_MCU_EXAMPLE_NAME "pt_i2c_adapter"
#define PT_I2C_DATA_SIZE (2 * 256)

struct pt_core_data {
	const struct device *dev;
	int irq_gpio;
	const struct device *irq_gpio_dev;
	int rst_gpio;
	const struct device *rst_gpio_dev;

	struct k_timer timer_60hz;

	int vdda_gpio;
	int vddd_gpio;
};

// for zephyr input device
struct cyttsp5_data my_cyttsp5_data;
/************************************/
/********** static storage **********/
struct pt_core_data *g_cdata;
static TOUCH_FUNC touch_handler;
static TOUCH_FUNC touch_noise_poller_60hz;
// static struct timer_list timer_60hz;
/************************************/

void add_touch_handler(TOUCH_FUNC handler)
{
	touch_handler = handler;
}
void add_touch_poller_60hz(TOUCH_FUNC handler)
{
	touch_noise_poller_60hz = handler;
}

/* 0-success !0-fail */
int pt_i2c_read(u8 addr, u8 *read_buf, u16 read_size, u16 buf_size)
{
	if (!g_cdata)
		return -EINVAL;

	if (!read_buf || !read_size || read_size > PT_I2C_DATA_SIZE)
		return -EINVAL;

	struct i2c_message msg = {
		.msg =
			{
				.flags = I2C_MSG_READ,
				.len = read_size,
				.buf = read_buf,
			},
		.reg_addr = 0x0300,
		.reg_len = 2,
	};

	return i2c_transfer(g_cdata->dev, &msg.msg, 1, addr);
}

/* 0-success !0-fail */
int pt_i2c_write(u8 addr, u8 *write_buf, u16 write_size)
{
	int rc = 0;
	if (!g_cdata)
		return -EINVAL;

	if (write_size > PT_I2C_DATA_SIZE)
		return -EINVAL;

	if (!write_buf || !write_size) {
#if 0
		if (!write_buf)
			dev_err(g_cdata->dev, "%s write_buf is NULL", __func__);
		if (!write_size)
			dev_err(g_cdata->dev, "%s write_size is NULL", __func__);
#endif
		return -EINVAL;
	}

	if (write_size == 2) {
		uint16_t reg = write_buf[0] << 8 | write_buf[1];
		uint8_t reg_len = write_size;
		struct i2c_message msg = {
			.msg =
				{
					.flags = I2C_MSG_WRITE,
					.len = 0,
					.buf = NULL,
				},
			.reg_addr = reg,
			.reg_len = reg_len,
		};
		rc = i2c_transfer(g_cdata->dev, &msg.msg, 1, addr);
		if (rc < 0) {
			return rc;
		}
	} else {
		uint16_t reg = write_buf[0] << 8 | write_buf[1];
		uint8_t reg_len = 2;
		struct i2c_message msg = {
			.msg =
				{
					.flags = I2C_MSG_WRITE,
					.len = write_size - 2,
					.buf = &write_buf[2],
				},
			.reg_addr = reg,
			.reg_len = reg_len,
		};
		rc = i2c_transfer(g_cdata->dev, &msg.msg, 1, addr);
		if (rc < 0) {
			return rc;
		}
	}
	return 0;
}

int pt_set_vdda_gpio(u8 value)
{
#if 0
	int vdda_gpio = g_cdata->vdda_gpio;
	int rc = 0;

	if (!g_cdata)
		return -EINVAL;

	rc = gpio_request(vdda_gpio, NULL);
	if (rc < 0) {
		gpio_free(vdda_gpio);
		rc = gpio_request(vdda_gpio, NULL);
	}
	if (!rc) {
		rc = gpio_direction_output(vdda_gpio, value);
		gpio_free(vdda_gpio);
	}

	return rc;
#else
	return 0;
#endif
}

int pt_set_vddd_gpio(u8 value)
{
#if 0
	int vddd_gpio = g_cdata->vddd_gpio;
	int rc = 0;

	if (!g_cdata)
		return -EINVAL;

	rc = gpio_request(vddd_gpio, NULL);
	if (rc < 0) {
		gpio_free(vddd_gpio);
		rc = gpio_request(vddd_gpio, NULL);
	}
	if (!rc) {
		rc = gpio_direction_output(vddd_gpio, value);
		gpio_free(vddd_gpio);
	}

	return rc;
#else
	return 0;
#endif
}

int pt_set_rst_gpio(u8 value)
{
	gpio_pin_set(g_cdata->rst_gpio_dev, g_cdata->rst_gpio, value);
	return 0;
}

int pt_set_int_gpio_mode(void)
{
#if 0
	int irq_gpio = g_cdata->irq_gpio;
	int rc = 0;

	if (!g_cdata)
		return -EINVAL;

	rc = gpio_request(irq_gpio, NULL);
	if (rc < 0) {
		gpio_free(irq_gpio);
		rc = gpio_request(irq_gpio, NULL);
	}

	if (!rc) {
		rc = gpio_direction_input(irq_gpio);
		gpio_free(irq_gpio);
	}

	return rc;
#else
	return 0;
#endif
}

int pt_get_irq_state(void)
{
	return gpio_pin_get(g_cdata->irq_gpio_dev, g_cdata->irq_gpio & 31);
}

void pt_delay_ms(u32 ms)
{
	k_msleep(ms);
}

void pt_delay_us(u32 us)
{
	k_usleep(us);
}

/*******************************************/
/************ File operations **************/
/*******************************************/
struct file *g_filp = NULL;
/* 0 - Succes; !0 - Fail*/
int pt_open_file(char *file_path, int flags)
{
#if 0
	int rc = 0;
	if (file_path == NULL) {
		pr_err("%s: path is NULL.\n", __func__);
		return -EINVAL;
	}

	pr_info("%s: path = %s\n", __func__, file_path);

	g_filp = filp_open(file_path, flags, 0600);
	if (IS_ERR(g_filp)) {
		pr_err("%s: Failed to open %s\n", __func__, file_path);
		rc = -EIO;
		return rc;
	}

	return rc;
#else
	return 0;
#endif
}

int pt_rename_file(char *old_file, char *new_file)
{
#if 0
	int rc = 0;

	pr_err("%s: olde file is %s renamed to new_file %s\n", __func__, old_file,
		   new_file);

	return rc;
#else
	return 0;
#endif
}

int pt_close_file(void)
{
#if 0
	int rc;

	if (IS_ERR(g_filp)) {
		pr_err("%s: file is not available\n", __func__);
		return -EINVAL;
	}

	rc = filp_close(g_filp, NULL);
	if (rc)
		pr_err("%s: file close error.\n", __func__);

	return rc;
#else
	return 0;
#endif
}

int pt_write_file(const u8 *wbuf, u32 wlen)
{
#if 0
	ssize_t write_size = 0;

	if (IS_ERR(g_filp)) {
		pr_err("%s: file is not available\n", __func__);
		return -EINVAL;
	}

	write_size = kernel_write(g_filp, wbuf, wlen, &g_filp->f_pos);
	if (write_size < 0) {
		pr_err("%s: file write error %d\n", __func__, write_size);
		return -EIO;
	}

	return 0;
#else
	return 0;
#endif
}

int pt_read_file(u8 *rbuf, u32 rlen)
{
#if 0
	ssize_t read_size = 0;

	if (IS_ERR(g_filp)) {
		pr_err("%s: file is not available\n", __func__);
		return -EINVAL;
	}

	read_size = kernel_read(g_filp, rbuf, rlen, &g_filp->f_pos);
	if (read_size < 0) {
		pr_err("%s: file read error %d\n", __func__, read_size);
		return -EIO;
	}

	return 0;
#else
	return 0;
#endif
}

int pt_file_seek(u32 offset)
{
#if 0
	loff_t pos = 0;

	pos = generic_file_llseek(g_filp, offset, SEEK_SET);
	if (pos < 0) {
		pr_err("%s: file seek error %lld\n", __func__, pos);
		return -EIO;
	}

	return 0;
#else
	return 0;
#endif
}

/* return size of file */
u32 pt_file_size(void)
{
#if 0
	loff_t pos = 0;

	pos = default_llseek(g_filp, 0, SEEK_END);
	if (pos < 0) {
		pr_err("%s: file seek error %lld\n", __func__, pos);
		return 0xffffffff;
	}

	return (u32)pos;
#else
	return 0;
#endif
}

static void pt_irq_handler(int pin, void *params)
{
	if (touch_handler)
		touch_handler();
}

static int pt_setup_irq(struct pt_core_data *cdata, int on)
{
#if 0
	struct device *dev = cdata->dev;
	unsigned long irq_flags;
	int rc = 0;

	if (on) {
		/* Initialize IRQ */
		cdata->irq_num = gpio_to_irq(cdata->irq_gpio);

		if (cdata->irq_num < 0)
			return -EINVAL;

		dev_info(dev, "%s: initialize threaded irq=%d\n", __func__,
				 cdata->irq_num);

		/* use level triggered interrupts */
		/* irq_flags = IRQF_TRIGGER_LOW | IRQF_ONESHOT; */

		/* use edge triggered interrupts */
		irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;

		rc = request_threaded_irq(cdata->irq_num, NULL, pt_irq_handler,
								  irq_flags, dev_name(dev), g_cdata);
		if (rc < 0)
			dev_err(dev, "%s: Error, could not request irq\n", __func__);
	} else {
		disable_irq_nosync(cdata->irq_num);
		free_irq(cdata->irq_num, g_cdata);
	}
	return rc;
#else
	return 0;
#endif
}

void pt_disable_irq(void)
{
	gpio_apollo3p_request_irq(g_cdata->irq_gpio_dev, g_cdata->irq_gpio & 31, pt_irq_handler, g_cdata, GPIO_INT_TRIG_LOW,
							  false);
}

void pt_enable_irq(void)
{
	gpio_apollo3p_request_irq(g_cdata->irq_gpio_dev, g_cdata->irq_gpio & 31, pt_irq_handler, g_cdata, GPIO_INT_TRIG_LOW,
							  true);
}

unsigned int pt_get_time_stamp(void)
{
#if 0
	struct timeval tv;

	do_gettimeofday(&tv);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#else
	return 0;
#endif
}

static void pt_timer(struct k_timer *timer)
{
#if 0
	mod_timer(&timer_60hz, jiffies + msecs_to_jiffies(16));
	if (touch_noise_poller_60hz)
		touch_noise_poller_60hz();
#else
	if (touch_noise_poller_60hz)
		touch_noise_poller_60hz();

	k_timer_start(&g_cdata->timer_60hz, K_MSEC(20), K_NO_WAIT);
	return;
#endif
}

static void kscan_default_handler(const struct device *dev, uint32_t row, 
    uint32_t column, bool pressed)
{
    //LOG_WRN("Please set callback for touch-pannel\n");
}

static struct pt_core_data cd_prv = {0};
static int pt_i2c_probe(const struct device *dev)
{
	struct cyttsp5_data *data = dev->data;

	data->callback = kscan_default_handler;
	data->dev = dev;
	data->enable = false;

	struct pt_core_data *cdata;
	// u32 value;
	int rc = 0;
	const struct device *i2c_dev = device_get_binding(BUS_DEV);
	if (i2c_dev == NULL) {
		return -ENODEV;
	}

	cdata = &cd_prv;
	memset((char *)cdata, 0, sizeof(*cdata));

	g_cdata = cdata;
	cdata->dev = i2c_dev;

	cdata->irq_gpio = CYTTSP5_I2C_IRQ_GPIO & 31;
	cdata->rst_gpio = CYTTSP5_I2C_RST_GPIO & 31;

	const struct device *irq_gpio_dev = device_get_binding("GPIO_1");
	if (irq_gpio_dev == NULL) {
		rc = -ENODEV;
		goto exit;
	}
	cdata->irq_gpio_dev = irq_gpio_dev;

	const struct device *rst_gpio_dev = device_get_binding("GPIO_1");
	if (rst_gpio_dev == NULL) {
		rc = -ENODEV;
		goto exit;
	}
	cdata->rst_gpio_dev = rst_gpio_dev;

	if (gpio_apollo3p_request_irq(cdata->irq_gpio_dev, cdata->irq_gpio & 31, pt_irq_handler, cdata, GPIO_INT_TRIG_LOW,
								  false)) {
		rc = -ENODEV;
		goto exit;
	}

	// k_timer_init(&cdata->timer_60hz, pt_timer, NULL);
	// k_timer_start(&cdata->timer_60hz, K_MSEC(20), K_NO_WAIT);

	pt_example();

exit:
	return rc;
}

// for zephyr input device
int cyttsp5_config(const struct device *dev, kscan_callback_t callback)
{
	struct cyttsp5_data *data = dev->data;
	data->callback = callback;
	return 0;
}
int cyttsp5_disable_callback(const struct device *dev)
{
	struct cyttsp5_data *data = dev->data;
	data->enable = false;
	return 0;
}
int cyttsp5_enable_callback(const struct device *dev)
{
	struct cyttsp5_data *data = dev->data;
	data->enable = true;
	return 0;
}

static const struct kscan_driver_api cyttsp5_driver_api = {
	.config = cyttsp5_config,
	.enable_callback = cyttsp5_enable_callback,
	.disable_callback = cyttsp5_disable_callback,
};

DEVICE_DEFINE(touch_cyttsp5, "touch_cyttsp5", &pt_i2c_probe, NULL, &my_cyttsp5_data, NULL, POST_KERNEL,
			  CONFIG_APPLICATION_INIT_PRIORITY, &cyttsp5_driver_api);
