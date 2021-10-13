#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_apollo3p);

#include <errno.h>
#include <device.h>
#include <drivers/spi.h>
#include <soc.h>

#include "spi/spi_context.h"

struct spi_apollo3p_config {
    int devno;
    int irq;
    int priority;
};

struct spi_apollo3p_data {
    struct spi_context ctx;
    void *handle;
    int  devno;
    size_t chunk_len;
 #ifdef CONFIG_SPI_APOLLO3P_INTERRUPT
    uint32_t dbuf[128];
 #endif
};

#define DT_DRV_COMPAT ambiq_apollo3p_spi

static const uint32_t spi_insmask[] = {
    0x00000000, 0x000000FF, 0x0000FFFF, 0x00FFFFFF, 0xFFFFFFFF
};

#ifdef CONFIG_SPI_APOLLO3P_INTERRUPT
static void spi_apollo3p_isr(const void *parameter)
{
    struct spi_apollo3p_data *data = (struct spi_apollo3p_data *)parameter;
    uint32_t status = IOMn(data->devno)->INTSTAT;
  
    IOMn(data->devno)->INTCLR = status;
    am_hal_iom_interrupt_service(data->handle, status);  
}
#endif

static int spi_apollo3p_configure(const struct device *dev,
			     const struct spi_config *config)
{
	const struct spi_apollo3p_config *cfg = dev->config;
	struct spi_apollo3p_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	am_hal_iom_config_t iom_cfg;
	int ret;

	if (spi_context_configured(ctx, config))
		return 0;

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER)
		return -ENOTSUP;

	/* Set master mode, disable mode fault detection, set fixed peripheral
	 * select mode.
	 */
	iom_cfg.eInterfaceMode = AM_HAL_IOM_SPI_MODE;
    iom_cfg.eSpiMode = AM_HAL_IOM_SPI_MODE_0;
    if (config->operation & SPI_MODE_CPHA)
		iom_cfg.eSpiMode |= AM_HAL_IOM_SPI_MODE_1;

	if (config->operation & SPI_MODE_CPOL)
		iom_cfg.eSpiMode |= AM_HAL_IOM_SPI_MODE_2;

    if (config->frequency == AM_HAL_IOM_1MHZ ||
        config->frequency == AM_HAL_IOM_2MHZ ||
        config->frequency == AM_HAL_IOM_3MHZ ||
        config->frequency == AM_HAL_IOM_4MHZ ||
        config->frequency == AM_HAL_IOM_6MHZ ||
        config->frequency == AM_HAL_IOM_8MHZ ||
        config->frequency == AM_HAL_IOM_12MHZ ||
        config->frequency == AM_HAL_IOM_16MHZ ||
        config->frequency == AM_HAL_IOM_24MHZ) {
        iom_cfg.ui32ClockFreq = config->frequency;
    } else {
        return -ENOTSUP;
    }

#ifdef CONFIG_SPI_APOLLO3P_INTERRUPT
    iom_cfg.ui32NBTxnBufLength = data->dbuf;
    iom_cfg.pNBTxnBuf = ARRAY_SIZE(data->dbuf);
#else
    iom_cfg.ui32NBTxnBufLength = 0;
    iom_cfg.pNBTxnBuf = NULL;
#endif
    ret = am_hal_iom_initialize(cfg->devno, &data->handle);
    if (ret)
        goto exit;

    ret = am_hal_iom_power_ctrl(data->handle, AM_HAL_SYSCTRL_WAKE, false);
    if (ret)
        goto exit;

    am_hal_iom_disable(data->handle);
    ret = am_hal_iom_configure(data->handle, &iom_cfg);
    if (ret)
        goto exit;

    ret = am_hal_iom_enable(data->handle);
    if (ret)
        goto exit;

