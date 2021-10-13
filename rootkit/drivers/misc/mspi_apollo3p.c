#include <errno.h>
#include <kernel.h>
#include <soc.h>

#include "mspi_apollo3p.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(mspi, CONFIG_MSPI_LOG_LEVEL);

struct mspi_mmap {
    uint32_t start;
    uint32_t end;
};

struct mspi_device {
    struct k_mutex mutex;
    struct k_sem sync;
    const struct mspi_mmap *mmap;
    void *handle;
    int devno;
    int error;
    bool initialized;
    bool xip;
    int irq;
};

#define MSPI_DEVICE_DEFINE(_inst) \
    static struct mspi_mmap CONCAT(mspi_mmap_, _inst) = { \
        .start = 0x02000000 * (_inst + 1),                \
        .end =   0x02000000 * (_inst + 1) + 0x1FFFFFFF  \
    }; \
    static struct mspi_device CONCAT(mspi_hdev_, _inst) = {       \
        .mutex = Z_MUTEX_INITIALIZER(CONCAT(mspi_hdev_, _inst).mutex),    \
        .sync = Z_SEM_INITIALIZER(CONCAT(mspi_hdev_, _inst).sync, 0, 1), \
        .mmap = &CONCAT(mspi_mmap_, _inst), \
        .handle = NULL, \
        .devno = _inst, \
        .initialized = false, \
        .xip = false, \
        .irq = MSPI##_inst##_IRQn \
    }


#ifdef CONFIG_APOLLO3P_MSPI0
MSPI_DEVICE_DEFINE(0);
#endif
#ifdef CONFIG_APOLLO3P_MSPI1
MSPI_DEVICE_DEFINE(1);
#endif
#ifdef CONFIG_APOLLO3P_MSPI2
MSPI_DEVICE_DEFINE(2);
#endif

static void mspi_apollo3p_isr(const void *parameter)
{
    struct mspi_device *dev = (struct mspi_device *)parameter;
    uint32_t status = MSPIn(dev->devno)->INTSTAT;

    MSPIn(dev->devno)->INTCLR = status;
    am_hal_mspi_interrupt_service(dev->handle, status);	
}

static void mspi_apollo3p_dma_cb(void *context, uint32_t status)
{
    struct mspi_device *dev = context;
    ARG_UNUSED(status);
	
    if (status & AM_HAL_MSPI_INT_ERR)
        dev->error = -EIO;
    else
        dev->error = 0;
    k_sem_give(&dev->sync);
}

static int mspi_apollo3p_wait_for_completion(struct mspi_device *dev)
{
    int ret;
    ret = k_sem_take(&dev->sync, K_MSEC(1000));
    return dev->error? dev->error: ret;
}

int mspi_apollo3p_transfer_poll(struct mspi_device *dev, 
	am_hal_mspi_pio_transfer_t *xfer)
{
    int ret;
    k_mutex_lock(&dev->mutex, K_FOREVER);
    if (dev->xip) {
        ret = -EBUSY;
        goto _unlock;
    }
    ret = am_hal_mspi_blocking_transfer(dev->handle, xfer, 100000);
_unlock:
    k_mutex_unlock(&dev->mutex);
    return ret;
}

int mspi_apollo3p_transfer(struct mspi_device *dev, 
	am_hal_mspi_dma_transfer_t *xfer)
{
    int ret;
    k_mutex_lock(&dev->mutex, K_FOREVER);
    if (dev->xip) {
        ret = -EBUSY;
        goto _unlock;
    }
    
    ret = am_hal_mspi_nonblocking_transfer(dev->handle, xfer, 
            AM_HAL_MSPI_TRANS_DMA, mspi_apollo3p_dma_cb, dev);                                  
    if (!ret)
        ret = mspi_apollo3p_wait_for_completion(dev);
_unlock:
    k_mutex_unlock(&dev->mutex);
    return ret;
}

