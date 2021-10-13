#include <soc.h>
#include <init.h>


static void *spi_handle;

static int sdk_spi_init(void)
{
    am_hal_iom_config_t spi_config = {
        .eInterfaceMode = AM_HAL_IOM_SPI_MODE,
        .ui32ClockFreq  = AM_HAL_IOM_1MHZ,
        .eSpiMode = AM_HAL_IOM_SPI_MODE_3,
        .pNBTxnBuf = NULL,
        .ui32NBTxnBufLength = 0
    };

    //am_hal_gpio_pinconfig(16, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_output_set(16);
    
    //am_bsp_iom_pins_enable(spi_gpio_mode,AM_HAL_IOM_SPI_MODE);

    if (am_hal_iom_initialize(4, &spi_handle))
		return AM_HAL_STATUS_HW_ERR;
       
	if (am_hal_iom_power_ctrl(spi_handle, AM_HAL_SYSCTRL_WAKE, false))
		return AM_HAL_STATUS_HW_ERR;

	if (am_hal_iom_configure(spi_handle, &spi_config))
		return AM_HAL_STATUS_HW_ERR;

    am_hal_iom_disable(spi_handle);
	if (am_hal_iom_enable(spi_handle))
		return AM_HAL_STATUS_HW_ERR;
    
    return AM_HAL_STATUS_SUCCESS;
}

static int sdk_spi_read(uint8_t addr, void *buf, uint32_t data_len)
{
	am_hal_iom_transfer_t xfer;
    
	xfer.ui32InstrLen = 1;
	xfer.ui32Instr = addr;
	xfer.pui32TxBuffer = NULL;
	xfer.pui32RxBuffer = (uint32_t *)buf;
	xfer.ui32NumBytes = data_len;
    xfer.eDirection = AM_HAL_IOM_RX;
	xfer.bContinue = false;
	xfer.ui8Priority = 0;
	xfer.ui8RepeatCount = 0;
	xfer.ui32PauseCondition = 0;
	xfer.uPeerInfo.ui32SpiChipSelect = 0;
	xfer.ui32StatusSetClr = 0;
	return (int)am_hal_iom_blocking_transfer(spi_handle, &xfer);
}

static int lis2dh_hw_test(const struct device *dev)
{
    ARG_UNUSED(dev);
    uint8_t id = 0;
    
    sdk_spi_init();
    if (!spi_handle)
        return -EINVAL;

    am_util_delay_ms(10);
    if (sdk_spi_read(0x8F, &id, 1))
        return -EIO;

    if (id != 0x33)
        return -EINVAL;

    return 0;
}

SYS_INIT(lis2dh_hw_test, POST_KERNEL,
    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT+6);

