#include <errno.h>
#include <stdlib.h>

#include <sys/__assert.h>
#include <sys/atomic.h>

#include <device.h>
#include <drivers/gpio.h>
#include <kernel.h>
#include <soc.h>

#include "gx_api.h"
#include "gx_display.h"
#include "gx_utility.h"

#include "gpio/gpio_apollo3p.h"

//#define CONFIG_LCD_DEBUG
#define CONFIG_LCD_RGB565_FORMAT

#define OFFSET_OF_X 0x16
#define OFFSET_OF_Y 0

#define CONFIG_NO_TE_PIN 0

static uint8_t brightness;

struct lcd_driver;
struct lcd_render {
	struct k_sem sem;

	struct k_sem te_ready;
	struct k_mutex mutex_op;

	GX_RECTANGLE rect;
	struct lcd_driver *lcd;
	uint16_t *buffer;
	size_t remain;
	size_t row_size;
	bool first;
	k_timeout_t timeout;
	void (*draw)(struct lcd_render *render);
	void (*state)(struct lcd_render *render);

	struct GX_CANVAS_STRUCT *canvas;

#if IS_ENABLED(CONFIG_LCD_DOUBLE_BUFFER)
	struct GX_CANVAS_STRUCT *canvas_next;
	GX_RECTANGLE *area_next;
	struct k_spinlock lock;
#endif
};

struct lcd_driver {
	/*
	 * Inherit guix driver base class and must be
	 * place at first
	 */
	struct guix_driver drv;
	struct lcd_render render;
	struct observer_base pm;
	// struct gpio_callback cb;
	int spino;
	int spi_irq;
	void *spi;
	const struct device *gpio_rst;
	int pin_rst;
	const struct device *gpio_te;
	int pin_te;
};

struct lcd_cmd {
	uint8_t cmd;
	uint8_t data;
	uint8_t len;
	uint8_t delay_ms;
};

#define INIT_CMD(_cmd, _data, _len, _delay)                                                                            \
	{                                                                                                                  \
		_cmd, _data, _len, _delay                                                                                      \
	}

#ifdef CONFIG_LCD_RGB565_FORMAT
#define PIXEL_BYTE 2
#else
#define PIXEL_BYTE 3
#endif
#define LCD_WIDTH 320

#define MIN_WIDTH_BY_DMA 100
#define MIN_COL_SIZE_BY_DMA 5
#define LCD_HEIGH 360

#ifdef CONFIG_LCD_DEBUG
#define lcd_dbg(...) printk(__VA_ARGS__)
#else
#define lcd_dbg(...)
#endif

static void lcd_render_state_idle(struct lcd_render *render);
static void lcd_render_state_ready(struct lcd_render *render);
static void lcd_render_state_fire(struct lcd_render *render);
static void lcd_renderer_init(struct lcd_driver *lcd, struct GX_CANVAS_STRUCT *canvas, GX_RECTANGLE *area);

#if IS_ENABLED(CONFIG_LCD_DOUBLE_BUFFER)
#define LCD_BUFFER_SIZE ((LCD_WIDTH * LCD_HEIGH) / (sizeof(ULONG) / PIXEL_BYTE))
static ULONG lcd_fbuffer[LCD_BUFFER_SIZE];
static ULONG *lcd_current_fbuffer = lcd_fbuffer;
#endif

static const struct lcd_cmd lcd_command[] = {
	INIT_CMD(0xFE, 0x01, 1, 0), /* Set to page CMD1 */
	INIT_CMD(0x6A, 0x41, 1, 0), /*normal: ELVSS=-3.3V */
	INIT_CMD(0xAB, 0x3D, 1, 0), /*HBM: ELVSS=-3.7V */

	INIT_CMD(0xFE, 0x00, 1, 0), /* USER COMMAND */
	INIT_CMD(0x3A, 0x75, 1, 0), /* qspi 3553 format */
	INIT_CMD(0x80, 0x17, 1, 0), /* qspi 3553 format */
	INIT_CMD(0x35, 0x00, 1, 0), /* enable te*/
	INIT_CMD(0x53, 0x20, 1, 0), /* close diming*/
	INIT_CMD(0x51, 0xff, 1, 0),
};

