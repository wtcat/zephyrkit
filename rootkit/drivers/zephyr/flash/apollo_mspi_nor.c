/*
 * For qspi flash (mx25u512g) 
 */

#include <errno.h>
#include <drivers/flash.h>
#include <drivers/gpio.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <logging/log.h>

//#include "flash/flash_priv.h"
#include "misc/mspi_apollo3p.h"

LOG_MODULE_REGISTER(mspi_nor, CONFIG_FLASH_LOG_LEVEL);
#define DT_DRV_COMPAT gd_mspi_nor

#if defined(CONFIG_MX25U256)
#define MX25_JEDEC_RDID     0xC23925
#define MX25_JEDEC_RDID_QPI 0x3925C2
#elif defined(CONFIG_MX25U512)
#define MX25_JEDEC_RDID     0xC23A25
#define MX25_JEDEC_RDID_QPI 0x3A25C2
#else
#error "Wrong flash chid select!"
#endif

/* Write Operations */
#define WRITE_ENABLE_CMD                     0x06
#define WRITE_DISABLE_CMD                    0x04

/* qspi mode */
#define ENTER_QPI_MODE_CMD                   0x35
#define EXIT_QPI_MODE_CMD                    0xf5

/* qspi reset */
#define ENABLE_RESET                         0x66
#define RESET_DEV                            0x99

/* Register Operations */
#define READ_STATUS_REG_1_CMD                0x05
#define READ_STATUS_REG_2_CMD                0x15
#define READ_STATUS_REG_3_CMD                0x2B
#define WRITE_STATUS_REG_1_CMD               0x01
#define WRITE_STATUS_REG_3_CMD               0x2F //

/* Read Operations */
#define READ_CMD                             0x03
#define FAST_READ_CMD                        0x0B
#define DUAL_OUT_READ_CMD                    0x3B
#define DUAL_INOUT_READ_CMD                  0xBB
#define QUAD_OUT_READ_CMD                    0x6B
#define QUAD_INOUT_READ_CMD                  0xEB

/* Program Operations */
#define PAGE_PROG_CMD                        0x02
#define QPAGE_PROG_CMD                       0x32
     
/* Erase Operations */
#define SECTOR_ERASE_CMD                     0x21 //4K
#define SUBBLOCK_ERASE_CMD                   0x5C //32K
#define BLOCK_ERASE_CMD                      0xDC //64K
#define CHIP_ERASE_CMD                       0x60
#define CHIP_ERASE_CMD_2                     0xC7
#define PROG_ERASE_RESUME_CMD                0x7A
#define PROG_ERASE_SUSPEND_CMD               0x75

/* Power Down Operations */
#define POWER_DOWN_CMD                       0xB9
#define RELEASE_POWER_DOWN_CMD               0xAB


#define GD25LQ256_4B_ENABLE     			0xB7
#define GD25LQ256_4B_DISABLE     			0xE9

/* qspi ID */
#define READ_QPI_ID_CMD                   	0xAF
#define READ_SPI_ID_CMD                     0x9F


#define STATUS_WIP			(1<<0)
#define STATUS_WEL			(1<<1)
#define STATUS_BP0			(1<<2)
#define STATUS_BP1			(1<<3)
#define STATUS_BP2			(1<<4)
#define STATUS_BP3			(1<<5)
#define STATUS_QE			(1<<6)
#define STATUS_SRWD			(1<<7)

#define STATUS_ODS_0		(1<<0)
#define STATUS_ODS_1		(1<<1)
#define STATUS_ODS_2		(1<<2)
#define STATUS_TB			(1<<3)
#define STATUS_PBE			(1<<4)
#define STATUS_4B			(1<<5)
#define STATUS_DC0			(1<<6)
#define STATUS_DC1			(1<<7)

#define FLASH_WBLK_SIZE (256)
#define FLASH_PAGE_SIZE (4 * 1024ul)

#if defined(CONFIG_MX25U256)
#define FLASH_SIZE (32 * 1024 * 1024ul)
#elif defined(CONFIG_MX25U512)
#define FLASH_SIZE (64 * 1024 * 1024ul)
#else
#error not support!
#endif
#define FLASH_DATA_AREA_SIZE (10 * 1024 * 1024)
#define FLASH_FILE_AREA_SIZE (FLASH_SIZE - FLASH_DATA_AREA_SIZE)

enum mspi_transfer_dir {
    MSPI_RX = AM_HAL_MSPI_RX,
    MSPI_TX = AM_HAL_MSPI_TX
};

struct mspi_nor_priv {
    struct k_mutex mutex;
    struct mspi_device *mspi;
    const struct device *en;
};