#ifdef CONFIG_SPI_APOLLO3P_INTERRUPT
    am_hal_iom_interrupt_clear(data->handle,  AM_HAL_IOM_INT_ALL);
    ret = irq_connect_dynamic(cfg->irq, cfg->priority, spi_apollo3p_isr,
        data, 0);
    if (ret)
        goto exit;

    am_hal_iom_interrupt_enable(data->handle, AM_HAL_IOM_INT_CQUPD);
    irq_enable(cfg->irq);
#endif
    ctx->config = config;
    spi_context_cs_configure(ctx);
exit:
	return ret;
}

#ifdef CONFIG_SPI_APOLLO3P_INTERRUPT
static void spi_transfer_next(struct spi_apollo3p_data *data,
            struct spi_context *ctx);

static void spi_transfer_done(void *ctx, uint32_t status)
{
    struct spi_apollo3p_data *data = ctx;

    spi_context_update_tx(ctx, 1, data->chunk_len);
    spi_context_update_rx(ctx, 1, data->chunk_len);
    spi_transfer_next(data, &data->ctx);
}

static void spi_transfer_next(struct spi_apollo3p_data *data,
            struct spi_context *ctx)
{
    size_t chunk_len;
    int error = 0;

    chunk_len = spi_context_max_continuous_chunk(ctx);
    if (chunk_len > 0) {
        am_hal_iom_transfer_t xfer;
        uint32_t ret;

        data->chunk_len = chunk_len;
        
        if (spi_context_tx_buf_on(ctx)) {
            xfer.pui32TxBuffer = (uint32_t *)ctx->tx_buf;
            xfer.eDirection = AM_HAL_IOM_TX;
        } else if (spi_context_rx_buf_on(ctx)) {
            xfer.pui32RxBuffer = (uint32_t *)ctx->rx_buf;
            xfer.eDirection = AM_HAL_IOM_RX;
        } else {
            goto _exit;
        }
    
        xfer.ui32NumBytes = chunk_len;
        xfer.ui32InstrLen = 0;
        xfer.ui32Instr = 0;
        xfer.uPeerInfo.ui32SpiChipSelect = 0;
        xfer.bContinue = false;
        xfer.ui8RepeatCount = 0;
        xfer.ui32PauseCondition = 0;
        xfer.ui32StatusSetClr = 0;
        ret = am_hal_iom_nonblocking_transfer(data->handle, &xfer, 
            spi_transfer_done, data);
        if (ret == AM_HAL_STATUS_SUCCESS)
            return;

        error = -EIO;
    }

_exit:
    spi_context_cs_control(ctx, false);
    LOG_DBG("Transaction finished with status %d", error);
    spi_context_complete(ctx, error);  
}

#else /* !CONFIG_SPI_APOLLO3P_INTERRUPT */
static void spi_transfer_next(struct spi_apollo3p_data *data,
            struct spi_context *ctx)
{
    am_hal_iom_transfer_t xfer;
    uint32_t ins_len;
    int error = 0;

    if (ctx->rx_count == 2 && !ctx->rx_buf) {
        ins_len = ctx->tx_len - 1;
        xfer.eDirection = AM_HAL_IOM_RX;
        xfer.ui32Instr = *(uint32_t *)ctx->tx_buf & spi_insmask[ins_len];
        xfer.ui32InstrLen = ins_len;
        xfer.pui32RxBuffer = (uint32_t *)ctx->current_rx[1].buf;
        xfer.pui32TxBuffer = NULL;
        xfer.ui32NumBytes = ctx->current_rx[1].len;
    } else if (ctx->tx_count) {
        xfer.eDirection = AM_HAL_IOM_TX;
        xfer.pui32RxBuffer = NULL;
        if (ctx->tx_count == 2) {
            ins_len = ctx->tx_len;
            xfer.ui32Instr = *(uint32_t *)ctx->tx_buf & spi_insmask[ins_len];
            xfer.ui32InstrLen = ins_len;
            xfer.pui32TxBuffer = (uint32_t *)ctx->current_tx[1].buf;
            xfer.ui32NumBytes = ctx->current_tx[1].len;
        } else if (ctx->tx_count == 1) {
            xfer.ui32InstrLen = 0;
            xfer.ui32Instr = 0; 
            xfer.pui32TxBuffer = (uint32_t *)ctx->tx_buf;
            xfer.ui32NumBytes = ctx->tx_len;
        } else {
            error = -EINVAL;
            goto _out;
        }
    } else {
            error = -EINVAL;
            goto _out;
    }
  