static uint32_t dma_tx_buffer[2560]; // TODO: Reduce buffer size
const am_hal_mspi_dev_config_t lcd_spi_config = {
	.eSpiMode = AM_HAL_MSPI_SPI_MODE_0,
	.eClockFreq = AM_HAL_MSPI_CLK_48MHZ,
	.ui8TurnAround = 0,
	.eAddrCfg = AM_HAL_MSPI_ADDR_3_BYTE,
	.eInstrCfg = AM_HAL_MSPI_INSTR_1_BYTE,
	.eDeviceConfig = AM_HAL_MSPI_FLASH_QUAD_CE0,
	.bSeparateIO = false,
	.bSendInstr = false,
	.bSendAddr = false,
	.bTurnaround = false,
	.ui8ReadInstr = 0x0B,  // Fast read
	.ui8WriteInstr = 0x02, // Program page
	.ui32TCBSize = ARRAY_SIZE(dma_tx_buffer),
	.pTCB = dma_tx_buffer,
	.scramblingStartAddr = 0,
	.scramblingEndAddr = 0,
};

static void mdelay(unsigned int ms)
{
	k_busy_wait(ms * 1000);
}

static void lcd_spi_isr(const void *arg)
{
	struct lcd_driver *lcd = (struct lcd_driver *)arg;
	uint32_t status = MSPIn(lcd->spino)->INTSTAT;

	MSPIn(lcd->spino)->INTCLR = status;
	am_hal_mspi_interrupt_service(lcd->spi, status);
}

static void lcd_hardware_reset(struct lcd_driver *lcd)
{
	gpio_pin_set(lcd->gpio_rst, lcd->pin_rst, 0);
	// am_hal_gpio_output_clear(lcd->pin_rst);
	mdelay(20);
	gpio_pin_set(lcd->gpio_rst, lcd->pin_rst, 1);
	// am_hal_gpio_output_set(lcd->pin_rst);
	mdelay(150);
}

static void lcd_render_next(struct lcd_render *render)
{
	if (render->state != lcd_render_state_ready)
		render->state(render);
}

static void spi_dma_cb(void *pCallbackCtxt, uint32_t status)
{
	compiler_barrier();
	struct lcd_driver *lcd = pCallbackCtxt;
	lcd_render_next(&lcd->render);
}

static int lcd_write_cmd(void *dev, uint8_t cmd, const uint8_t *buffer, size_t size, bool con)
{
	__ASSERT_NO_MSG(size <= 4);
	static const uint8_t addr_cmd[] = {0x02, 0x32};
	am_hal_mspi_pio_transfer_t xfer;
	uint8_t format_buffer[32];

	format_buffer[0] = addr_cmd[con];
	format_buffer[1] = 0x00;
	format_buffer[2] = cmd;
	format_buffer[3] = 0x00;
	for (int i = 0; i < size; i++)
		format_buffer[4 + i] = buffer[i];
	xfer.ui32NumBytes = 4 + size;
	xfer.bDCX = true;

	xfer.bScrambling = false;
	xfer.eDirection = AM_HAL_MSPI_TX;
	xfer.bSendAddr = false; // always false
	xfer.ui32DeviceAddr = 0;
	xfer.bSendInstr = false;
	xfer.ui16DeviceInstr = 0;
	xfer.bTurnaround = false;
	xfer.bContinue = con;
	xfer.bQuadCmd = false;
	xfer.pui32Buffer = (uint32_t *)format_buffer;

	/* Execute the transction over MSPI. */
	return am_hal_mspi_blocking_transfer(dev, &xfer, 100000);
}