struct mspi_nor_config {
    int devno;
    uint32_t page_size;
    uint32_t wpage_size;
    uint32_t start;
    uint32_t size;
};


static int mspi_send_cmd(struct mspi_nor_priv *data, uint16_t cmd, 
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
    return mspi_apollo3p_transfer_poll(data->mspi, &xfer);
}
	
static int mspi_transfer_data(struct mspi_nor_priv *data, uint32_t addr, 
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
    return mspi_apollo3p_transfer(data->mspi, &xfer);
}

static uint32_t nor_end(const struct mspi_nor_config *cfg)
{
    return cfg->start + cfg->size;
}

static uint32_t mspi_nor_read_id(struct mspi_nor_priv *data, uint16_t id_cmd)
{
    uint32_t id = 0;
    mspi_send_cmd(data, id_cmd, &id, 3, MSPI_RX);
    return id;
}

static int mspi_nor_wait_operation(struct mspi_nor_priv *data)
{
    int timeout = 50;
    uint8_t val = 0xFF;

    do {
        mspi_send_cmd(data, READ_STATUS_REG_1_CMD, &val, 1, MSPI_RX);
        if (!(val & STATUS_WIP))
            return 0;
        k_sleep(K_MSEC(1));
    } while (--timeout > 0);
    return -EIO;
}

static int mspi_nor_erase_page(struct mspi_nor_priv *data, uint32_t addr)
{
    uint8_t buf[4];
    int ret;

    buf[0] = ((addr & 0xFF000000) >> 24);
    buf[1] = ((addr & 0xFF0000) >> 16);
    buf[2] = ((addr & 0xFF00) >> 8);
    buf[3] = ((addr & 0xFF));
    ret = mspi_send_cmd(data, SECTOR_ERASE_CMD, buf, 4, MSPI_TX);
    if (ret)
        goto _out;

    ret = mspi_nor_wait_operation(data);
    if (ret)
        goto _out;

    ret = mspi_send_cmd(data, WRITE_DISABLE_CMD, NULL, 0, MSPI_TX);
_out:
    return ret;
}

static int mspi_nor_read(const struct device *dev, off_t addr, 
	void *dest, size_t size)
{
    const struct mspi_nor_config *cfg = dev->config;
    struct mspi_nor_priv *data = dev->data;
    uint32_t offset = addr + cfg->start;
    int ret;

    if (!dest)
        return -EINVAL;

    if (offset < cfg->start ||
        offset + size > nor_end(cfg)) {
        LOG_ERR("flash read address(0x%x) is invalid\n", (uint32_t)addr);
        return -EINVAL;
    }

    k_mutex_lock(&data->mutex, K_FOREVER);
    ret = mspi_transfer_data(data, offset, dest, size, MSPI_RX);
    k_mutex_unlock(&data->mutex);
    return ret;
}

static int mspi_nor_write(const struct device *dev, off_t addr,
	const void *src, size_t size)
{
    const struct mspi_nor_config *cfg = dev->config;
    struct mspi_nor_priv *data = dev->data;
    uint32_t offset = addr + cfg->start;
    char *psrc = (char *)src;
    size_t wr_bytes;
    int ret = 0;

    if (!src)
        return -EINVAL;

    if (offset & (cfg->wpage_size - 1))
        return -EINVAL;

    if (offset < cfg->start ||
        offset + size > nor_end(cfg)) {
        LOG_ERR("flash write address(0x%x) is invalid\n", (uint32_t)addr);
        return -EINVAL;
    }

    k_mutex_lock(&data->mutex, K_FOREVER);
    while (size > 0) {
        ret = mspi_send_cmd(data, WRITE_ENABLE_CMD, NULL, 0, MSPI_TX);
        if (ret)
            break;

        wr_bytes = MIN(size, cfg->wpage_size);
        ret = mspi_transfer_data(data, offset, psrc, wr_bytes, MSPI_TX);
        if (ret)
            break;
        ret = mspi_nor_wait_operation(data);
        if (ret)
            break;

        psrc += wr_bytes;
        offset += wr_bytes;
        size -= wr_bytes;
    }

    k_mutex_unlock(&data->mutex);
    return ret;
}

static int mspi_nor_erase(const struct device *dev, off_t addr, 
	size_t size)
{
    const struct mspi_nor_config *cfg = dev->config;
    struct mspi_nor_priv *data = dev->data;
    uint32_t offset = addr + cfg->start;
    int ret = 0;

    if (offset & (cfg->wpage_size - 1))
        return -EINVAL;

    if (size % cfg->page_size)
        return -EINVAL;

    if (offset < cfg->start ||
        offset + size > nor_end(cfg)) {
        LOG_ERR("flash erase address(0x%x) is invalid\n", (uint32_t)addr);
        return -EINVAL;
    }

    k_mutex_lock(&data->mutex, K_FOREVER);
    while (size > 0) {
        ret = mspi_send_cmd(data, WRITE_ENABLE_CMD, NULL, 0, MSPI_TX);
        if (ret)
            break;
        ret = mspi_nor_erase_page(data, offset);
        if (ret)
            break;

        offset += cfg->page_size;
        size -= cfg->page_size;
    }
    k_mutex_unlock(&data->mutex);
    return ret;
}

