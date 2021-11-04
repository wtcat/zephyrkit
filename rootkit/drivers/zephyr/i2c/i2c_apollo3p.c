
#include <errno.h>
#include <sys/util.h>

#include <version.h>
#include <kernel.h>
#include <device.h>
#include <pm/device.h>
#include <soc.h>
#include <drivers/i2c.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_apollo3p);

#include "i2c/i2c-priv.h"
#include "i2c/i2c-message.h"



#if (KERNEL_VERSION_NUMBER >= ZEPHYR_VERSION(2,6,0))
#define device_pm_cb pm_device_cb
#define DEVICE_PM_SET_POWER_STATE  PM_DEVICE_STATE_SET
#define DEVICE_PM_GET_POWER_STATE  PM_DEVICE_STATE_GET
#define DEVICE_PM_ACTIVE_STATE     PM_DEVICE_STATE_ACTIVE
#define DEVICE_PM_LOW_POWER_STATE  PM_DEVICE_STATE_LOW_POWER
#define DEVICE_PM_SUSPEND_STATE    PM_DEVICE_STATE_SUSPEND
#define DEVICE_PM_FORCE_SUSPEND_STATE PM_DEVICE_STATE_FORCE_SUSPEND
#endif

#define I2C_DMABUF_SIZE (2*26)

struct i2c_apollo3p_config {
    uint32_t bitrate;
    int irq;
    int priority;
};

struct i2c_apollo3p_data {
#ifdef CONFIG_I2C_APOLLO3P_INTERRUPT
    uint32_t txbuf[I2C_DMABUF_SIZE];
#endif
    struct k_mutex mutex;
#ifdef CONFIG_I2C_APOLLO3P_INTERRUPT
	struct k_sem   sync;
    uint32_t status;
#endif
    void *handle;
    int  devno;
};


#ifdef CONFIG_I2C_APOLLO3P_INTERRUPT
static void i2c_apollo3p_isr(const void *params)
{
    struct i2c_apollo3p_data *data = (struct i2c_apollo3p_data *)params;
    uint32_t status = IOMn(data->devno)->INTSTAT;
  
    IOMn(data->devno)->INTCLR = status;
    am_hal_iom_interrupt_service(data->handle, status);        
}

static void i2c_apollo3p_transfer_cb(void *context, uint32_t status)
{
    struct i2c_apollo3p_data *data = context;
    k_sem_give(&data->sync);
    (void) status;
}

static int i2c_wait_for_completion(struct i2c_apollo3p_data *data, 
	am_hal_iom_transfer_t *xfer)
{
	int ret;

	ret = am_hal_iom_nonblocking_transfer(i2c->dev, 
		xfer, i2c_apollo3p_transfer_cb, i2c);
	if (ret == 0) {
        ret = z_impl_k_sem_take(&data->sync, K_MSEC(3000));
        if (ret)
            LOG_ERR("Error***: i2c(%d) transfer timeout\n", data->devno);
    }
	return ret;
}

static void i2c_transfer_complete(struct i2c_apollo3p_data *data, int status)
{
    data->status = status;
    k_sem_give(&data->sync);
}
#endif /* CONFIG_I2C_APOLLO3P_INTERRUPT */