static int lcd_write_data(struct lcd_driver *lcd, const uint8_t *buffer, size_t size, bool first)
{
	static const uint8_t wr_cmd[] = {0x3C, 0x2C};
	am_hal_mspi_dma_transfer_t xfer;
	int rc;

	rc = lcd_write_cmd(lcd->spi, wr_cmd[first], NULL, 0, true);
	if (rc != 0)
		return -EIO;

	/* Set the DMA priority */
	xfer.ui8Priority = 1;

	/* Set the transfer direction to TX (Write) */
	xfer.eDirection = AM_HAL_MSPI_TX;
	xfer.ui32TransferCount = size;

	/* Set the source SRAM buffer address */
	xfer.ui32SRAMAddress = (uint32_t)buffer;

	/* Clear the CQ stimulus */
	xfer.ui32PauseCondition = 0;

	/* Clear the post-processing */
	xfer.ui32StatusSetClr = 0;

	/* Start the transaction */
	return am_hal_mspi_nonblocking_transfer(lcd->spi, &xfer, AM_HAL_MSPI_TRANS_DMA, spi_dma_cb, lcd);
}

static void lcd_set_rectangle(struct lcd_driver *lcd, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1)
{
	uint8_t cmd_buf[4];
	uint16_t x0_with_offset = x0 + OFFSET_OF_X;
	uint16_t x1_with_offset = x1 + OFFSET_OF_X;
	cmd_buf[0] = (x0_with_offset & 0xff00) >> 8U;
	cmd_buf[1] = (x0_with_offset & 0x00ff); // column start : 0
	cmd_buf[2] = (x1_with_offset & 0xff00) >> 8U;
	cmd_buf[3] = (x1_with_offset & 0x00ff); // column end   : 359
	lcd_write_cmd(lcd->spi, 0x2A, cmd_buf, 4, false);

	uint16_t y0_with_offset = y0 + OFFSET_OF_Y;
	uint16_t y1_with_offset = y1 + OFFSET_OF_Y;
	cmd_buf[0] = (y0_with_offset & 0xff00) >> 8U;
	cmd_buf[1] = (y0_with_offset & 0x00ff); // row start : 0
	cmd_buf[2] = (y1_with_offset & 0xff00) >> 8U;
	cmd_buf[3] = (y1_with_offset & 0x00ff); // row end   : 359
	lcd_write_cmd(lcd->spi, 0x2B, cmd_buf, 4, false);
}

static void lcd_set_partial_para(struct lcd_driver *lcd, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1)
{
	uint8_t cmd_buf[4];
	cmd_buf[0] = (y0 & 0xff00) >> 8U;
	cmd_buf[1] = (y0 & 0x00ff); // column start : 0
	cmd_buf[2] = (y1 & 0xff00) >> 8U;
	cmd_buf[3] = (y1 & 0x00ff); // column end   : 359
	lcd_write_cmd(lcd->spi, 0x30, cmd_buf, 4, false);

	cmd_buf[0] = (x0 & 0xff00) >> 8U;
	cmd_buf[1] = (x0 & 0x00ff); // row start : 0
	cmd_buf[2] = (x1 & 0xff00) >> 8U;
	cmd_buf[3] = (x1 & 0x00ff); // row end   : 359
	lcd_write_cmd(lcd->spi, 0x31, cmd_buf, 4, false);
}

static int lcd_setup_commands(struct lcd_driver *lcd, const struct lcd_cmd *cmd_table, size_t n)
{
	for (int i = 0; i < n; i++) {
		int rc = lcd_write_cmd(lcd->spi, cmd_table[i].cmd, &cmd_table[i].data, cmd_table[i].len, false);
		if (rc != 0)
			return -EIO;

		if (cmd_table[i].delay_ms)
			mdelay(cmd_table[i].delay_ms);
	}

	return 0;
}

static void lcd_draw_by_line(struct lcd_render *render)
{
	lcd_dbg("LCD render line(buffer(%p): %d bytes)\n", render->buffer, render->row_size);
	lcd_write_data(render->lcd, (const uint8_t *)render->buffer, render->row_size, render->first);
	render->buffer += render->canvas->gx_canvas_x_resolution;
	render->remain -= render->row_size;
}

