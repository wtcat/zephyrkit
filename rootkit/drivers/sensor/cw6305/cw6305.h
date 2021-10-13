#ifndef ZEPHYR_DRIVERS_SENSOR_CW6305_H_
#define ZEPHYR_DRIVERS_SENSOR_CW6305_H_

/*
 * CW6305 Register address
 */
#define CHG_REG_VERSION      0x00
#define CHG_REG_VBUS_VOLT    0x01
#define CHG_REG_VBUS_CUR     0x02
#define CHG_REG_CHG_VOLT     0x03
#define CHG_REG_CHG_CUR1     0x04
#define CHG_REG_CHG_CUR2     0x05
#define CHG_REG_CHG_SAFETY   0x06
#define CHG_REG_CONFIG       0x07
#define CHG_REG_SAFETY_CFG   0x08
#define CHG_REG_SAFETY_CFG2  0x09
#define CHG_REG_INT_SET      0x0A
#define CHG_REG_INT_SRC      0x0B
#define CHG_REG_IC_STATUS    0x0D
#define CHG_REG_IC_STATUS2   0x0E


/*  CHG_REG_IC_STATUS */
#define CW6305_CHRG_BIT   0x20

/*
 * Interrupt sources
 */
#define CHARGE_INT_PRECHG_TIMER  0x01 /* Pre-Charge Timer Expiration */
#define CHARGE_INT_SAFETY_TIMER  0x02 /* Safety Timer Expiration */
#define CHARGE_INT_BAT_UT        0x04 /* Battery Under Temperature */
#define CHARGE_INT_BAT_OT        0x08 /* Battery Over Temperature */
#define CHARGE_INT_CHG_OV        0x10 /* Charger Over-Voltage */
#define CHARGE_INT_CHG_FINISH    0x20 /* Charging finished */
#define CHARGE_INT_CHG_REMOVE    0x40 /* Adapter Unplug */
#define CHARGE_INT_CHG_DET       0x80 /* Valid Adapter Detected on VBUS */

#endif /* ZEPHYR_DRIVERS_SENSOR_CW6305_H_ */