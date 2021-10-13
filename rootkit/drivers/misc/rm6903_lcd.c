#include <errno.h>
#include <stdlib.h>

#include <sys/atomic.h>
#include <sys/__assert.h>

#include <kernel.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>

#include "gx_api.h"
#include "gx_display.h"
#include "gx_utility.h"

#include "gpio/gpio_apollo3p.h"


//#define CONFIG_LCD_DEBUG
#define CONFIG_SWAP_BYTEORDER
#define CONFIG_LCD_RGB565_FORMAT

struct lcd_driver;
struct lcd_render {
    struct k_sem sem;
    GX_RECTANGLE  rect;
    struct lcd_driver *lcd;
    uint16_t *buffer;
    size_t remain;
    size_t row_size;
    bool   first;
    k_timeout_t timeout;
    void (*draw)(struct lcd_render *render);
    void (*state)(struct lcd_render *render);
    
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
    //struct gpio_callback cb;
    const struct device *gpio;
    int			spino;
    int			spi_irq;
	void		*spi;
	int			pin_rst;
	int			pin_te;
};

struct lcd_cmd {
    uint8_t cmd;
    uint8_t data;
    uint8_t len;
    uint8_t delay_ms;
};

#define INIT_CMD(_cmd, _data, _len, _delay) \
	{_cmd, _data, _len, _delay}


#ifdef CONFIG_SWAP_BYTEORDER
#define __write_pixel(_addr, _color) \
    *(USHORT *)(_addr) = __builtin_bswap16((USHORT)_color)
#define __read_pixel(_addr) \
    __builtin_bswap16(*(USHORT *)_addr)
#endif


#ifdef CONFIG_LCD_RGB565_FORMAT
#define PIXEL_BYTE 2
#else
#define PIXEL_BYTE 3
#endif
#define LCD_WIDTH 360
#define LCD_HEIGH 360



#ifdef CONFIG_LCD_DEBUG
#define lcd_dbg(...) printk(__VA_ARGS__)
#else
#define lcd_dbg(...)
#endif
     

static void lcd_render_state_idle(struct lcd_render *render);
static void lcd_render_state_ready(struct lcd_render *render);
static void lcd_render_state_fire(struct lcd_render *render);
static void lcd_renderer_init(struct lcd_driver *lcd, 
    struct GX_CANVAS_STRUCT *canvas, GX_RECTANGLE *area);



#if IS_ENABLED(CONFIG_LCD_DOUBLE_BUFFER)
#define LCD_BUFFER_SIZE ((LCD_WIDTH * LCD_HEIGH) / (sizeof(ULONG) / PIXEL_BYTE))
static ULONG lcd_fbuffer[LCD_BUFFER_SIZE];
static ULONG *lcd_current_fbuffer = lcd_fbuffer;
#endif

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
#ifdef CONFIG_LCD_RGB565_FORMAT
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
    gpio_pin_set(lcd->gpio, lcd->pin_rst, 0); //am_hal_gpio_output_clear(lcd->pin_rst);
    mdelay(20);
    gpio_pin_set(lcd->gpio, lcd->pin_rst, 1); //am_hal_gpio_output_set(lcd->pin_rst);
    mdelay(150);
}

static void lcd_render_next(struct lcd_render *render)
{
    if (render->state != lcd_render_state_ready)
        render->state(render);
}

static void spi_dma_cb(void *pCallbackCtxt, uint32_t status)
{
	struct lcd_driver *lcd = pCallbackCtxt;
	lcd_render_next(&lcd->render);
}