struct mspi_device *mspi_apollo3p_alloc(int devno)
{
#ifdef CONFIG_APOLLO3P_MSPI0
    if (devno == 0)
        return &CONCAT(mspi_hdev_, 0);
#endif
#ifdef CONFIG_APOLLO3P_MSPI1
    if (devno == 1)
        return &CONCAT(mspi_hdev_, 1);
#endif
#ifdef CONFIG_APOLLO3P_MSPI2
    if (devno == 2)
        return &CONCAT(mspi_hdev_, 2);
#endif
    return NULL;
}

int mspi_apollo3p_configure(struct mspi_device *dev, 
	const am_hal_mspi_dev_config_t *cfg)
{
    int ret;

    if (dev->initialized)
        return 0;

    k_mutex_lock(&dev->mutex, K_FOREVER);
#if AM_APOLLO3_MCUCTRL
    am_hal_mcuctrl_control(AM_HAL_MCUCTRL_CONTROL_FAULT_CAPTURE_ENABLE, 0);
#else
    am_hal_mcuctrl_fault_capture_enable();
#endif
    ret = am_hal_mspi_initialize(dev->devno, &dev->handle);
    if (ret) {
        LOG_ERR("Initialize mspi(%d) failed with error %d\n", 
        dev->devno, ret);
        goto _err;
    }

    ret = am_hal_mspi_power_control(dev->handle, AM_HAL_SYSCTRL_WAKE, false);
    if (ret) {
        LOG_ERR("Enable clock for  mspi(%d) failed with error %d\n", 
        dev->devno, ret);
        goto _deinit;
    }

    ret = am_hal_mspi_device_configure(dev->handle, (void *)cfg);
    if (ret) {
        LOG_ERR("Configure  mspi(%d) failed with error %d\n", 
        dev->devno, ret);
        goto _sleep;
    }

    ret = am_hal_mspi_enable(dev->handle);
    if (ret) {
        LOG_ERR("Enable  mspi(%d) failed with error %d\n", 
        dev->devno, ret);
        goto _disable;
    }

    irq_connect_dynamic(dev->irq, 0, mspi_apollo3p_isr, dev, 0);
    am_hal_mspi_interrupt_clear(dev->handle, 
            AM_HAL_MSPI_INT_CQUPD|AM_HAL_MSPI_INT_ERR);
    am_hal_mspi_interrupt_enable(dev->handle, 
            AM_HAL_MSPI_INT_CQUPD|AM_HAL_MSPI_INT_ERR);
    irq_enable(dev->irq);
    dev->initialized = true;
    goto _out;

_disable:
    am_hal_mspi_disable(dev->handle);
_sleep:
    am_hal_mspi_power_control(dev->handle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
_deinit:
    am_hal_mspi_deinitialize(dev->handle);
_err:
    dev->handle = NULL;
_out:
    k_mutex_unlock(&dev->mutex);
    return ret;
}

void mspi_apollo3p_release(struct mspi_device *dev)
{
    if (!dev->initialized)
        return;

    k_mutex_lock(&dev->mutex, K_FOREVER);
    am_hal_mspi_disable(dev->handle);
    irq_disable(dev->irq);
    am_hal_mspi_interrupt_clear(dev->handle, 
    AM_HAL_MSPI_INT_CQUPD|AM_HAL_MSPI_INT_ERR);
    am_hal_mspi_power_control(dev->handle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
    am_hal_mspi_deinitialize(dev->handle);
    dev->handle = NULL;
    dev->initialized = false;
    k_mutex_unlock(&dev->mutex);
}

void *mspi_apollo3p_xip_set(struct mspi_device *dev, bool enable)
{
    if (!dev->initialized)
        return NULL;
        
    k_mutex_lock(&dev->mutex, K_FOREVER); //TODO: Remove it to optimaze speed??
    MSPIn(dev->devno)->FLASH_b.XIPEN = enable;
    dev->xip = enable;
    k_mutex_unlock(&dev->mutex);
    return (void *)dev->mmap->start;
}