static void lcd_draw_by_dmabuf(struct lcd_render *render)
{
	lcd_dbg("LCD render all(buffer(%p): %d bytes)\n", render->buffer, size);
	lcd_write_data(render->lcd, (const uint8_t *)render->buffer, render->remain, render->first);
	render->remain = 0;
}

static void lcd_render_state_ready(struct lcd_render *render)
{
	GX_RECTANGLE *area = &render->rect;

	lcd_dbg("LCD render ready (Remain: %d bytes)\n", render->remain);
	render->first = true;
	uint16_t left = area->gx_rectangle_left;
	uint16_t right = area->gx_rectangle_right;
	right = right >= LCD_WIDTH ? LCD_WIDTH - 1 : right;
	uint16_t top = area->gx_rectangle_top;
	uint16_t bottom = area->gx_rectangle_bottom;
	bottom = bottom >= LCD_HEIGH ? LCD_HEIGH - 1 : bottom;

	lcd_set_rectangle(render->lcd, left, right, top, bottom);
	render->draw(render);
	render->first = false;
	render->state = lcd_render_state_fire;
}

#if IS_ENABLED(CONFIG_LCD_DOUBLE_BUFFER)
static bool lcd_render_next_canvas(struct lcd_render *render)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&render->lock);
	if (render->canvas_next) {
		lcd_renderer_init(render->lcd, render->canvas_next, render->area_next);
		render->canvas_next = NULL;
		render->area_next = NULL;
		k_spin_unlock(&render->lock, key);
		return true;
	}
	k_spin_unlock(&render->lock, key);
	return false;
}
#endif

static void lcd_render_state_fire(struct lcd_render *render)
{
	if (render->remain) {
		render->draw(render);
	} else {
		lcd_dbg("LCD display completed (Enter idle state)\n");
		render->state = lcd_render_state_idle;
#if IS_ENABLED(CONFIG_LCD_DOUBLE_BUFFER)
		if (lcd_render_next_canvas(render))
			k_sem_give(&render->sem);
#else
		k_sem_give(&render->sem);
#endif
	}
}

static void lcd_render_state_idle(struct lcd_render *render)
{
}
static void lcd_sync_render(struct lcd_driver *lcd);
static void lcd_renderer_init(struct lcd_driver *lcd, struct GX_CANVAS_STRUCT *canvas, GX_RECTANGLE *area)
{
	GX_RECTANGLE limit;
#if !CONFIG_NO_TE_PIN
	k_sem_reset(&lcd->render.te_ready);
#endif
	_gx_utility_rectangle_define(&limit, 0, 0, canvas->gx_canvas_x_resolution - 1, canvas->gx_canvas_y_resolution - 1);
	if (_gx_utility_rectangle_overlap_detect(&limit, &canvas->gx_canvas_dirty_area, area)) {
		area->gx_rectangle_left &= 0xfffe;
		area->gx_rectangle_right |= 0x0001;
		area->gx_rectangle_top &= 0xfffe;
		area->gx_rectangle_bottom |= 0x0001;
		if (area->gx_rectangle_right >= LCD_WIDTH) {
			area->gx_rectangle_right = LCD_WIDTH - 1;
		}
		if (area->gx_rectangle_bottom >= LCD_HEIGH) {
			area->gx_rectangle_right = LCD_HEIGH - 1;
		}
		size_t row_size = (area->gx_rectangle_right - area->gx_rectangle_left + 1);
		size_t col_size = (area->gx_rectangle_bottom - area->gx_rectangle_top + 1);
		uint16_t *buffer = (uint16_t *)canvas->gx_canvas_memory;
		struct lcd_render *render = &lcd->render;
		ULONG offset;

		if ((row_size < MIN_WIDTH_BY_DMA) && (col_size < MIN_COL_SIZE_BY_DMA)) {
			render->draw = lcd_draw_by_line;
			render->row_size = row_size * PIXEL_BYTE;
			render->remain = row_size * col_size * PIXEL_BYTE;
		} else {
			render->draw = lcd_draw_by_dmabuf;
			render->row_size = LCD_WIDTH * PIXEL_BYTE;
			area->gx_rectangle_left = 0;
			area->gx_rectangle_right = LCD_WIDTH - 1;
			render->remain = LCD_WIDTH * col_size * PIXEL_BYTE;
		}

		offset = area->gx_rectangle_top * canvas->gx_canvas_x_resolution + area->gx_rectangle_left;
		render->buffer = buffer + offset;
		render->lcd = lcd;
		render->rect.gx_rectangle_left = area->gx_rectangle_left + canvas->gx_canvas_display_offset_x;
		render->rect.gx_rectangle_right = area->gx_rectangle_right + canvas->gx_canvas_display_offset_x;
		render->rect.gx_rectangle_top = area->gx_rectangle_top + canvas->gx_canvas_display_offset_y;
		render->rect.gx_rectangle_bottom = area->gx_rectangle_bottom + canvas->gx_canvas_display_offset_y;
		render->canvas = canvas;
		render->state = lcd_render_state_ready;

#if !CONFIG_NO_TE_PIN
		k_sem_take(&render->te_ready, K_FOREVER);
#endif
		lcd_sync_render(lcd);
	} else {
		k_sem_give(&lcd->render.sem);
	}
}

