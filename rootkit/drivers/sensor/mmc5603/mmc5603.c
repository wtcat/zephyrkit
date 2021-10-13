#include <string.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>

#include <version.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <logging/log.h>
#include <soc.h>

#include <drivers/sensor.h>
#include "drivers_ext/sensor_priv.h"
#include "mmc5603.h"

LOG_MODULE_REGISTER(MMC5603, CONFIG_SENSOR_LOG_LEVEL);


#if (KERNEL_VERSION_NUMBER >= ZEPHYR_VERSION(2,6,0))
#include <pm/device_runtime.h>
#define _device_pm_get(_dev) pm_device_get(_dev)
#define _device_pm_put(_dev) pm_device_put(_dev)
#define device_pm_enable(_dev) pm_device_enable(_dev)
#define device_pm_cb pm_device_cb

#define DEVICE_PM_SET_POWER_STATE  PM_DEVICE_STATE_SET
#define DEVICE_PM_GET_POWER_STATE  PM_DEVICE_STATE_GET
#define DEVICE_PM_ACTIVE_STATE     PM_DEVICE_STATE_ACTIVE
#define DEVICE_PM_LOW_POWER_STATE  PM_DEVICE_STATE_LOW_POWER
#define DEVICE_PM_SUSPEND_STATE    PM_DEVICE_STATE_SUSPEND
#else
#define _device_pm_get(_dev) device_pm_get_sync(_dev)
#define _device_pm_put(_dev) device_pm_put(_dev)
#endif

#define DT_DRV_COMPAT memsic_mmc5603

/*
 * Chip ID
 */
#define MMC5603_CHIPID 0x10

/*
 * Internal Control Register 0
 */
#define ICTRL0_TAKE_MEAS_M 0x01
#define ICTRL0_TAKE_MEAS_T 0x02
#define ICTRL0_AUTO_SR_EN   0x20
#define ICTRL0_AUTO_ST_EN   0x40
#define ICTRL0_DO_SET    0x08
#define ICTRL0_DO_RESET 0x10
#define ICTRL0_CMM_FREQ_EN 0x80

#define MEAS_MASK (ICTRL0_TAKE_MEAS_M | \
                    ICTRL0_TAKE_MEAS_T | \
                    ICTRL0_AUTO_SR_EN | \
                    ICTRL0_AUTO_ST_EN)


#define ICTRL2_HPOWER 0x80
#define ICTRL2_CMM_EN 0x10

/*
 * Status Register
 */
#define STA_SAT_SENSOR   0x20
#define STA_MEAS_T_DONE 0x80
#define STA_MEAS_M_DONE 0x40

#define MMC5603_ATTR_EXT(n) (SENSOR_ATTR_PRIV_START + n)
    

static inline int mmc5603_read_reg(struct mmc5603_data *priv, uint8_t addr, 
    uint8_t *value)
{
    int ret = priv->read(priv, addr, value, 1);
    if (ret) {
        LOG_ERR("%s(): Read register address(0x%x) failed:%d\n", 
            __func__, addr, ret);
    }
    return 0;
}

static inline int mmc5603_write_reg(struct mmc5603_data *priv, uint8_t addr, 
    uint8_t value)
{
    int ret = priv->write(priv, addr, &value, 1);
    if (ret) {
        LOG_ERR("%s(): Write register address(0x%x) failed: %d\n", 
            __func__, addr, ret);
    }
    return 0;
}

static inline int mmc5603_read_data(struct mmc5603_data *priv, uint8_t addr, 
    uint8_t *value, uint8_t len)
{
    int ret = priv->read(priv, addr, value, len);
    if (ret) {
        LOG_ERR("%s(): Read multi-registers address(0x%x) failed:%d\n", 
            __func__, addr, ret);
    }
    return 0;
}

static inline int mmc5603_do_set(struct mmc5603_data *priv)
{
    return mmc5603_write_reg(priv, MMC5603_ICTRL0, ICTRL0_DO_SET);
}

static inline int mmc5603_do_reset(struct mmc5603_data *priv)
{
    return mmc5603_write_reg(priv, MMC5603_ICTRL0, ICTRL0_DO_RESET);
}

static inline int mmc5603_soft_reset(struct mmc5603_data *priv)
{
    if (mmc5603_write_reg(priv, MMC5603_ICTRL1, 0x80))
        return -EIO;
    k_msleep(30);
    return 0;
}

static inline int mmc5603_read_chipid(struct mmc5603_data *priv)
{
    uint8_t chipid = 0;
    (void) mmc5603_read_reg(priv, MMC5603_ID, &chipid);
    return chipid;
}

