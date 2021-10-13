#ifndef ZEPHYR_MSPI_APOLLO3P_H_
#define ZEPHYR_MSPI_APOLLO3P_H_

#include <soc.h>

#ifdef __cplusplus
extern "C"{
#endif

struct mspi_device;

struct mspi_device *mspi_apollo3p_alloc(int devno);

int mspi_apollo3p_configure(struct mspi_device *dev, 
    const am_hal_mspi_dev_config_t *cfg);

void mspi_apollo3p_release(struct mspi_device *dev);

int mspi_apollo3p_transfer_poll(struct mspi_device *dev, 
    am_hal_mspi_pio_transfer_t *xfer);

int mspi_apollo3p_transfer(struct mspi_device *dev, 
    am_hal_mspi_dma_transfer_t *xfer);

void *mspi_apollo3p_xip_set(struct mspi_device *dev, bool enable);


#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_MSPI_APOLLO3P_H_ */