static int i2c_apollo3p_configure(const struct device *dev,
    uint32_t dev_config)
{
    struct i2c_apollo3p_data *data = dev->data;
    am_hal_iom_config_t iomcfg = {0};
    int ret = 0;

    if (!(dev_config & I2C_MODE_MASTER))
        return -EINVAL;

#ifdef CONFIG_I2C_APOLLO3P_INTERRUPT
    iomcfg.ui32NBTxnBufLength = I2C_DMABUF_SIZE;
    iomcfg.pNBTxnBuf = data->txbuf;
#endif

    iomcfg.eInterfaceMode = AM_HAL_IOM_I2C_MODE;
    iomcfg.eSpiMode = AM_HAL_IOM_SPI_MODE_0;
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
        iomcfg.ui32ClockFreq = AM_HAL_IOM_100KHZ;
		break;
	case I2C_SPEED_FAST:
		iomcfg.ui32ClockFreq = AM_HAL_IOM_400KHZ;
		break;
	case I2C_SPEED_FAST_PLUS:
		iomcfg.ui32ClockFreq = AM_HAL_IOM_1MHZ;
		break;
	default:
		iomcfg.ui32ClockFreq = AM_HAL_IOM_400KHZ;
        break;
	}

    k_mutex_lock(&data->mutex, K_FOREVER);
    if (am_hal_iom_initialize(data->devno, &data->handle)) {
        LOG_ERR("Error***: iom-i2c%d initialize failed\n", data->devno);
        ret = -EINVAL;
        goto exit;
    }

    if (am_hal_iom_power_ctrl(data->handle, AM_HAL_SYSCTRL_WAKE, false)) {
        LOG_ERR("Error***: iom-i2c%d power on failed\n", data->devno);
        ret = -EINVAL;
        goto exit;
    }

    am_hal_iom_disable(data->handle);
    if (am_hal_iom_configure(data->handle, &iomcfg)) {
        LOG_ERR("Error***: iom-i2c%d configure failed\n", data->devno);
        ret = -EINVAL;
        goto exit;
    }

    if (am_hal_iom_enable(data->handle)) {
        LOG_ERR("Error***: iom-i2c%d enable failed\n", data->devno);
        ret = -EINVAL;
        goto exit;
    }

#ifdef CONFIG_I2C_APOLLO3P_INTERRUPT
    am_hal_iom_interrupt_clear(data->handle,  AM_HAL_IOM_INT_ALL);
    am_hal_iom_interrupt_enable(data->handle, AM_HAL_IOM_INT_CQUPD);
#endif
exit:
    k_mutex_unlock(&data->mutex);
    return ret;
}

static int i2c_apollo3p_transfer(const struct device *dev, struct i2c_msg *msgs,
    uint8_t num_msgs, uint16_t addr)
{
    struct i2c_apollo3p_data *data = dev->data;
    am_hal_iom_transfer_t xfer;
    uint8_t index;
    int ret = -EINVAL;

    k_mutex_lock(&data->mutex, K_FOREVER);
    xfer.bContinue = false;
    xfer.ui8RepeatCount  = 0;
    xfer.ui32PauseCondition = 0;
    xfer.ui32StatusSetClr = 0;
    
    for (index = 0; index < num_msgs; index++) {
        struct i2c_msg *xmsg = msgs + index;
        struct i2c_message *m = CONTAINER_OF(xmsg, 
            struct i2c_message, msg); 

        xfer.uPeerInfo.ui32I2CDevAddr = addr;
        xfer.ui32InstrLen = m->reg_len;
        xfer.ui32Instr = m->reg_addr;
#ifdef CONFIG_I2C_APOLLO3P_INTERRUPT        
        if (xmsg->flags & I2C_MSG_READ) {
            xfer.eDirection      = AM_HAL_IOM_TX;
            xfer.ui32NumBytes    = 0;
            xfer.pui32TxBuffer   = NULL;
            ret = am_hal_iom_nonblocking_transfer(data->handle, 
                &xfer, NULL, NULL);
            if (ret == 0) {
                xfer.ui32InstrLen   = 0;
                xfer.ui32Instr      = 0;
                xfer.eDirection     = AM_HAL_IOM_RX;
                xfer.ui32NumBytes   = xmsg->len;
                xfer.pui32RxBuffer  = (uint32_t *)xmsg->buf;
                ret = i2c_wait_for_completion(data, &xfer);
            } 
            
        } else {
            xfer.eDirection      = AM_HAL_IOM_TX;
            xfer.ui32NumBytes    = xmsg->len;
            xfer.pui32TxBuffer   = (uint32_t *)xmsg->buf;
            ret = i2c_wait_for_completion(data, &xfer);
        }
#else
        if (xmsg->flags & I2C_MSG_READ) {
            xfer.eDirection     = AM_HAL_IOM_RX;
            xfer.ui32NumBytes   = xmsg->len;
            xfer.pui32RxBuffer  = (uint32_t *)xmsg->buf;
            ret = am_hal_iom_blocking_transfer(data->handle, &xfer);
        } else {
            xfer.eDirection      = AM_HAL_IOM_TX;
            xfer.ui32NumBytes    = xmsg->len;
            xfer.pui32TxBuffer   = (uint32_t *)xmsg->buf;
            ret = am_hal_iom_blocking_transfer(data->handle, &xfer);
        }

#endif /* CONFIG_I2C_APOLLO3P_INTERRUPT */
        if (ret)
            break;
    }

    k_mutex_unlock(&data->mutex);
    return ret;
}