static void lcd_sync_render(struct lcd_driver *lcd)
{
	struct lcd_render *render = &lcd->render;
	if (render->state == lcd_render_state_ready)
		render->state(render);
}

static uint8_t value_test;
static void lcd_display_update(struct GX_CANVAS_STRUCT *canvas, GX_RECTANGLE *area)
{
	struct lcd_driver *lcd = canvas->gx_canvas_display->gx_display_driver_data;
	struct lcd_render *render = &lcd->render;
	GX_EVENT event;
	int ret;

	// value_test = gpio_pin_get(lcd->gpio_te, lcd->pin_te & 31);

#if IS_ENABLED(CONFIG_LCD_DOUBLE_BUFFER)
	k_spinlock_key_t key;
	key = k_spin_lock(&render->lock);

	if (render->state == lcd_render_state_idle) {
		lcd_dbg("LCD display start P0(%d, %d) P1(%d, %d)\n", area->gx_rectangle_left, area->gx_rectangle_top,
				area->gx_rectangle_right, area->gx_rectangle_bottom);
		lcd_renderer_init(lcd, canvas, area);
		k_spin_unlock(&render->lock, key);

		/* Switch frame buffer */
		ULONG *ptr = lcd_current_fbuffer;
		lcd_current_fbuffer = canvas->gx_canvas_memory;
		canvas->gx_canvas_memory = ptr;
		return;
	}

	render->area_next = area;
	render->canvas_next = canvas;
	k_spin_unlock(&render->lock, key);
	ret = k_sem_take(&render->sem, render->timeout);
	if (ret)
		goto reinit;
#else
	k_mutex_lock(&render->mutex_op, K_FOREVER);
	if (render->state == lcd_render_state_idle) {
		lcd_dbg("LCD display start P0(%d, %d) P1(%d, %d)\n", area->gx_rectangle_left, area->gx_rectangle_top,
				area->gx_rectangle_right, area->gx_rectangle_bottom);
		lcd_renderer_init(lcd, canvas, area);
		ret = k_sem_take(&render->sem, render->timeout);
		if (ret)
			goto reinit;
	} else {
		printk("%s LCD controller occurred error\n", __func__);
	}
#endif /* CONFIG_LCD_DOUBLE_BUFFER */
	k_mutex_unlock(&render->mutex_op);
	return;

reinit:
	printk("%s LCD render timeout(state: 0x%p)\n", __func__, render->state);
	lcd_hardware_reset(lcd);
	lcd_setup_commands(lcd, lcd_command, ARRAY_SIZE(lcd_command));
	brightness = 255;
#if 1
	lcd_set_partial_para(lcd, 0x15, 0x152, 0x01, 0x166);
#else
	lcd_set_partial_para(lcd, 0, 320, 0, 360);
#endif
	lcd_write_cmd(lcd->spi, 0x12, NULL, 0, false);
	lcd_write_cmd(lcd->spi, 0x11, NULL, 0, false);
	lcd_write_cmd(lcd->spi, 0x29, NULL, 0, false);
	render->state = lcd_render_state_idle;

	event.gx_event_type = GX_EVENT_REDRAW;
	event.gx_event_target = NULL;
	gx_system_event_fold(&event);
	k_mutex_unlock(&render->mutex_op);
}