static int mmc5603_set_odr(struct mmc5603_data *priv, uint16_t odr, bool auto_set)
{
    /*
     * BW1    BW0   Measurement-Time
     *  0      0    6.6ms
     *  0      1    3.5ms
     *  1      0    2.0ms
     *  1      1    1.2ms
     */
    bool hppower = false;
    uint8_t bw;
    int ret = 0;
    if (auto_set) {
        if (odr <= 75) {
            bw = 0x00;
        } else if (odr <= 150) {
            bw = 0x01;
        } else if (odr <= 255){
            bw = 0x10;
        } else {
            return -EINVAL;
        }
    } else {
        if (odr <= 75) {
            bw = 0x00;
        } else if (odr <= 150) {
            bw = 0x01;
        } else if (odr <= 255){
            bw = 0x10;
        } else if (odr <= 1000){
            odr = (uint16_t)((int)odr * 255 / 1000);
            bw = 0x11;
            hppower = true;
        } else {
            return -EINVAL;
        }
    }

    ret |= mmc5603_write_reg(priv, MMC5603_ICTRL1, bw);
    if (ret)
        return ret;

    if (auto_set) {
        ret |= mmc5603_write_reg(priv, MMC5603_ICTRL0, ICTRL0_AUTO_SR_EN);
        if (!ret)
            priv->ictrl0 |= ICTRL0_AUTO_SR_EN;
    }
    if (hppower) {
        ret |= mmc5603_write_reg(priv, MMC5603_ICTRL2, ICTRL2_HPOWER);
        if (!ret)
            priv->ictrl2 |= ICTRL2_HPOWER;
    }
    ret |= mmc5603_write_reg(priv, MMC5603_ODR, odr);
    if (!ret)
        priv->odr = odr;
    return ret;
}

static int mmc5603_set_continue(struct mmc5603_data *priv)
{
    int ret;
    ret = mmc5603_write_reg(priv, MMC5603_ICTRL0, 
            priv->ictrl0 | ICTRL0_CMM_FREQ_EN);
    if (ret)
        goto _out;

    priv->ictrl0 |= ICTRL0_CMM_FREQ_EN;
    ret = mmc5603_write_reg(priv, MMC5603_ICTRL2, 
            priv->ictrl2 | ICTRL2_CMM_EN);
    if (ret)
        goto _out;

    priv->ictrl2 |= ICTRL2_CMM_EN;
_out:
    return 0;
}

static int mmc5603_enable_measure(struct mmc5603_data *priv, bool continue_en)
{
    /* Continue measure mode */
    if (continue_en) {
        int ret = mmc5603_set_continue(priv);
        if (ret) {
            LOG_ERR("%s(): Enter continue mode failed\n", __func__);
            return ret;
        }
    }
    /* Enable measure */
    return mmc5603_write_reg(priv, MMC5603_ICTRL0, 
        priv->ictrl0 | ICTRL0_TAKE_MEAS_M | ICTRL0_DO_SET);
}

static int mmc5603_disable_measure(struct mmc5603_data *priv)
{
    return mmc5603_write_reg(priv, MMC5603_ICTRL0, ICTRL0_DO_RESET);
}

static int mmc5603_mode_config(const struct device *dev, 
                               struct mmc5603_data *priv,
                               enum sensor_attribute attr, 
                               const struct sensor_value *val)       
{
    int ret = 0;

	switch ((int)attr) {
	case SENSOR_ATTR_FULL_SCALE:
		priv->sensitivity = val->val1;
        break;
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
        priv->odr = (uint16_t)val->val1;
        break;
    case SENSOR_ATTR_START:
        if (priv->odr == 0)
            return -EINVAL;
#ifdef CONFIG_PM_DEVICE
        _device_pm_get(dev);
#endif
        ret = mmc5603_set_odr(priv, priv->odr, true);
        if (!ret)
            ret = mmc5603_enable_measure(priv, true);
        break;
    case SENSOR_ATTR_STOP:
        ret = mmc5603_disable_measure(priv);
#ifdef CONFIG_PM_DEVICE
        ret |= _device_pm_put(dev);
#endif
        break;
	default:
		LOG_DBG("Attribute not supported.");
		return -ENOTSUP;
	}
	return ret;
}

static int mmc5603_attr_set(const struct device *dev,
                            enum sensor_channel chan,
                            enum sensor_attribute attr,
                            const struct sensor_value *val)
{
    struct mmc5603_data *priv = dev->data;
	switch (chan) {
	case SENSOR_CHAN_MAGN_XYZ:
		return mmc5603_mode_config(dev, priv, attr, val);
	default:
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}
	return 0;
}

