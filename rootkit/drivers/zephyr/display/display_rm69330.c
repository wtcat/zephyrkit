/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Marc Reilly
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#define DT_DRV_COMPAT sitronix_rm69330

#include "display_rm69330.h"

#include <device.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <drivers/display.h>
#include <soc.h>

#include "gpio/gpio_apollo3p.h"

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(rm69330);

struct lcd_render {
    struct k_sem sem;
    const uint8_t *buffer;

	//display parameter
	struct display_buffer_descriptor desc;
	uint16_t x_start;
	uint16_t y_start;	

    size_t remain;
    size_t row_size;
    bool   first;
    k_timeout_t timeout;
    void (*draw)(struct lcd_render *render);
    void (*state)(struct lcd_render *render);
};

struct rm69330_data {
	//private mspi device
	void *mspi_dev; 
	int	 spino;      
    int	 spi_irq;

	//reset gpio and te gpio
	const struct device *reset_gpio;
	int			pin_rst;
	const struct device *te_gpio;
	int			pin_te;

	//render for display
	struct lcd_render render;

	//rm69330 para
	uint16_t height;
	uint16_t width;
	uint16_t x_offset;
	uint16_t y_offset;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t pm_state;
#endif
};

struct lcd_cmd {
    uint8_t cmd;
    uint8_t data;
    uint8_t len;
    uint8_t delay_ms;
};

static struct rm69330_data rm69330_data;

#define INIT_CMD(_cmd, _data, _len, _delay) \
	{_cmd, _data, _len, _delay}

#ifdef CONFIG_RM69330_RGB565
#define RM69330_PIXEL_SIZE 2u
#else
#define RM69330_PIXEL_SIZE 3u
#endif

static void lcd_render_state_fire(struct lcd_render *render);
static void lcd_render_state_idle(struct lcd_render *render);
static void lcd_render_state_ready(struct lcd_render *render);
static void lcd_render_state_fire(struct lcd_render *render);

static void lcd_render_next(struct lcd_render *render)
{
    if (render->state != lcd_render_state_ready)
        render->state(render);
}

static void spi_dma_cb(void *pCallbackCtxt, uint32_t status)
{
	struct rm69330_data *data = pCallbackCtxt;
	(void)data;
	lcd_render_next(&data->render);
}

static int rm69330_write_cmd(void *dev, uint8_t cmd, const uint8_t *buffer, 
	size_t size, bool con)
{
	__ASSERT_NO_MSG(size <= 4);
	static const uint8_t addr_cmd[] = {0x02, 0x32};
    am_hal_mspi_pio_transfer_t  xfer;
	uint8_t format_buffer[32];
	
	format_buffer[0] = addr_cmd[con];
	format_buffer[1] = 0x00;
	format_buffer[2] = cmd;
	format_buffer[3] = 0x00;
	for (int i = 0; i < size; i++)
		format_buffer[4+i] = buffer[i];
	xfer.ui32NumBytes       = 4 + size;
	xfer.bDCX				= true;

    xfer.bScrambling        = false;
    xfer.eDirection         = AM_HAL_MSPI_TX;
    xfer.bSendAddr          = false;                    // always false
    xfer.ui32DeviceAddr     = 0;
    xfer.bSendInstr         = false;
    xfer.ui16DeviceInstr    = 0;
    xfer.bTurnaround        = false;
    xfer.bContinue          = con;
    xfer.bQuadCmd           = false;
    xfer.pui32Buffer        = (uint32_t *)format_buffer;

    /* Execute the transction over MSPI. */
    return am_hal_mspi_blocking_transfer(dev, &xfer, 100000);                      
}

static int rm69330_write_data(struct rm69330_data *data, const uint8_t *buffer, 
	size_t size, bool first)
{
	static const uint8_t wr_cmd[] = {0x3C, 0x2C};
    am_hal_mspi_dma_transfer_t	xfer;
    int rc;

	rc = rm69330_write_cmd(data->mspi_dev, wr_cmd[first], NULL, 0, true);
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
    return am_hal_mspi_nonblocking_transfer(data->mspi_dev, &xfer,
		AM_HAL_MSPI_TRANS_DMA, spi_dma_cb, data);					
}