static int i2c_apollo3p_transfer_compatible(const struct device *dev, 
    struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr)
{
    struct i2c_apollo3p_data *data = dev->data;
    am_hal_iom_transfer_t xfer;
    uint8_t index;
    bool ready;
    int ret = -EINVAL;

    k_mutex_lock(&data->mutex, K_FOREVER);
    xfer.bContinue = false;
    xfer.ui8RepeatCount  = 0;
    xfer.ui32PauseCondition = 0;
    xfer.ui32StatusSetClr = 0;
    ready = false;
    
    for (index = 0; index < num_msgs; index++) {
        struct i2c_msg *xmsg = msgs + index;

        if (!(xmsg->flags & I2C_MSG_STOP)) {
            if (ready) {
                ret = -EINVAL;
                break;
            }
            xfer.ui32InstrLen = xmsg->len;
            xfer.ui32Instr = (uint32_t)*xmsg->buf; /* support only 1 byte  */
            ready = true;
            continue;
        }

        xfer.uPeerInfo.ui32I2CDevAddr = addr;
#ifdef CONFIG_I2C_APOLLO3P_INTERRUPT        
        if (xmsg->flags & I2C_MSG_READ) {
            xfer.eDirection      = AM_HAL_IOM_TX;
            xfer.ui32NumBytes    = 0;
            xfer.pui32TxBuffer   = NULL;
            ret = am_hal_iom_nonblocking_transfer(data->handle, 
                &xfer, NULL, NULL);
            if (ret == 0) {
                xfer.ui32InstrLen   = 0;
                xfer.ui32Instr      = 0;
                xfer.eDirection     = AM_HAL_IOM_RX;
                xfer.ui32NumBytes   = xmsg->len;
                xfer.pui32RxBuffer  = (uint32_t *)xmsg->buf;
                ret = i2c_wait_for_completion(data, &xfer);
            } 
            
        } else {
            xfer.eDirection      = AM_HAL_IOM_TX;
            xfer.ui32NumBytes    = xmsg->len;
            xfer.pui32TxBuffer   = (uint32_t *)xmsg->buf;
            ret = i2c_wait_for_completion(data, &xfer);
        }
#else
        if (xmsg->flags & I2C_MSG_READ) {
            xfer.eDirection     = AM_HAL_IOM_RX;
            xfer.ui32NumBytes   = xmsg->len;
            xfer.pui32RxBuffer  = (uint32_t *)xmsg->buf;
            ret = am_hal_iom_blocking_transfer(data->handle, &xfer);
        } else {
            xfer.eDirection      = AM_HAL_IOM_TX;
            xfer.ui32NumBytes    = xmsg->len;
            xfer.pui32TxBuffer   = (uint32_t *)xmsg->buf;
            ret = am_hal_iom_blocking_transfer(data->handle, &xfer);
        }

#endif /* CONFIG_I2C_APOLLO3P_INTERRUPT */
        if (ret)
            break;
        
        ready = false;
    }

    k_mutex_unlock(&data->mutex);
    return ret;
}