static int mspi_nor_write_protection_set(const struct device *dev,
	bool write_protect)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(write_protect);
    return 0;
}

static  const struct flash_parameters *mspi_flash_get_parameters(
	const struct device *dev)
{
    static const struct flash_parameters parameters = {
        .write_block_size = FLASH_WBLK_SIZE,
        .erase_value = 0xFF
    };
    ARG_UNUSED(dev);
    return &parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static struct flash_pages_layout dev_layout;
static void mspi_nor_pages_layout(const struct device *dev,
	const struct flash_pages_layout **layout, size_t *layout_size)
{
    *layout_size = 1;
    *layout = &dev_layout;
}
#endif

static const struct flash_driver_api qspi_nor_api = {
    .read = mspi_nor_read,
    .write = mspi_nor_write,
    .erase = mspi_nor_erase,
    .write_protection = mspi_nor_write_protection_set,
    .get_parameters = mspi_flash_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
    .page_layout = mspi_nor_pages_layout,
#endif
};


static uint32_t mspi_dma_buffer[2048];
static const am_hal_mspi_dev_config_t serial_mspi_ce0_cfg = {
    .eSpiMode             = AM_HAL_MSPI_SPI_MODE_0,
    .eClockFreq           = AM_HAL_MSPI_CLK_48MHZ,
    .ui8TurnAround        = 8,
    .eAddrCfg             = AM_HAL_MSPI_ADDR_4_BYTE,
    .eInstrCfg            = AM_HAL_MSPI_INSTR_1_BYTE,
    .eDeviceConfig        = AM_HAL_MSPI_FLASH_SERIAL_CE0,
    .bSeparateIO          = true,
    .bSendInstr           = true,
    .bSendAddr            = true,
    .bTurnaround          = true,
    .ui8ReadInstr         = 0,
    .ui8WriteInstr        = 0,
#ifndef AM_PART_APOLLO3
    .ui8WriteLatency      = 0,
    .bEnWriteLatency      = false,
    .bEmulateDDR          = false,
    .ui16DMATimeLimit     = 0,
    .eDMABoundary         = AM_HAL_MSPI_BOUNDARY_NONE,
#endif
    .ui32TCBSize          = ARRAY_SIZE(mspi_dma_buffer),
    .pTCB                 = mspi_dma_buffer,
    .scramblingStartAddr  = 0,
    .scramblingEndAddr    = 0,
};

static const am_hal_mspi_dev_config_t quad_mspi_ce0_cfg = {
    .eSpiMode             = AM_HAL_MSPI_SPI_MODE_0,
    .eClockFreq           = AM_HAL_MSPI_CLK_48MHZ,
    .ui8TurnAround        = 6,  //
    .eAddrCfg             = AM_HAL_MSPI_ADDR_4_BYTE,
    .eInstrCfg            = AM_HAL_MSPI_INSTR_1_BYTE,
    .eDeviceConfig        = AM_HAL_MSPI_FLASH_QUAD_CE0,
    .bSeparateIO          = false,
    .bSendInstr           = true,
    .bSendAddr            = true,
    .bTurnaround          = true,
    .ui8ReadInstr         = 0xEC,//0x0B,
    .ui8WriteInstr        = 0X12,//0x02,
#ifndef AM_PART_APOLLO3
    .ui8WriteLatency      = 0,
    .bEnWriteLatency      = false,
    .bEmulateDDR          = false,
    .ui16DMATimeLimit     = 0,
    .eDMABoundary         = AM_HAL_MSPI_BOUNDARY_NONE,
#endif
    .ui32TCBSize          = ARRAY_SIZE(mspi_dma_buffer),
    .pTCB                 = mspi_dma_buffer,
    .scramblingStartAddr  = 0,
    .scramblingEndAddr    = 0,
};

static struct mspi_nor_priv nor_data;

/*
 * Flash xip mode for GUI
 */
#ifdef CONFIG_GUIX
#include "gx_api.h"

static void *flash_xip_mmap(size_t size)
{
    struct mspi_nor_priv *data = &nor_data;
    void *ptr;

    ARG_UNUSED(size);
    k_mutex_lock(&data->mutex, K_FOREVER);
    ptr = mspi_apollo3p_xip_set(data->mspi, true);
    return ptr;
}

static void flash_xip_ummap(void *ptr)
{
    struct mspi_nor_priv *data = &nor_data;

    ARG_UNUSED(ptr);
    (void)mspi_apollo3p_xip_set(data->mspi, false);
    k_mutex_unlock(&data->mutex);
}

static struct gxres_device gxres_norflash = {
    .dev_name = "norflash",
    .link_id = 0,
    .mmap = flash_xip_mmap,
    .unmap = flash_xip_ummap
};
#endif

static int mspi_nor_init(const struct device *dev)
{
    const struct mspi_nor_config *cfg = dev->config;
    struct mspi_nor_priv *data = dev->data;
    static bool ready = false;
    uint8_t buf[4];
    uint32_t id;
    int pin, ret = 0;

    k_mutex_init(&data->mutex);
    k_mutex_lock(&data->mutex, K_FOREVER);
    data->mspi = mspi_apollo3p_alloc(cfg->devno);
    if (!data->mspi) {
        LOG_ERR("Allocate mspi device faile: device number(%d)\n", cfg->devno);
        ret = -EINVAL;
        goto out;
    }

    if (ready)
        goto out;

    pin = soc_get_pin("flash_en");
    if (pin < 0) {
        LOG_ERR("flash_en pin is not found\n");
        return -EINVAL;
    }
    data->en = device_get_binding(pin2name(pin));
    if (!data->en) {
        LOG_ERR("Device not found: %s\n", pin2name(pin));
        return -EINVAL;
    }

    ret = gpio_pin_configure(data->en, pin2gpio(pin), GPIO_OUTPUT_LOW);
    if (ret < 0) {
        LOG_ERR("GPIO(%d) configure failed\n", pin);
        return ret;
    }
    
    /* Enable flash power and wait it get stable */
    gpio_pin_set(data->en, pin2gpio(pin), 1);
    k_busy_wait(2000);

    /* Configure 1-wire mode */
    ret = mspi_apollo3p_configure(data->mspi, &serial_mspi_ce0_cfg);
    if (ret)
        goto out;

    id = mspi_nor_read_id(data, READ_SPI_ID_CMD);
    if (id == MX25_JEDEC_RDID) {
        /* Switch to 4-wire mode */
        mspi_send_cmd(data, WRITE_ENABLE_CMD, NULL, 0, MSPI_TX);
        buf[0] = STATUS_QE;
        buf[1] = 0x00;
        mspi_send_cmd(data, WRITE_STATUS_REG_1_CMD, buf, 2, MSPI_TX);
        mspi_nor_wait_operation(data);
        mspi_send_cmd(data, WRITE_ENABLE_CMD, NULL, 0, MSPI_TX);
        mspi_send_cmd(data, ENTER_QPI_MODE_CMD, NULL, 0, MSPI_TX);
        mspi_nor_wait_operation(data);
    }
    mspi_apollo3p_release(data->mspi);

    /* Configure 4-wires mode */
    ret = mspi_apollo3p_configure(data->mspi, &quad_mspi_ce0_cfg);
    if (ret)
        goto out;

    mspi_send_cmd(data, READ_STATUS_REG_2_CMD, buf, 1, MSPI_RX);
    id = mspi_nor_read_id(data, READ_QPI_ID_CMD);
    if (id != MX25_JEDEC_RDID_QPI ) {
        ret = -ENOTSUP;
        goto out;
    }

    ret |= mspi_send_cmd(data, GD25LQ256_4B_ENABLE, NULL, 0, MSPI_TX);
    ret |= mspi_send_cmd(data, WRITE_DISABLE_CMD, NULL, 0, MSPI_TX);
    ret |= mspi_send_cmd(data, READ_STATUS_REG_1_CMD, buf, 1, MSPI_RX);
    ret |= mspi_send_cmd(data, READ_STATUS_REG_2_CMD, buf, 1, MSPI_RX);
    if (!ret)
        ready = true;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	dev_layout.pages_count = FLASH_SIZE/FLASH_PAGE_SIZE;
	dev_layout.pages_size = FLASH_PAGE_SIZE;
#endif
out:
    k_mutex_unlock(&data->mutex);
#ifdef CONFIG_GUIX
    gxres_device_register(&gxres_norflash);
#endif
    return ret;
}

static const struct mspi_nor_config nor_config = {
    .devno = 1,
    .page_size = FLASH_PAGE_SIZE,
    .wpage_size = FLASH_WBLK_SIZE,
    .start = 0,
    .size = FLASH_SIZE
};
DEVICE_DT_DEFINE(DT_DRV_INST(0), mspi_nor_init, NULL,
		 &nor_data, &nor_config,
		 POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		 &qspi_nor_api);