static void lcd_set_rectangle(struct rm69330_data *data, uint16_t x0, uint16_t y0, 
	uint16_t x1, uint16_t y1)
{
    uint8_t cmd_buf[4];
    cmd_buf[0] = (x0 & 0xff00) >> 8U;
    cmd_buf[1] = (x0 & 0x00ff);        // column start : 0
    cmd_buf[2] = (y0 & 0xff00) >> 8U;
    cmd_buf[3] = (y0 & 0x00ff);          // column end   : 359
    rm69330_write_cmd(data->mspi_dev, 0x2A, cmd_buf, 4, false);
    
    cmd_buf[0] = (x1 & 0xff00) >> 8U;
    cmd_buf[1] = (x1 & 0x00ff);        // row start : 0
    cmd_buf[2] = (y1 & 0xff00) >> 8U;
    cmd_buf[3] = (y1 & 0x00ff);          // row end   : 359
    rm69330_write_cmd(data->mspi_dev, 0x2B, cmd_buf, 4, false);
}

static void rm69330_exit_sleep(struct rm69330_data *data)
{
	rm69330_write_cmd(data->mspi_dev, ST7789V_CMD_SLEEP_OUT, NULL, 0, false);
	k_sleep(K_MSEC(120));
}

static void rm69330_reset_display(struct rm69330_data *data)
{
	LOG_DBG("Resetting display");
#if 1 //DT_INST_NODE_HAS_PROP(0, reset_gpios)
	gpio_pin_set(data->reset_gpio, data->pin_rst, 0);
	k_sleep(K_MSEC(20));
	gpio_pin_set(data->reset_gpio, data->pin_rst, 1);
	k_sleep(K_MSEC(150));
#else
	rm69330_write_cmd(data->mspi_dev, ST7789V_CMD_SW_RESET, NULL, 0, false);
	k_sleep(K_MSEC(5));
#endif
}

static int rm69330_blanking_on(const struct device *dev)
{
	struct rm69330_data *data = (struct rm69330_data *)dev->data;

	rm69330_write_cmd(data->mspi_dev, ST7789V_CMD_DISP_OFF, NULL, 0, false);
	return 0;
}

static int rm69330_blanking_off(const struct device *dev)
{
	struct rm69330_data *data = (struct rm69330_data *)dev->data;

	rm69330_write_cmd(data->mspi_dev, ST7789V_CMD_DISP_ON, NULL, 0, false);
	return 0;
}

static int rm69330_read(const struct device *dev,
			const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	return -ENOTSUP;
}

static void lcd_draw_by_line(struct lcd_render *render)
{
    //lcd_dbg("LCD render line(buffer(%p): %d bytes)\n", render->buffer, render->row_size);
	struct rm69330_data *data = CONTAINER_OF(render, struct rm69330_data, render);
    rm69330_write_data(data, (const uint8_t *)render->buffer, 
        				render->row_size, render->first);
    render->buffer += render->row_size;
    render->remain -= render->row_size;
}

static void lcd_draw_by_dmabuf(struct lcd_render *render)
{
    size_t size = render->remain;
	struct rm69330_data *data = CONTAINER_OF(render, struct rm69330_data, render);
    //lcd_dbg("LCD render all(buffer(%p): %d bytes)\n", render->buffer, size);
    rm69330_write_data(data, (const uint8_t *)render->buffer, 
        size, render->first);
    render->remain -= size; 
}

static void lcd_render_state_ready(struct lcd_render *render)
{
    //lcd_dbg("LCD render ready (Remain: %d bytes)\n", render->remain);
    render->first = true;
	#if 0
	struct rm69330_data *data = CONTAINER_OF(render, struct rm69330_data, render);
	#else
	struct rm69330_data *data = &rm69330_data;
	#endif
    lcd_set_rectangle(data, render->x_start, render->x_start + render->desc.width - 1,
		render->y_start, render->y_start + render->desc.height - 1);
    render->draw(render);
    render->first = false; 
    render->state = lcd_render_state_fire;
}

static void lcd_render_state_fire(struct lcd_render *render)
{
    if (render->remain) {
        render->draw(render);
    } else {
        //lcd_dbg("LCD display completed (Enter idle state)\n");
        render->state = lcd_render_state_idle;
        k_sem_give(&render->sem);
	}
}

static void lcd_render_state_idle(struct lcd_render *render)
{
    
}