static inline void mmc5603_convert(struct sensor_value *val, 
    int raw_val, int sensitivity)	    
{
    double fv = (raw_val - 524288) * 0.0625f;
    int32_t iv = (int32_t)fv;
	val->val1 = iv;
	val->val2 = (int32_t)((fv - iv) * 1000000.0f);
}

static inline int mmc5603_get_channel_xyz(enum sensor_channel chan,
                                           struct sensor_value *val,
                                           struct mmc5603_data *priv,
                                           int sensitivity)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		mmc5603_convert(val, priv->sample_x, sensitivity);
		break;
	case SENSOR_CHAN_MAGN_Y:
		mmc5603_convert(val, priv->sample_y, sensitivity);
		break;
	case SENSOR_CHAN_MAGN_Z:
		mmc5603_convert(val, priv->sample_z, sensitivity);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		mmc5603_convert(val, priv->sample_x, sensitivity);
		mmc5603_convert(val + 1, priv->sample_y, sensitivity);
		mmc5603_convert(val + 2, priv->sample_z, sensitivity);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mmc5603_sample_fetch_xyz(const struct device *dev)
{
    struct mmc5603_data *priv = dev->data;
    uint8_t status = 0;
    uint8_t buf[9];

    mmc5603_read_reg(priv, MMC5603_STATUS1, &status);
    if (!(status & STA_MEAS_M_DONE))
        return -EBUSY;
    if (mmc5603_read_data(priv, 0, buf, sizeof(buf))) {
        LOG_WRN("failed to read sample");
        return -EIO;
    }
    priv->sample_x = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | (buf[6] >> 4);
    priv->sample_y = ((uint32_t)buf[2] << 12) | ((uint32_t)buf[3] << 4) | (buf[7] >> 4);
    priv->sample_z = ((uint32_t)buf[4] << 12) | ((uint32_t)buf[5] << 4) | (buf[8] >> 4);
    return 0;
}

static int mmc5603_sample_fetch(const struct device *dev,
    enum sensor_channel chan)
{
    int ret = -EINVAL;
	switch (chan) {
	case SENSOR_CHAN_MAGN_XYZ:
		ret = mmc5603_sample_fetch_xyz(dev);
		break;
#if defined(CONFIG_MMC5603_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
        //TODO:
		break;
#endif
	case SENSOR_CHAN_ALL:
		ret = mmc5603_sample_fetch_xyz(dev);
#if defined(CONFIG_MMC5603_ENABLE_TEMP)
        //TODO:
#endif
		break;
	default:
		return -ENOTSUP;
	}
	return ret;
}

static inline int mmc5603_get_channel(const struct device *dev, 
    enum sensor_channel chan, struct sensor_value *val)
{
    struct mmc5603_data *priv = dev->data;
    return mmc5603_get_channel_xyz(chan, val, priv, priv->sensitivity);
}

static const struct sensor_driver_api mmc5603_driver = {
     .attr_set = mmc5603_attr_set,
     .sample_fetch = mmc5603_sample_fetch,
     .channel_get = mmc5603_get_channel,
};

static int mmc5603_selftest_setup(struct mmc5603_data *priv)
{
    uint8_t st_data[3];
    uint8_t st_val[3];
    int16_t st_thr[3];
    int16_t temp;
    int ret;
    
    ret = mmc5603_read_data(priv, MMC5603_ST_X, st_data, sizeof(st_data));
    if (ret)
        return ret;

    for (int i = 0; i < ARRAY_SIZE(st_data); i++) {
        st_thr[i] = (int16_t)(st_data[i] - 128) * 32;
        if (st_thr[i] < 0) 
            st_thr[i] *= -1;

        temp = st_thr[i] - st_thr[i] / 5;
        temp = temp / 8;
        
        if (temp > 255)
            st_val[i] = 0xFF;
        else
            st_val[i] = (uint8_t)temp;
    }

    ret |= mmc5603_write_reg(priv, MMC5603_ST_X_TH, st_val[0]);
    ret |= mmc5603_write_reg(priv, MMC5603_ST_Y_TH, st_val[1]);
    ret |= mmc5603_write_reg(priv, MMC5603_ST_Z_TH, st_val[2]);
    return ret;
}

static bool mmc5603_selftest_run(struct mmc5603_data *priv)
{
    uint8_t status = 0xFF;
    uint8_t val;

    val = priv->ictrl0 | ICTRL0_AUTO_ST_EN;
    if (mmc5603_write_reg(priv, MMC5603_ICTRL0, val))
        return false;

    k_busy_wait(15000);
    mmc5603_read_reg(priv, MMC5603_STATUS1, &status);
    if (status & STA_SAT_SENSOR)
        return false;
    return true;
}