static int i2c_apollo3p_init(const struct device *dev)
{
    const struct i2c_apollo3p_config *cfg = dev->config;
    struct i2c_apollo3p_data *data = dev->data;
    uint32_t bitrate_cfg;

#ifdef CONFIG_I2C_APOLLO3P_INTERRUPT 
    k_sem_init(&data->sync, 0, 1);
#endif
    k_mutex_init(&data->mutex);

#ifdef CONFIG_I2C_APOLLO3P_INTERRUPT
    irq_connect_dynamic(cfg->irq, cfg->priority, i2c_apollo3p_isr, data, 0);
    irq_enable(cfg->irq);
#endif
    bitrate_cfg = i2c_map_dt_bitrate(cfg->bitrate);
    return i2c_apollo3p_configure(dev, I2C_MODE_MASTER|bitrate_cfg);
}

static const struct i2c_driver_api i2c_apollo3p_driver_api = {
	.configure = i2c_apollo3p_configure,
	.transfer = i2c_apollo3p_transfer,
};

static const struct i2c_driver_api i2c_apollo3p_compatible_driver_api = {
    .configure = i2c_apollo3p_configure,
    .transfer = i2c_apollo3p_transfer_compatible,
};


/*
#define DEVICE_PM_ACTIVE_STATE          1
#define DEVICE_PM_LOW_POWER_STATE       2
#define DEVICE_PM_SUSPEND_STATE         3
#define DEVICE_PM_FORCE_SUSPEND_STATE	4
#define DEVICE_PM_OFF_STATE             5
*/

#ifdef CONFIG_PM_DEVICE

static int i2c_pm_control(const struct device *dev, 
                        uint32_t command,
#if KERNEL_VERSION_NUMBER >= ZEPHYR_VERSION(2,6,0)
                        uint32_t *context,
#else
                        void *context, 
#endif
                        pm_device_cb cb, 
                        void *arg)
{
    uint32_t state = *(uint32_t *)context;
    if (command == DEVICE_PM_SET_POWER_STATE) {
        switch (state) {
        case DEVICE_PM_ACTIVE_STATE:
            break;
        case DEVICE_PM_LOW_POWER_STATE:
        case DEVICE_PM_SUSPEND_STATE:
        case DEVICE_PM_FORCE_SUSPEND_STATE:
            break;
        }
    } else {

    }
    return -ENOTSUP;
}
#else
#define i2c_pm_control NULL
#endif /* CONFIG_PM_DEVICE */

#if defined(i2c_pm_control)
#error "#####"
#endif


#define I2C_APOLLO3P(name, _driver_api) \
	struct i2c_apollo3p_data i2c##name##_apollo3p_data = { \
	    .devno = name, \
	}; \
	struct i2c_apollo3p_config i2c##name##_apollo3p_cfg = { \
	    .bitrate = 400000, \
	    .irq = DT_IRQ(DT_NODELABEL(i2c##name), irq), \
	    .priority = DT_IRQ(DT_NODELABEL(i2c##name), priority), \
	}; \
	DEVICE_DEFINE(i2c##name##_apollo3p, \
		            DT_LABEL(DT_NODELABEL(i2c##name)), \
			    i2c_apollo3p_init, \
                i2c_pm_control, \
			    &i2c##name##_apollo3p_data, \
			    &i2c##name##_apollo3p_cfg, \
			    POST_KERNEL, \
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			    &_driver_api)

/* I2C0 */
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i2c0), ambiq_apollo3p_i2c, okay)
I2C_APOLLO3P(0, i2c_apollo3p_driver_api);
#endif

/* I2C1 */
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i2c1), ambiq_apollo3p_i2c, okay)
I2C_APOLLO3P(1, i2c_apollo3p_driver_api);
#endif

/* I2C2 */
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i2c2), ambiq_apollo3p_i2c, okay)
I2C_APOLLO3P(2, i2c_apollo3p_driver_api);
#endif

/* I2C3 */
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i2c3), ambiq_apollo3p_i2c, okay)
I2C_APOLLO3P(3, i2c_apollo3p_driver_api);
#endif

/* I2C4 */
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i2c4), ambiq_apollo3p_i2c, okay)
I2C_APOLLO3P(4, i2c_apollo3p_driver_api);
#endif

/* I2C5 */
#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(i2c5), ambiq_apollo3p_i2c, okay)
I2C_APOLLO3P(5, i2c_apollo3p_compatible_driver_api);
#endif