static void lcd_renderer_init(struct rm69330_data *data, 
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct lcd_render *render = &data->render;

	if (desc->pitch > desc->width) {
		render->draw = lcd_draw_by_line;
		render->row_size = desc->width * RM69330_PIXEL_SIZE;
	} else {
		render->draw = lcd_draw_by_dmabuf;
		render->row_size = desc->pitch * RM69330_PIXEL_SIZE;
	}

	render->buffer = buf;
	render->remain = (size_t)desc->width * RM69330_PIXEL_SIZE * desc->height;

	render->desc = *desc;
	render->x_start = x;
	render->y_start = y;

	__asm__ volatile("" ::: "memory");
	render->state = lcd_render_state_ready;
}

static void lcd_sync_render(struct rm69330_data *data)
{
    struct lcd_render *render = &data->render;
    
    if (render->state == lcd_render_state_ready)
        render->state(render);
}

static int rm69330_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct rm69330_data *data = (struct rm69330_data *)dev->data;
	struct lcd_render *render = &data->render;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * RM69330_PIXEL_SIZE * desc->height) <= desc->buf_size,
			"Input buffer to small");

	if (render->state == lcd_render_state_idle) {
        //lcd_dbg("LCD display start P0(%d, %d) \n", x, y);
        lcd_renderer_init(data, x, y, desc, buf);
        int ret = k_sem_take(&render->sem, render->timeout);
        if (ret) {
			printk("drv: write to rm69330 timeout!\n");
			return -EIO;
		}
    } else {
        printk("%s LCD controller occurred error\n", __func__);
		return -EIO;
    }
	return 0;
}

static void *rm69330_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int rm69330_set_brightness(const struct device *dev,
			   const uint8_t brightness)
{
	return -ENOTSUP;
}

static int rm69330_set_contrast(const struct device *dev,
			 const uint8_t contrast)
{
	return -ENOTSUP;
}