static void lcd_te_isr(int pin, void *params)
{
	struct lcd_driver *lcd = params;
#if !CONFIG_NO_TE_PIN
	compiler_barrier();
	if (lcd->render.state == lcd_render_state_ready) {
		k_sem_give(&lcd->render.te_ready);
	}
#endif
	(void)pin;
}

static int lcd_setup_spi(struct lcd_driver *lcd)
{
	const am_hal_mspi_dev_config_t *cfg = &lcd_spi_config;

#if AM_APOLLO3_MCUCTRL
	am_hal_mcuctrl_control(AM_HAL_MCUCTRL_CONTROL_FAULT_CAPTURE_ENABLE, 0);
#else
	am_hal_mcuctrl_fault_capture_enable();
#endif
	if (am_hal_mspi_initialize(lcd->spino, &lcd->spi))
		goto _err;

	if (am_hal_mspi_power_control(lcd->spi, AM_HAL_SYSCTRL_WAKE, false))
		goto _err;

	if (am_hal_mspi_device_configure(lcd->spi, (am_hal_mspi_dev_config_t *)cfg))
		goto _err;

	if (am_hal_mspi_enable(lcd->spi))
		goto _err;

	irq_connect_dynamic(lcd->spi_irq, 0, lcd_spi_isr, lcd, 0);
	irq_enable(lcd->spi_irq);

	am_hal_mspi_interrupt_clear(lcd->spi, AM_HAL_MSPI_INT_CQUPD | AM_HAL_MSPI_INT_ERR);
	am_hal_mspi_interrupt_enable(lcd->spi, AM_HAL_MSPI_INT_CQUPD | AM_HAL_MSPI_INT_ERR);

	return 0;
_err:
	return -EIO;
}

static struct lcd_driver lcd_drv = {
	.render =
		{
			.state = lcd_render_state_idle,
		},
	.spino = 0,
	.spi_irq = MSPI0_IRQn,
	.spi = NULL,
	.pin_rst = 10,
	.pin_te = 28,
};

static int lcd_power_manage(struct observer_base *nb, unsigned long action, void *data)
{
	struct lcd_driver *lcd = &lcd_drv;

	switch (action) {
	case GUIX_ENTER_SLEEP:
		lcd_write_cmd(lcd->spi, 0x28, NULL, 0, false);
		break;
	case GUIX_EXIT_SLEEP:
		lcd_write_cmd(lcd->spi, 0x29, NULL, 0, false);
		break;
	}
	return NOTIFY_DONE;
}

int lcd_brightness_adjust(uint8_t value)
{
	struct lcd_driver *lcd = &lcd_drv;
	uint8_t bright = value;
#if 0
	k_mutex_lock(&lcd->render.mutex_op, K_FOREVER);
	if (lcd->render.state == lcd_render_state_idle) {
		unsigned key = irq_lock();
		lcd_write_cmd(lcd->spi, 0x51, &bright, 1, false);
		irq_unlock(key);
		k_mutex_unlock(&lcd->render.mutex_op);
		return 0;
	} else {
		printk("69092 busy!\n");
		k_mutex_unlock(&lcd->render.mutex_op);
		return -EBUSY;
	}
#else
	k_mutex_lock(&lcd->render.mutex_op, K_FOREVER);
	lcd_write_cmd(lcd->spi, 0x51, &bright, 1, false);
	k_mutex_unlock(&lcd->render.mutex_op);
	brightness = value;
#endif
}

