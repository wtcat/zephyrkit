#ifndef ZEPHYR_DRIVERS_SENSOR_MMC5603_MMC5603_H_
#define ZEPHYR_DRIVERS_SENSOR_MMC5603_MMC5603_H_

#include <zephyr/types.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>

#ifdef __cplusplus
extern "C"{
#endif

/*
 * Register address map
 */
#define MMC5603_XOUT0    0x00
#define MMC5603_XOUT1    0x01
#define MMC5603_YOUT0    0x02
#define MMC5603_YOUT1    0x03
#define MMC5603_ZOUT0    0x04
#define MMC5603_ZOUT1    0x05
#define MMC5603_XOUT2    0x06
#define MMC5603_YOUT2    0x07
#define MMC5603_ZOUT2    0x08
#define MMC5603_TOUT     0x09
#define MMC5603_STATUS1 0x18
#define MMC5603_ODR      0x1A
#define MMC5603_ICTRL0   0x1B
#define MMC5603_ICTRL1   0x1C
#define MMC5603_ICTRL2   0x1D
#define MMC5603_ST_X_TH 0x1E
#define MMC5603_ST_Y_TH 0x1F
#define MMC5603_ST_Z_TH 0x20
#define MMC5603_ST_X     0x27
#define MMC5603_ST_Y     0x28
#define MMC5603_ST_Z     0x29
#define MMC5603_ID       0x39

struct device;

/*
 * MMC5603 data 
 */
struct mmc5603_data {
    const struct device *i2c_master;
    uint32_t power_state;
	int sample_x;
	int sample_y;
	int sample_z;
	int sensitivity;
    
    uint16_t odr;
    uint8_t ictrl0;
    uint8_t ictrl2;

    /*
     * For performance(not structure)
     */
    int (*read)(struct mmc5603_data *priv, uint8_t addr, 
        uint8_t *value, uint8_t len);
    int (*write)(struct mmc5603_data *priv, uint8_t addr, 
        const uint8_t *value, uint8_t len);
};

struct mmc5603_config {
    const char *i2c_name;
    const struct device *pwren;
    int pin;
    int polarity;
};

int mmc5603_interface_init(const struct device *dev);

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_DRIVERS_SENSOR_MMC5603_MMC5603_H_ */