static void rm69330_get_capabilities(const struct device *dev,
			      struct display_capabilities *capabilities)
{
	struct rm69330_data *data = (struct rm69330_data *)dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = data->width;
	capabilities->y_resolution = data->height;

#ifdef CONFIG_RM69330_RGB565
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
#else
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_888;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
#endif
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int rm69330_set_pixel_format(const struct device *dev,
			     const enum display_pixel_format pixel_format)
{
#ifdef CONFIG_RM69330_RGB565
	if (pixel_format == PIXEL_FORMAT_RGB_565) {
#else
	if (pixel_format == PIXEL_FORMAT_RGB_888) {
#endif
		return 0;
	}
	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int rm69330_set_orientation(const struct device *dev,
			    const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static const struct lcd_cmd lcd_command[] = {
	INIT_CMD(0xFE,  0x01, 1, 0), 	/* Set to page CMD1 */
	INIT_CMD(0X0A,  0xF8, 1, 0),//E8   N565
	INIT_CMD(0X05,  0x20, 1, 0),
	INIT_CMD(0X06,  0x5A, 1, 0),
	INIT_CMD(0X0D,  0x00, 1, 0),
	INIT_CMD(0X0E,  0x81, 1, 0), //AVDD Charge Pump 81H-6.4V
	INIT_CMD(0X0F,  0x81, 1, 0), //AVDD Charge Pump 81H-6.4V idle
	INIT_CMD(0X10,  0x11, 1, 0), //AVDD/VCL REGULATOR ENABLE		avdd:2
	INIT_CMD(0X11,  0x81, 1, 0), //VCL Charge Pump 80:  vcc	1
	INIT_CMD(0X12,  0x81, 1, 0),
	INIT_CMD(0X13,  0x80, 1, 0), //VGH Charge Pump   avdd 	2
	INIT_CMD(0X14,  0x80, 1, 0),
	INIT_CMD(0X15,  0x81, 1, 0), //VGL Charge Pump  VCL-VCI		3
	INIT_CMD(0X16,  0x81, 1, 0), //VGL
	INIT_CMD(0X18,  0x66, 1, 0), //VGHR  44=5V 66=6v
	INIT_CMD(0X19,  0x88, 1, 0), //VGLR  33=-5V 55=-7 77=-9
	INIT_CMD(0X5B,  0x10, 1, 0), //01=VREFP on	 VREFN=off	10=??VREFPN5V Regulator Enable
	INIT_CMD(0X5C,  0x55, 1, 0), //VPREF5 and VNREF5 output status(default,
	INIT_CMD(0X62,  0x19, 1, 0), //Normal VREFN=-3.5V
	INIT_CMD(0X63,  0x19, 1, 0), //Idle VREFN=-3.5V
	INIT_CMD(0X70,  0x55, 1, 0), //Source Sequence 2
	INIT_CMD(0X25,  0x03, 1, 0), //Normal
	INIT_CMD(0X26,  0xC8, 1, 0),
	INIT_CMD(0X27,  0x0A, 1, 0),
	INIT_CMD(0X28,  0x0A, 1, 0),
	INIT_CMD(0X74,  0x0C, 1, 0), //OVDD / SD power source control
	INIT_CMD(0xC5,  0x10, 1, 0), // NOR=IDLE=GAM1 // HBM=GAM2
	INIT_CMD(0X2A,  0x23, 1, 0),  //IDLE  8Color
	INIT_CMD(0X2B,  0xC8, 1, 0),
	INIT_CMD(0X2D,  0x0A, 1, 0),//VBP
	INIT_CMD(0X2F,  0x0A, 1, 0),//VFP
	INIT_CMD(0X66,  0x90, 1, 0),  //Idle interal elvdd,elvss---------------------
	INIT_CMD(0X72,  0x1A, 1, 0),  //OVDD  4.6V
	INIT_CMD(0X73,  0x13, 1, 0),   //OVSS	13:-2.2 	  15:-2.4v
	INIT_CMD(0X30,  0x43, 1, 0), //43: 15Hz
	INIT_CMD(0xFE,  0x01, 1, 0),
	INIT_CMD(0x6A,  0x03, 1, 0),//????????OWER IC ???
	INIT_CMD(0X1B,  0x80, 1, 0),//8:Deep idle 8color 0:HBM 24bit


	//VSR power saving
	INIT_CMD(0x1D,  0x03, 1, 0),
	INIT_CMD(0x1E,  0x03, 1, 0),
	INIT_CMD(0x1F,  0x03, 1, 0),
	INIT_CMD(0x20,  0x03, 1, 0),
	INIT_CMD(0xFE,  0x04, 1, 0),
	INIT_CMD(0x63,  0x80, 1, 0),	///
	INIT_CMD(0xFE,  0x01, 1, 0),
	INIT_CMD(0X36,  0x00, 1, 0),  //Source Bias Current Control
	INIT_CMD(0X6D,  0x19, 1, 0), //VGMP VGSP turn off at idle mode
	INIT_CMD(0X6C,  0x80, 1, 0),  //SD_COPEN_OFF
	INIT_CMD(0xFE,  0x04, 1, 0),
	INIT_CMD(0x64,  0x0E, 1, 0),


	INIT_CMD(0XFE,  0x02, 1, 0),
	INIT_CMD(0XA9,  0x40, 1, 0),//6V VGMP		c8:4.5v
	INIT_CMD(0XAA,  0xBA, 1, 0),//2V VGSP E0=3V 90=2V	28:0.7v 		BA=2.525		50:1.2v
	INIT_CMD(0XAB,  0x01, 1, 0),//

	INIT_CMD(0xFE,  0x03, 1, 0),	//page2
	INIT_CMD(0XA9,  0x40, 1, 0), //6V VGMP
	INIT_CMD(0XAA,  0x90, 1, 0), //2V VGSP E0=3V 90=2V
	INIT_CMD(0XAB,  0x01, 1, 0), //

	//	VSR Command
	INIT_CMD(0XFE,  0x04, 1, 0),
	INIT_CMD(0X5D,  0x11, 1, 0),
	INIT_CMD(0X5A,  0x00, 1, 0),
	
	//VSR1
	INIT_CMD(0XFE,  0x04, 1, 0),
	INIT_CMD(0X00,  0x8D, 1, 0),
	INIT_CMD(0X01,  0x00, 1, 0),
	INIT_CMD(0X02,  0x01, 1, 0),


	INIT_CMD(0X03,  0x05, 1, 0),
	INIT_CMD(0X04,  0x00, 1, 0),
	INIT_CMD(0X05,  0x06, 1, 0),
	INIT_CMD(0X06,  0x00, 1, 0),
	INIT_CMD(0X07,  0x00, 1, 0),
	INIT_CMD(0X08,  0x00, 1, 0),
	
	//VSR2
	INIT_CMD(0XFE,  0x04, 1, 0),
	INIT_CMD(0X09,  0xCC, 1, 0),
	INIT_CMD(0X0A,  0x00, 1, 0),
	INIT_CMD(0X0B,  0x02, 1, 0),
	INIT_CMD(0X0C,  0x00, 1, 0),
	INIT_CMD(0X0D,  0x60, 1, 0),
	INIT_CMD(0X0E,  0x01, 1, 0),
	INIT_CMD(0X0F,  0x2C, 1, 0),
	INIT_CMD(0X10,  0x9C, 1, 0),
	INIT_CMD(0X11,  0x00, 1, 0),
	//VSR3
	INIT_CMD(0XFE,  0x04, 1, 0),
	INIT_CMD(0X12,  0xCC, 1, 0),
	INIT_CMD(0X13,  0x00, 1, 0),
	INIT_CMD(0X14,  0x02, 1, 0),
	INIT_CMD(0X15,  0x00, 1, 0),
	INIT_CMD(0X16,  0x60, 1, 0),
	INIT_CMD(0X17,  0x00, 1, 0),
	INIT_CMD(0X18,  0x2C, 1, 0),
	INIT_CMD(0X19,  0x9C, 1, 0),
	INIT_CMD(0X1A,  0x00, 1, 0),

	//VSR5
	INIT_CMD(0XFE,  0x04, 1, 0),
	INIT_CMD(0X24,  0xCC, 1, 0),
	INIT_CMD(0X25,  0x00, 1, 0),
	INIT_CMD(0X26,  0x04, 1, 0),
	INIT_CMD(0X27,  0x01, 1, 0),
	INIT_CMD(0X28,  0x60, 1, 0),
	INIT_CMD(0X29,  0x03, 1, 0),
	INIT_CMD(0X2A,  0x15, 1, 0),
	INIT_CMD(0X2B,  0xB1, 1, 0),
	INIT_CMD(0X2D,  0x00, 1, 0),
	//VSR6
	INIT_CMD(0XFE,  0x04, 1, 0),
	INIT_CMD(0X2F,  0xCC, 1, 0),
	INIT_CMD(0X30,  0x00, 1, 0),
	INIT_CMD(0X31,  0x04, 1, 0),
	INIT_CMD(0X32,  0x01, 1, 0),
	INIT_CMD(0X33,  0x60, 1, 0),
	INIT_CMD(0X34,  0x01, 1, 0),
	INIT_CMD(0X35,  0x15, 1, 0),
	INIT_CMD(0X36,  0xB1, 1, 0),
	INIT_CMD(0X37,  0x00, 1, 0),

	//VEN	EM-STV
	INIT_CMD(0XFE,  0x04, 1, 0),
	INIT_CMD(0X53,  0xCE, 1, 0),
	INIT_CMD(0X54,  0x00, 1, 0),
	INIT_CMD(0X55,  0x07, 1, 0),
	INIT_CMD(0X56,  0x01, 1, 0),
	INIT_CMD(0X58,  0x00, 1, 0),
	INIT_CMD(0X59,  0x00, 1, 0),
	INIT_CMD(0X65,  0x7C, 1, 0),
	INIT_CMD(0X66,  0x09, 1, 0),
	INIT_CMD(0X67,  0x10, 1, 0),
	
	INIT_CMD(0xFE,  0x01, 1, 0),
	INIT_CMD(0X3B,  0x41, 1, 0),//pre-charge on time
	INIT_CMD(0X3A,  0x00, 1, 0),//2Eh=2us 17h=1us 0Ch=0.5u
	INIT_CMD(0X3D,  0x17, 1, 0),
	INIT_CMD(0X3F,  0x69, 1, 0),//DMSW on time 41h=3us 5B=4u 2e=2u
	INIT_CMD(0X40,  0x17, 1, 0),
	INIT_CMD(0X41,  0x0D, 1, 0),//source delay
	INIT_CMD(0X37,  0x0C, 1, 0),//00??round 0C:VGSP	NO SWAP

	// VSR Marping command
	INIT_CMD(0XFE,  0x04, 1, 0),
	INIT_CMD(0X5E,  0x9B, 1, 0),
	INIT_CMD(0X5F,  0x45, 1, 0),
	INIT_CMD(0X60,  0x10, 1, 0),
	INIT_CMD(0X61,  0xB2, 1, 0),//EM_STV
	INIT_CMD(0X62,  0xBB, 1, 0),
	
	INIT_CMD(0XFE,  0x01, 1, 0),
	INIT_CMD(0X6A,  0x21, 1, 0),//33 Pulse ELVSS=-2.2V
	
	INIT_CMD(0XFE,  0x00, 1, 0),
#ifdef CONFIG_RM69330_RGB565
	INIT_CMD(0x3A,  0x75, 1, 0),//RGB565
#endif
	INIT_CMD(0X35,  0x00, 1, 0),
	INIT_CMD(0XC4,  0x80, 1, 0),
//	INIT_CMD(0X36,  0x08, 1, 0),//Red Blue change
//#if GUI_ANGE_MODE	== 1
	//INIT_CMD(0X36,  0x12, 1, 0),//Red Blue change
//#elif GUI_ANGE_MODE	== 0
	//INIT_CMD(0X36,  0x00, 1, 0),//Red Blue change
//#endif	
	INIT_CMD(0X51,  0xFF, 1, 0),
	INIT_CMD(0x11,  0x00, 1, 200),
	INIT_CMD(0x29,  0x00, 1, 0),
};

static int lcd_setup_commands(struct rm69330_data *data, 
	const struct lcd_cmd *cmd_table, size_t n)
{
	for (int i = 0; i < n; i++) {
		int rc = rm69330_write_cmd(data->mspi_dev, cmd_table[i].cmd, &cmd_table[i].data, 
			cmd_table[i].len, false);				
		if (rc != 0)
			return -EIO;
		
		if (cmd_table[i].delay_ms)
			k_sleep(K_MSEC(cmd_table[i].delay_ms));
	}
	return 0;
}

static int rm69330_lcd_init(struct rm69330_data *data)
{
    return lcd_setup_commands(data, lcd_command, ARRAY_SIZE(lcd_command));
}

static uint32_t dma_tx_buffer[2560]; //TODO: Reduce buffer size
const am_hal_mspi_dev_config_t lcd_spi_config = {
	  .eSpiMode 			= AM_HAL_MSPI_SPI_MODE_0,
	  .eClockFreq			= AM_HAL_MSPI_CLK_48MHZ,
	  .ui8TurnAround		= 0,
	  .eAddrCfg 			= AM_HAL_MSPI_ADDR_3_BYTE,
	  .eInstrCfg			= AM_HAL_MSPI_INSTR_1_BYTE,
	  .eDeviceConfig		= AM_HAL_MSPI_FLASH_QUAD_CE0,
	  .bSeparateIO			= false,
	  .bSendInstr			= false,
	  .bSendAddr			= false,
	  .bTurnaround			= false,
	  .ui8ReadInstr 		= 0x0B, //Fast read
	  .ui8WriteInstr		= 0x02, //Program page
	  .ui32TCBSize			= ARRAY_SIZE(dma_tx_buffer),
	  .pTCB 			= dma_tx_buffer,
	  .scramblingStartAddr	= 0,
	  .scramblingEndAddr	= 0,
};

static void rm69330_spi_isr(const void *arg)
{
	struct rm69330_data *data = (struct rm69330_data *)arg;
    uint32_t status = MSPIn(data->spino)->INTSTAT;

	MSPIn(data->spino)->INTCLR = status;
    am_hal_mspi_interrupt_service(data->mspi_dev, status);	
}

static void lcd_te_isr(int pin, void *params)
{
    struct rm69330_data *data = params;
    lcd_sync_render(data);
    (void) pin;
}

static int lcd_setup_spi(struct rm69330_data *data)
{
	const am_hal_mspi_dev_config_t *cfg = &lcd_spi_config;
	
#if AM_APOLLO3_MCUCTRL
	am_hal_mcuctrl_control(AM_HAL_MCUCTRL_CONTROL_FAULT_CAPTURE_ENABLE, 0);
#else
	am_hal_mcuctrl_fault_capture_enable();
#endif
	if (am_hal_mspi_initialize(data->spino, &data->mspi_dev))
		goto _err;

	if (am_hal_mspi_power_control(data->mspi_dev, AM_HAL_SYSCTRL_WAKE, false))
		goto _err;

	if (am_hal_mspi_device_configure(data->mspi_dev, (am_hal_mspi_dev_config_t *)cfg))
		goto _err;

	if (am_hal_mspi_enable(data->mspi_dev))
		goto _err;

    irq_connect_dynamic(data->spi_irq, 0, rm69330_spi_isr, data, 0);
    irq_enable(data->spi_irq);

	am_hal_mspi_interrupt_clear(data->mspi_dev, 
		AM_HAL_MSPI_INT_CQUPD|AM_HAL_MSPI_INT_ERR);
	am_hal_mspi_interrupt_enable(data->mspi_dev, 
		AM_HAL_MSPI_INT_CQUPD|AM_HAL_MSPI_INT_ERR);
	
	return 0;
_err:
	return -EIO;
}

static int rm69330_init(const struct device *dev)
{
	struct rm69330_data *data = (struct rm69330_data *)dev->data;

	data->reset_gpio = device_get_binding("GPIO_0"); //reset gpio device
    if (!data->reset_gpio) {
        return -ENODEV;
	}

	data->te_gpio = device_get_binding("GPIO_0");    //te gpio device
    if (!data->te_gpio) {
		return -ENODEV;
	}

	if (gpio_pin_configure(data->reset_gpio, data->pin_rst, //configure reset gpio
			       			GPIO_OUTPUT_LOW)) {
		LOG_ERR("Couldn't configure reset pin");
		return -EIO;
	}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	data->pm_state = DEVICE_PM_ACTIVE_STATE;
#endif

	k_sem_init(&data->render.sem, 0, 1);
	data->render.timeout = K_MSEC(100);

	/* Setup SPI interface */
    if (lcd_setup_spi(data)) { //private spi driver configure
		return -EIO;
	}

	rm69330_reset_display(data);


	if (rm69330_lcd_init(data)) {
		return -EIO;
	}
	
	int ret = gpio_apollo3p_request_irq(data->te_gpio, data->pin_te, lcd_te_isr, data,
         GPIO_INT_TRIG_LOW, true);
    if (ret) {
		return -EIO;
	}

	rm69330_blanking_on(dev);

	rm69330_exit_sleep(data);

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void rm69330_enter_sleep(struct rm69330_data *data)
{
	rm69330_write_cmd(data->mspi_dev, ST7789V_CMD_SLEEP_OUT, NULL, 0, false);
}

static int rm69330_pm_control(const struct device *dev, uint32_t ctrl_command,
				 void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;
	struct st7789v_data *data = (struct st7789v_data *)dev->data;

	switch (ctrl_command) {
	case DEVICE_PM_SET_POWER_STATE:
		if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			rm69330_exit_sleep(data);
			data->pm_state = DEVICE_PM_ACTIVE_STATE;
			ret = 0;
		} else {
			rm69330_enter_sleep(data);
			data->pm_state = DEVICE_PM_LOW_POWER_STATE;
			ret = 0;
		}
		break;
	case DEVICE_PM_GET_POWER_STATE:
		*((uint32_t *)context) = data->pm_state;
		break;
	default:
		ret = -EINVAL;
	}

	if (cb != NULL) {
		cb(dev, ret, context, arg);
	}
	return ret;
}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

static const struct display_driver_api rm69330_api = {
	.blanking_on = rm69330_blanking_on,
	.blanking_off = rm69330_blanking_off,
	.write = rm69330_write,
	.read = rm69330_read,
	.get_framebuffer = rm69330_get_framebuffer,
	.set_brightness = rm69330_set_brightness,
	.set_contrast = rm69330_set_contrast,
	.get_capabilities = rm69330_get_capabilities,
	.set_pixel_format = rm69330_set_pixel_format,
	.set_orientation = rm69330_set_orientation,
};

static struct rm69330_data rm69330_data = {
	.render = {
        .state = lcd_render_state_idle,
    },
	.width = 360,
	.height = 360,
	.x_offset = 0,
	.y_offset = 0,
	.spino = 0,
	.spi_irq = MSPI0_IRQn,
	.pin_te = 30,
	.pin_rst = 28,
};

#if CONFIG_DEVICE_POWER_MANAGEMENT
DEVICE_DEFINE(rm69330, DT_INST_LABEL(0), &rm69330_init,
	      rm69330_pm_control, &rm69330_data, NULL, APPLICATION,
	      CONFIG_APPLICATION_INIT_PRIORITY, &rm69330_api);
#else
DEVICE_DEFINE(rm69330, "rm69330", &rm69330_init,
	      NULL, &rm69330_data, NULL, APPLICATION,
	      CONFIG_APPLICATION_INIT_PRIORITY, &rm69330_api);
#endif