static int lcd_write_cmd(void *dev, uint8_t cmd, const uint8_t *buffer, 
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

static int lcd_write_data(struct lcd_driver *lcd, const uint8_t *buffer, 
	size_t size, bool first)
{
	static const uint8_t wr_cmd[] = {0x3C, 0x2C};
    am_hal_mspi_dma_transfer_t	xfer;
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
    return am_hal_mspi_nonblocking_transfer(lcd->spi, &xfer,
		AM_HAL_MSPI_TRANS_DMA, spi_dma_cb, lcd);					
}

static void lcd_set_rectangle(struct lcd_driver *lcd, uint16_t x0, uint16_t y0, 
	uint16_t x1, uint16_t y1)
{
    uint8_t cmd_buf[4];
    cmd_buf[0] = (x0 & 0xff00) >> 8U;
    cmd_buf[1] = (x0 & 0x00ff);        // column start : 0
    cmd_buf[2] = (y0 & 0xff00) >> 8U;
    cmd_buf[3] = (y0 & 0x00ff);          // column end   : 359
    lcd_write_cmd(lcd->spi, 0x2A, cmd_buf, 4, false);
    
    cmd_buf[0] = (x1 & 0xff00) >> 8U;
    cmd_buf[1] = (x1 & 0x00ff);        // row start : 0
    cmd_buf[2] = (y1 & 0xff00) >> 8U;
    cmd_buf[3] = (y1 & 0x00ff);          // row end   : 359
    lcd_write_cmd(lcd->spi, 0x2B, cmd_buf, 4, false);
}

static int lcd_setup_commands(struct lcd_driver *lcd, 
	const struct lcd_cmd *cmd_table, size_t n)
{
	for (int i = 0; i < n; i++) {
		int rc = lcd_write_cmd(lcd->spi, cmd_table[i].cmd, &cmd_table[i].data, 
			cmd_table[i].len, false);				
		if (rc != 0)
			return -EIO;
		
		if (cmd_table[i].delay_ms)
			mdelay(cmd_table[i].delay_ms);
	}
	
	return 0;
}

static void lcd_draw_by_line(struct lcd_render *render)
{
    lcd_dbg("LCD render line(buffer(%p): %d bytes)\n", render->buffer,
        render->row_size);
    lcd_write_data(render->lcd, (const uint8_t *)render->buffer, 
        render->row_size, render->first);
    render->buffer += LCD_WIDTH;
    render->remain -= render->row_size;
}

static void lcd_draw_by_dmabuf(struct lcd_render *render)
{
    size_t size = render->remain;
    lcd_dbg("LCD render all(buffer(%p): %d bytes)\n", render->buffer,
        size);
    lcd_write_data(render->lcd, (const uint8_t *)render->buffer, 
        size, render->first);
    render->remain -= size; 
}

static void lcd_render_state_ready(struct lcd_render *render)
{
    GX_RECTANGLE *area = &render->rect;

    lcd_dbg("LCD render ready (Remain: %d bytes)\n", render->remain);
    render->first = true;
    lcd_set_rectangle(render->lcd, area->gx_rectangle_left, area->gx_rectangle_right, 
        area->gx_rectangle_top, area->gx_rectangle_bottom);
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
        lcd_renderer_init(render->lcd, render->canvas_next, 
            render->area_next);
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

static void lcd_renderer_init(struct lcd_driver *lcd, 
    struct GX_CANVAS_STRUCT *canvas, GX_RECTANGLE *area)
{
    GX_RECTANGLE limit;

    _gx_utility_rectangle_define(&limit, 0, 0,
                                canvas->gx_canvas_x_resolution - 1,
                                canvas->gx_canvas_y_resolution - 1);
    if (_gx_utility_rectangle_overlap_detect(&limit, 
        &canvas->gx_canvas_dirty_area, area)) {
        area->gx_rectangle_left  &= 0xfffe;
        area->gx_rectangle_right |= 0x0001;
        area->gx_rectangle_top &= 0xfffe;
        area->gx_rectangle_bottom |= 0x0001;
        size_t row_size = (area->gx_rectangle_right - area->gx_rectangle_left + 1);
        size_t col_size = (area->gx_rectangle_bottom - area->gx_rectangle_top + 1);
        uint16_t *buffer = (uint16_t *)canvas->gx_canvas_memory;
        struct lcd_render *render = &lcd->render;
        ULONG offset;

        if (row_size != LCD_WIDTH) {
            render->draw = lcd_draw_by_line;
            render->row_size = row_size * PIXEL_BYTE;
        } else {       
            render->draw = lcd_draw_by_dmabuf;
            render->row_size = LCD_WIDTH * PIXEL_BYTE;
        }

        offset = area->gx_rectangle_top * canvas->gx_canvas_x_resolution +
                 area->gx_rectangle_left;
        render->buffer = buffer + offset;
        render->remain = row_size * col_size * PIXEL_BYTE;
        render->lcd = lcd;
        
        render->rect.gx_rectangle_left = area->gx_rectangle_left +
                                       canvas->gx_canvas_display_offset_x;
        render->rect.gx_rectangle_right = area->gx_rectangle_right +
                                        canvas->gx_canvas_display_offset_x;
        render->rect.gx_rectangle_top = area->gx_rectangle_top +
                                      canvas->gx_canvas_display_offset_y;           
        render->rect.gx_rectangle_bottom = area->gx_rectangle_bottom +
                                      canvas->gx_canvas_display_offset_y;
        
        compiler_barrier();
        render->state = lcd_render_state_ready;
    }
}
    

static void lcd_sync_render(struct lcd_driver *lcd)
{
    struct lcd_render *render = &lcd->render;
    
    if (render->state == lcd_render_state_ready)
        render->state(render);
}

static void lcd_display_update(struct GX_CANVAS_STRUCT *canvas, GX_RECTANGLE *area)
{
    struct lcd_driver *lcd = canvas->gx_canvas_display->gx_display_driver_data;
    struct lcd_render *render = &lcd->render;
    GX_EVENT event;
    int ret;

#if IS_ENABLED(CONFIG_LCD_DOUBLE_BUFFER)
    k_spinlock_key_t key;
    key = k_spin_lock(&render->lock);
    
    if (render->state == lcd_render_state_idle) {
        lcd_dbg("LCD display start P0(%d, %d) P1(%d, %d)\n", 
            area->gx_rectangle_left, area->gx_rectangle_top, 
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
    if (render->state == lcd_render_state_idle) {
        lcd_dbg("LCD display start P0(%d, %d) P1(%d, %d)\n", 
            area->gx_rectangle_left, area->gx_rectangle_top, 
            area->gx_rectangle_right, area->gx_rectangle_bottom);
        lcd_renderer_init(lcd, canvas, area);
        ret = k_sem_take(&render->sem, render->timeout);
        if (ret)
            goto reinit;
    } else {
        printk("%s LCD controller occurred error\n", __func__);
    }
#endif /* CONFIG_LCD_DOUBLE_BUFFER */
    return;

reinit:
    printk("%s LCD render timeout(state: 0x%p)\n", __func__, render->state);
    lcd_hardware_reset(lcd);
    lcd_setup_commands(lcd, lcd_command, ARRAY_SIZE(lcd_command));
    lcd_write_cmd(lcd->spi, 0x29, NULL, 0, false);
    render->state = lcd_render_state_idle;

    event.gx_event_type = GX_EVENT_REDRAW;
    event.gx_event_target = NULL;
    gx_system_event_fold(&event);  
}


//static void lcd_te_isr(const struct device *dev, struct gpio_callback *cb,
//    gpio_port_pins_t pins)
static void lcd_te_isr(int pin, void *params)
{
    struct lcd_driver *lcd = params;
    lcd_sync_render(lcd);
    (void) pin;
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

	am_hal_mspi_interrupt_clear(lcd->spi, 
		AM_HAL_MSPI_INT_CQUPD|AM_HAL_MSPI_INT_ERR);
	am_hal_mspi_interrupt_enable(lcd->spi, 
		AM_HAL_MSPI_INT_CQUPD|AM_HAL_MSPI_INT_ERR);
	
	return 0;
_err:
	return -EIO;
}


static struct lcd_driver lcd_drv = {
    .render = {
        .state = lcd_render_state_idle,
    },
    .spino    = 0,
    .spi_irq  = MSPI0_IRQn,
    .spi      = NULL,     
    .pin_rst  = 28,
    .pin_te   = 30
};

static int lcd_power_manage(struct observer_base *nb, 
    unsigned long action, void *data)
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

static VOID lcd_canvas_draw_start(struct GX_DISPLAY_STRUCT *display, 
    struct GX_CANVAS_STRUCT *canvas)
{
    struct guix_driver *drv = display->gx_display_driver_data;
    guix_resource_map(drv, ~0u);
}

static VOID lcd_canvas_draw_stop(struct GX_DISPLAY_STRUCT *display, 
    struct GX_CANVAS_STRUCT *canvas)
{
    struct guix_driver *drv = display->gx_display_driver_data;
    guix_resource_unmap(drv, NULL);
}

static UINT lcd_driver_setup(GX_DISPLAY *display)
{
#ifdef CONFIG_RGB565_SWAP_BYTEORDER
    extern VOID guix_rgb565_display_driver_swap_byteorder(GX_DISPLAY *display);
#endif

    struct lcd_driver *lcd = &lcd_drv;
    int ret;

    if (display->gx_display_driver_data)
        return GX_TRUE;

    lcd->gpio = device_get_binding("GPIO_0");
    if (!lcd->gpio)
        return -ENODEV;

    ret = gpio_pin_configure(lcd->gpio, lcd->pin_rst,
        GPIO_OUTPUT_LOW);
    if (ret)
        goto exit;

    k_sem_init(&lcd->render.sem, 0, 1);
    
    /* Setup SPI interface */
    if (lcd_setup_spi(lcd))
        goto exit;

    /* Reset LCD chip */
    lcd_hardware_reset(lcd);

    /* Configure LCD */
    ret = lcd_setup_commands(lcd, lcd_command, ARRAY_SIZE(lcd_command));
    if (ret)
        goto exit;

    ret = gpio_apollo3p_request_irq(lcd->gpio, lcd->pin_te, lcd_te_isr, lcd,
         GPIO_INT_TRIG_LOW, true);
    if (ret)
        goto exit;

    /* Display ON */
    lcd_write_cmd(lcd->spi, 0x29, NULL, 0, false);
    lcd->render.timeout = K_MSEC(100);

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
void *disp_driver_data = &lcd_drv; //TODO:This is ugly!!!

