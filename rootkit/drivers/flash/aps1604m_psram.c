#include <errno.h>
#include <drivers/gpio.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(psram);

#include "misc/mspi_apollo3p.h"

/*
 * Operation commands
 */
#define CMD_READ             0x03
#define CMD_FAST_READ        0x0B
#define CMD_FAST_QREAD       0xEB
#define CMD_WRITE            0x02
#define CMD_QWRITE           0x38
#define CMD_WRAPPED_READ     0x8B
#define CMD_WRAPPED_WRITE    0x82
#define CMD_MR_READ          0xB5
#define CMD_MR_WRITE         0xB1
#define CMD_ENTER_QMODE      0x35
#define CMD_EXIT_QMODE       0xF5
#define CMD_RST_ENABLE       0x66
#define CMD_RESET            0x99
#define CMD_BURST_LEN_TOGGLE 0xC0
#define CMD_READ_ID          0x9F

#define PSRAM_SID 0x0d5d


struct mspi_psram {
    struct mspi_device *spi;
    const struct device *en;
    int devno;
};

enum mspi_transfer_dir {
    MSPI_RX = AM_HAL_MSPI_RX,
    MSPI_TX = AM_HAL_MSPI_TX
};


static uint32_t mspi_dma_buffer[2048];
static const am_hal_mspi_dev_config_t serial_mspi_ce0_cfg = {
    .eSpiMode             = AM_HAL_MSPI_SPI_MODE_0,
    .eClockFreq           = AM_HAL_MSPI_CLK_24MHZ,
    .ui8TurnAround        = 8,
    .eAddrCfg             = AM_HAL_MSPI_ADDR_3_BYTE,
    .eInstrCfg            = AM_HAL_MSPI_INSTR_1_BYTE,
    .eDeviceConfig        = AM_HAL_MSPI_FLASH_SERIAL_CE0,
    .bSeparateIO          = true,
    .bSendInstr           = true,
    .bSendAddr            = true,
    .bTurnaround          = true,
    .ui8ReadInstr         = CMD_FAST_READ,
    .ui8WriteInstr        = CMD_WRITE,
#ifndef AM_PART_APOLLO3
    .ui8WriteLatency      = 0,
    .bEnWriteLatency      = false,
    .bEmulateDDR          = false,
    .ui16DMATimeLimit     = 80,
    .eDMABoundary         = AM_HAL_MSPI_BOUNDARY_BREAK1K,//AM_HAL_MSPI_BOUNDARY_NONE,
#endif
    .ui32TCBSize          = ARRAY_SIZE(mspi_dma_buffer),
    .pTCB                 = mspi_dma_buffer,
    .scramblingStartAddr  = 0,
    .scramblingEndAddr    = 0,
};

static const am_hal_mspi_dev_config_t quad_mspi_ce0_cfg = {
    .eSpiMode             = AM_HAL_MSPI_SPI_MODE_0,
    .eClockFreq           = AM_HAL_MSPI_CLK_48MHZ,
    .eXipMixedMode        = AM_HAL_MSPI_XIPMIXED_NORMAL,
    .ui8TurnAround        = 6,  //
    .eAddrCfg             = AM_HAL_MSPI_ADDR_3_BYTE,
    .eInstrCfg            = AM_HAL_MSPI_INSTR_1_BYTE,
    .eDeviceConfig        = AM_HAL_MSPI_FLASH_QUAD_CE0,
    .bSeparateIO          = false,
    .bSendInstr           = true,
    .bSendAddr            = true,
    .bTurnaround          = true,
    .ui8ReadInstr         = CMD_FAST_QREAD,
    .ui8WriteInstr        = CMD_QWRITE,
#ifndef AM_PART_APOLLO3
    .ui8WriteLatency      = 0,
    .bEnWriteLatency      = false,
    .bEmulateDDR          = false,
    .ui16DMATimeLimit     = 30,
    .eDMABoundary         = AM_HAL_MSPI_BOUNDARY_BREAK1K,
#endif
    .ui32TCBSize          = ARRAY_SIZE(mspi_dma_buffer),
    .pTCB                 = mspi_dma_buffer,
    .scramblingStartAddr  = 0,
    .scramblingEndAddr    = 0,
};


static int psram_send_cmd(struct mspi_psram *data, uint16_t cmd, 
	void *buf, size_t size, enum mspi_transfer_dir dir)
{
    am_hal_mspi_pio_transfer_t xfer = {0};
    if (dir == MSPI_RX) {
        xfer.eDirection = AM_HAL_MSPI_RX;
        xfer.bTurnaround = true;
    } else {
        xfer.eDirection = AM_HAL_MSPI_TX;
        xfer.bTurnaround = false;        
    }
    xfer.ui32NumBytes       = size;
    xfer.bScrambling        = false;
    xfer.bSendAddr          = false;
    xfer.ui32DeviceAddr     = 0;
    xfer.bSendInstr         = true;
    xfer.ui16DeviceInstr    = cmd;
    xfer.bContinue          = false;
    xfer.bQuadCmd           = false;
    xfer.pui32Buffer        = (uint32_t *)buf;
    return mspi_apollo3p_transfer_poll(data->spi, &xfer);
}
	