#ifdef CONFIG_PM_DEVICE
static int mmc5603_pm_set(struct mmc5603_data *priv, uint32_t state)
{
    int ret = 0;

    switch (state) {
    case DEVICE_PM_ACTIVE_STATE:
        priv->power_state = state;
        break;
    case DEVICE_PM_LOW_POWER_STATE:
        break;
    case DEVICE_PM_SUSPEND_STATE:
        ret = mmc5603_soft_reset(priv);
        if (!ret)
            priv->power_state = state;
        break;
    }
    return ret;
}

#if (KERNEL_VERSION_NUMBER < ZEPHYR_VERSION(2,6,0))
static int mmc5603_pm_control(const struct device *dev, uint32_t command, 
    void *context, device_pm_cb cb, void *arg) 
#else
static int mmc5603_pm_control(const struct device *dev, uint32_t command, 
    uint32_t *context, device_pm_cb cb, void *arg) 
#endif
{
    struct mmc5603_data *priv = dev->data;
    uint32_t *state = context;
    int ret;

    switch (command) {
    case DEVICE_PM_SET_POWER_STATE:
        ret = mmc5603_pm_set(priv, *state);
        break;
    case DEVICE_PM_GET_POWER_STATE:
        *state = priv->power_state;
        ret = 0;
        break;
    default:
        ret = -EINVAL;
        break;
    }
    if (cb != NULL)
        cb(dev, ret, context, arg);
    return ret;
}
#else
#define mmc5603_pm_control NULL
#endif

static int mmc5603_init(const struct device *dev)
{
    const struct mmc5603_config *cfg = dev->config;
    struct mmc5603_data *priv = dev->data;
    uint8_t chipid;
    int ret;

    ret = mmc5603_interface_init(dev);
    if (ret) {
        LOG_ERR("%s(): sensor bus initialiaze failed\n", __func__);
        goto _out;
    }
    /* Power up */
    if (cfg->pwren) {
        ret = gpio_pin_configure(cfg->pwren, pin2gpio(cfg->pin), GPIO_OUTPUT_LOW);
        if (ret < 0) {
            LOG_ERR("GPIO(%d) configure failed\n", cfg->pin);
            return ret;
        }
        gpio_pin_set(cfg->pwren, pin2gpio(cfg->pin), cfg->polarity);
        k_busy_wait(2000);
    }
    
    chipid = mmc5603_read_chipid(priv);
    if (chipid != MMC5603_CHIPID) {
        LOG_ERR("%s(): Chip ID is invalid\n", __func__);
        ret = -EINVAL;
        goto _out;
    }
    if (mmc5603_soft_reset(priv)) {
        LOG_ERR("%s(): Soft reset failed\n", __func__);
        ret = -EIO;
        goto _out;
    }
    ret = mmc5603_set_odr(priv, 50, true);
    if (ret) {
        LOG_ERR("%s(): Set sensor ODR failed\n", __func__);
        ret = -EIO;
        goto _out;
    }

    /* Run selftest */
    ret = mmc5603_selftest_setup(priv);
    if (ret) {
        LOG_ERR("%s(): Configure selftest parameters failed\n", __func__);
        ret = -EINVAL;
        goto _out;
    }
    if (!mmc5603_selftest_run(priv)) {
        LOG_ERR("%s(): Selftest failed\n", __func__);
        ret = -EIO;
    }
#ifdef CONFIG_PM_DEVICE
    ret = mmc5603_soft_reset(priv);
    if (!ret)
        device_pm_enable(dev);
#endif
_out:
    return ret;
}

#define SENSOR_INIT(nid) \
    static const struct mmc5603_config mmc5603_config##nid = { \
        .i2c_name = DT_PROP(DT_PARENT(DT_DRV_INST(nid)), label), \
        .pwren = DEVICE_DT_GET(DT_GPIO_CTLR(DT_DRV_INST(nid), pwr_gpios)), \
        .pin = DT_PHA(DT_DRV_INST(nid), pwr_gpios, pin), \
        .polarity = !DT_PHA(DT_DRV_INST(nid), pwr_gpios, flags) \
    }; \
    static struct mmc5603_data mmc5603_private##nid = { \
        .sensitivity = 1000, \
    }; \
    DEVICE_DT_DEFINE(DT_DRV_INST(nid),   \
        mmc5603_init,              \
        mmc5603_pm_control,          \
        &mmc5603_private##nid,                   \
        &mmc5603_config##nid,           \
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
        &mmc5603_driver);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_INIT)