uint8_t lcd_brightness_get(void)
{
	return brightness;
}

static VOID lcd_canvas_draw_start(struct GX_DISPLAY_STRUCT *display, struct GX_CANVAS_STRUCT *canvas)
{
	struct guix_driver *drv = display->gx_display_driver_data;
	guix_resource_map(drv, ~0u);
}

static VOID lcd_canvas_draw_stop(struct GX_DISPLAY_STRUCT *display, struct GX_CANVAS_STRUCT *canvas)
{
	struct guix_driver *drv = display->gx_display_driver_data;
	guix_resource_unmap(drv, NULL);
}

static UINT lcd_driver_setup(GX_DISPLAY *display)
{
#ifdef CONFIG_RGB565_SWAP_BYTEORDER
	extern VOID guix_rgb565_display_driver_swap_byteorder(GX_DISPLAY * display);
#endif

	struct lcd_driver *lcd = &lcd_drv;
	int ret;

	if (display->gx_display_driver_data)
		return GX_TRUE;

	lcd->gpio_rst = device_get_binding("GPIO_0");
	if (!lcd->gpio_rst)
		return -ENODEV;

	lcd->gpio_te = device_get_binding("GPIO_0");
	if (!lcd->gpio_te)
		return -ENODEV;

	ret = gpio_pin_configure(lcd->gpio_rst, lcd->pin_rst % 32, GPIO_OUTPUT_LOW);
	if (ret)
		goto exit;

	k_sem_init(&lcd->render.sem, 0, 1);

	k_sem_init(&lcd->render.te_ready, 0, 1);

	k_mutex_init(&lcd->render.mutex_op);

	/* Setup SPI interface */
	if (lcd_setup_spi(lcd))
		goto exit;

	/* Reset LCD chip */
	lcd_hardware_reset(lcd);

	/* Configure LCD */

	ret = gpio_apollo3p_request_irq(lcd->gpio_te, lcd->pin_te % 32, lcd_te_isr, lcd, GPIO_INT_TRIG_LOW, true);
	if (ret)
		goto exit;

	ret = lcd_setup_commands(lcd, lcd_command, ARRAY_SIZE(lcd_command));
	if (ret)
		goto exit;
	brightness = 255;

	lcd_set_partial_para(lcd, 0x15, 0x152, 0x01, 0x166);
	lcd_write_cmd(lcd->spi, 0x12, NULL, 0, false);
	lcd_write_cmd(lcd->spi, 0x11, NULL, 0, false);
	/* Display ON */
	lcd_write_cmd(lcd->spi, 0x29, NULL, 0, false);
	lcd->render.timeout = K_MSEC(2000);

#ifdef CONFIG_LCD_RGB565_FORMAT
	_gx_display_driver_565rgb_setup(display, lcd, lcd_display_update);

#ifdef CONFIG_RGB565_SWAP_BYTEORDER
	guix_rgb565_display_driver_swap_byteorder(display);
#endif /* CONFIG_SWAP_BYTEORDER */

#else
	_gx_display_driver_24xrgb_setup(display, lcd, lcd_display_update);
#endif
	display->gx_display_driver_drawing_initiate = lcd_canvas_draw_start;
	display->gx_display_driver_drawing_complete = lcd_canvas_draw_stop;

	lcd->pm.next = NULL;
	lcd->pm.priority = 1;
	lcd->pm.update = lcd_power_manage;
	guix_suspend_notify_register(&lcd->pm);
	return 0;

exit:
	return 1;
}

static int lcd_driver_init(const struct device *dev __unused)
{
	struct guix_driver *drv = &lcd_drv.drv;
	drv->id = 0;
	drv->setup = lcd_driver_setup;
	guix_driver_register(&lcd_drv.drv);
	return 0;
}

SYS_INIT(lcd_driver_init, PRE_KERNEL_2, 0);

/*
 * For touch pannel
 */
void *disp_driver_data = &lcd_drv; // TODO:This is ugly!!!