static int psram_transfer_data(struct mspi_psram *data, uint32_t addr, 
	void *buf, size_t size, enum mspi_transfer_dir dir)
{
    am_hal_mspi_dma_transfer_t xfer;

    xfer.ui8Priority = 1;
    xfer.eDirection = dir;
    xfer.ui32TransferCount = size;
    xfer.ui32DeviceAddress = addr;
    xfer.ui32SRAMAddress = (uint32_t)buf;
    xfer.ui32PauseCondition = 0;
    xfer.ui32StatusSetClr = 0;	
    return mspi_apollo3p_transfer(data->spi, &xfer);
}

static int psram_read_id(struct mspi_psram *priv)
{
    uint8_t idbuf[12] = {0};
    if (psram_transfer_data(priv, 0, idbuf, 8, MSPI_RX)) {
        LOG_ERR("%s(): PSRAM read dummy failed\n", __func__);
        return -EIO;
    }
    if (psram_send_cmd(priv, CMD_READ_ID, (void *)idbuf, 12, MSPI_RX)) {
        LOG_ERR("%s(): PSRAM read id failed\n", __func__);
        return -EIO;
    }
    return ((uint16_t)idbuf[2] << 8) | idbuf[3];
}

static bool psram_enter_qmode(struct mspi_psram *priv)
{
    return psram_send_cmd(priv, CMD_ENTER_QMODE, NULL, 0, MSPI_TX) == 0;
}

static bool __unused psram_exit_qmode(struct mspi_psram *priv)
{
    return psram_send_cmd(priv, CMD_EXIT_QMODE, NULL, 0, MSPI_TX) == 0;
}

static void *psram_enter_xip(struct mspi_psram *priv)
{
    char *ptr = mspi_apollo3p_xip_set(priv->spi, true);
    return ptr + 0x50000000;
}

static void __unused psram_xip_ummap(struct mspi_psram *priv)
{
    (void)mspi_apollo3p_xip_set(priv->spi, true);
}

static bool psram_verify(struct mspi_psram *priv)
{
    char s1[] = {"Hello,World\n"};
    char buffer[32] = {0};
    char *ram;

    ram = psram_enter_xip(priv);
    memcpy(ram, s1, sizeof(s1));
    memcpy(buffer, ram, sizeof(s1));
    if (memcmp(buffer, ram, sizeof(s1)))
        return false;
    return true;
}

static struct mspi_psram psram_driver = {
    .devno = 2
};

static int psram_device_init(const struct device *dev)
{
    struct mspi_psram *priv = &psram_driver;
    int pin, id;
    int ret = 0;
    
    priv->spi = mspi_apollo3p_alloc(priv->devno);
    if (!priv->spi) {
        LOG_ERR("Allocate mspi device faile: device number(%d)\n", priv->devno);
        ret = -EINVAL;
        goto _out;
    }

    pin = soc_get_pin("psram_en");
    if (pin < 0) {
        LOG_ERR("psram_en pin is not found\n");
        return -EINVAL;
    }
    
    priv->en = device_get_binding(pin2name(pin));
    if (!priv->en) {
        LOG_ERR("Device not found: %s\n", pin2name(pin));
        return -EINVAL;
    }
    
    ret = gpio_pin_configure(priv->en, pin2gpio(pin), GPIO_OUTPUT_LOW);
    if (ret < 0) {
        LOG_ERR("GPIO(%d) configure failed\n", pin);
        return ret;
    }

    /* Configure 1-wire mode */
    ret = mspi_apollo3p_configure(priv->spi, &serial_mspi_ce0_cfg);
    if (ret)
        goto _out;

    /* Enable flash power and wait it get stable */
    gpio_pin_set(priv->en, pin2gpio(pin), 1);
    k_busy_wait(1000);

    id = psram_read_id(priv);
    if (id != PSRAM_SID) {
        LOG_ERR("%s: PSRAM device id error: %x\n", __func__, id);
        ret = -EIO;
        goto _out;
    }

    /* Enter Quad-SPI mode */
    if (!psram_enter_qmode(priv)) {
        LOG_ERR("%s(): Send command failed\n", __func__);
        ret = -EIO;
        goto _out;
    }

    /* Configure 4-wire mode */
    mspi_apollo3p_release(priv->spi);
    ret = mspi_apollo3p_configure(priv->spi, &quad_mspi_ce0_cfg);
    if (ret)
        goto _out;

    /* Enter XIPMM mode */
    psram_enter_xip(priv);

    /* Check it */
    if (!psram_verify(priv)) {
        LOG_ERR("%s(): PSRAM read/write failed\n", __func__);
        return -EIO;
    }

    extern void __psram_section_init(void);
    __psram_section_init();
_out:
    return ret;
}

SYS_INIT(psram_device_init, POST_KERNEL, 
    CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