    xfer.uPeerInfo.ui32SpiChipSelect = 0;
    xfer.bContinue = false;
    xfer.ui8RepeatCount = 0;
    xfer.ui32PauseCondition = 0;
    xfer.ui32StatusSetClr = 0;
    error = am_hal_iom_blocking_transfer(data->handle, &xfer);
    if (error) {
        LOG_DBG("Transaction finished with status %d", error);
    }

_out:    
    spi_context_complete(ctx, error);
}
#endif /* CONFIG_SPI_APOLLO3P_INTERRUPT */

static int spi_apollo3p_transceive(const struct device *dev,
                const struct spi_config *config,
                const struct spi_buf_set *tx_bufs,
                const struct spi_buf_set *rx_bufs)
{
	struct spi_apollo3p_data *data = dev->data;
	int err;

	spi_context_lock(&data->ctx, false, NULL, config);

	err = spi_apollo3p_configure(dev, config);
	if (err != 0)
		goto done;

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, 1);
    spi_context_cs_control(&data->ctx, true);
    
    spi_transfer_next(data, &data->ctx);

	spi_context_cs_control(&data->ctx, false);
    err = spi_context_wait_for_completion(&data->ctx);

done:
	spi_context_release(&data->ctx, err);
	return err;
}

static int spi_apollo3p_transceive_sync(const struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	return spi_apollo3p_transceive(dev, config, tx_bufs, rx_bufs);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_apollo3p_transceive_async(const struct device *dev,
				     const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs,
				     struct k_poll_signal *async)
{
	/* TODO: implement asyc transceive */
	return -ENOTSUP;
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_apollo3p_release(const struct device *dev,
			   const struct spi_config *config)
{
	struct spi_apollo3p_data *data = dev->data;
    
	if (!spi_context_configured(&data->ctx, config))
		return -EINVAL;
        
	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static int spi_apollo3p_init(const struct device *dev)
{
	struct spi_apollo3p_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static const struct spi_driver_api spi_apollo3p_driver_api = {
	.transceive = spi_apollo3p_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_apollo3p_transceive_async,
#endif
	.release = spi_apollo3p_release,
};

#define SPI_APOLLO3P_DEFINE_CONFIG(n)					\
	static const struct spi_apollo3p_config spi_apollo3p_config_##n = {	\
	    .devno = DT_INST_LABEL(n)[4] - '0', \
	    .irq = DT_IRQ(DT_NODELABEL(spi##n), irq),  \
	    .priority = DT_IRQ(DT_NODELABEL(spi##n), priority), \
	}

#define SPI_APOLLO3P_DEVICE_INIT(n)						\
	SPI_APOLLO3P_DEFINE_CONFIG(n);					\
	static struct spi_apollo3p_data spi_apollo3p_dev_data_##n = {		\
		SPI_CONTEXT_INIT_LOCK(spi_apollo3p_dev_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_apollo3p_dev_data_##n, ctx),	\
	};								\
	DEVICE_DEFINE(spi_apollo3p_##n,				\
			    DT_INST_LABEL(n),				\
			    &spi_apollo3p_init, NULL, &spi_apollo3p_dev_data_##n,	\
			    &spi_apollo3p_config_##n, POST_KERNEL,		\
			    CONFIG_SPI_INIT_PRIORITY, &spi_apollo3p_driver_api);


DT_INST_FOREACH_STATUS_OKAY(SPI_APOLLO3P_DEVICE_INIT)
