#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "drivers_ext/sensor_priv.h"
#include "MemsicOri.h"

LOG_MODULE_REGISTER(magnet);

#define SCHDULE_CYCLE 20 /* unit: ms */

#define GSENSOR_NAME "ICM42607"
#define MSENSOR_NAME "MMC5603"

struct magnet_service {
    struct k_timer timer;
    struct k_sem sync;
    struct k_spinlock spin;
    const struct device *m;
    const struct device *g;
    float mag_x;
    float mag_y;
    float mag_z;

    float azimuth;
    float pitch;
    float roll;
    struct sensor_value *m_axis;
    bool ready;
    unsigned int read_errors;
};


static inline void magnet_submit_work(struct magnet_service *msvr)
{
    k_sem_give(&msvr->sync);
}

static void magnet_timer_process(struct k_timer *timer)
{
    struct magnet_service *msvr = CONTAINER_OF(timer, 
        struct magnet_service, timer);
    magnet_submit_work(msvr);
}

static int magnet_service_init(struct magnet_service *msvr, 
    const char *mdev, const char *gdev)
{
    __ASSERT(msvr != NULL, "");
    __ASSERT(mdev != NULL, "");
    __ASSERT(gdev != NULL, "");
    msvr->m = device_get_binding(mdev);
    if (!msvr->m) {
        LOG_ERR("Binding device(%s) failed\n", mdev);
        return -ENODEV;
    }
    msvr->g = device_get_binding(gdev);
    if (!msvr->g) {
        LOG_ERR("Binding device(%s) failed\n", gdev);
        return -ENODEV;
    }
    k_timer_init(&msvr->timer, magnet_timer_process, NULL);
    return 0;
}

static inline float sensor_g_convert(struct sensor_value *val)
{
	int32_t conv_val = ((int64_t)val->val1 * SENSOR_G) >> (14 - 3);
    return (float)conv_val / 1000000.0f;
}

static void __unused magnet_coordinate_raw_to_real(int layout, 
    float *in, float *out)
{
    switch (layout) {
    case 0:
        //x'=-y y'=x z'=z
        out[0] = -in[1];
        out[1] = in[0];
        out[2] = in[2];
        break;
    case 1:
        //x'=x y'=y z'=z
        out[0] = in[0];
        out[1] = in[1];
        out[2] = in[2];
        break;
    case 2:
        //x'=y y'=-x z'=z
        out[0] = in[1];
        out[1] = -in[0];
        out[2] = in[2];
        break;
    case 3:
        //x'=-x y'=-y z'=z
        out[0] = -in[0];
        out[1] = -in[1];
        out[2] = in[2];
        break;
    case 4:
        //x'=y y'=x z'=-z
        out[0] = in[1];
        out[1] = in[0];
        out[2] = -in[2];
        break;
    case 5:
        //x'=x y'=-y z'=-z
        out[0] = in[0];
        out[1] =  -in[1];
        out[2] = -in[2];
        break;
    case 6:
        //x'=-y y'=-x z'=-z
        out[0] = -in[1];
        out[1] = -in[0];
        out[2] = -in[2];
        break;
    case 7:
        //x'=-x y'=y z'=-z
        out[0] = -in[0];
        out[1] = in[1];
        out[2] = -in[2];
        break;
    default:
        //x'=x y'=y z'=z
        out[0] = in[0];
        out[1] = in[1];
        out[2] = in[2];
        break;
    }
}

static void magnet_init_parameters(float mag_sp[9], float pa[8], 
    float pb[4], int pc[4])
{
    mag_sp[0] = 1.0f;
    mag_sp[1] = 0.0f;
    mag_sp[2] = 0.0f;
    mag_sp[3] = 0.0f;
    mag_sp[4] = 1.0f;
    mag_sp[5] = 0.0f;
    mag_sp[6] = 0.0f;
    mag_sp[7] = 0.0f;
    mag_sp[8] = 1.0f;

    pa[0] = 18;		//FirstLevel	= can not be more than 15
    pa[1] = 25;		//SecondLevel	= can not be more than 35
    pa[2] = 35;		//ThirdLevel	= can not be more than 5
    pa[3] = 0.025;	//dCalPDistLOne	
    pa[4] = 0.08;	//dCalPDistLTwo
    pa[5] = 0.85;   //dPointToCenterLow
    pa[6] = 1.15;	//dPointToCenterHigh dd
    pa[7] = 1.4;	//dMaxPiontsDistance

    pb[0] = 0.1;	//dSatAccVar 	= threshold for saturation judgment
    pb[1] = 0.00002;//dSatMagVar 	= threshold for saturation judgment
    pb[2] = 0.0002;	//dMovAccVar 	= threshold for move judgment
    pb[3] = 0.0001;	//dMovMagVar 	= threshold for move judgment

    pc[0] = 20;		//delay_ms 		= sampling interval	
    pc[1] = 15;		//yawFirstLen	= can not be more than 31, must be odd number.
    pc[2] = 10;		//yawFirstLen	= can not be more than 30
    pc[3] = 2;		//corMagLen		= can not be more than 20
}

static void magnet_daemon(struct magnet_service *msvr)
{
    struct sensor_value ms_axis[3];
    struct sensor_value gs_axis[3];
    k_spinlock_key_t key;
    float mag[3], acc[3];
    float mag_sp[9];
    float calib_mag[3];
    float output[3];
    float pa[8];
    float pb[4];
    int pc[4];
    int reset;
    int ret;

    reset = 1;
    msvr->m_axis = ms_axis;
    k_thread_name_set(k_current_get(), "/service@magnet");
    magnet_init_parameters(mag_sp, pa, pb, pc);
    AlgoInitial(mag_sp, pa, pb, pc);

    for (;;) {
        ret = k_sem_take(&msvr->sync, K_FOREVER);
        __ASSERT(ret == 0, "");
        sensor_channel_get(msvr->g, SENSOR_CHAN_ACCEL_XYZ, gs_axis);
        ret = sensor_sample_fetch_chan(msvr->m, SENSOR_CHAN_MAGN_XYZ);
        if (!ret) {
            sensor_channel_get(msvr->m, SENSOR_CHAN_MAGN_XYZ, ms_axis);
            /* Unit: GAUSS*/
            mag[0] = (float)sensor_value_to_double(&ms_axis[0]) / 1000.0f;
            mag[1] = (float)sensor_value_to_double(&ms_axis[1]) / 1000.0f;
            mag[2] = (float)sensor_value_to_double(&ms_axis[2]) / 1000.0f;
        } else {
            msvr->read_errors++;
            LOG_DBG("Read sensor data failed\n");
        }

        /* Unit: G */
        acc[0] = sensor_g_convert(&gs_axis[0]);
        acc[1] = sensor_g_convert(&gs_axis[1]);
        acc[2] = sensor_g_convert(&gs_axis[2]);
        ret = MainAlgorithmProcess(acc, mag, mag, reset, 0);
        if (ret < 0) {
            LOG_ERR("Magnet algorithm error\n");
            continue;
        }

        reset = 0;
        /* get corrected mag data */
        GetCalMag(calib_mag);

        /* calibrated magnetic sensor output */
        // calMagX = caliMag[0];	//unit is gauss
        // calMagY = caliMag[1];	//unit is gauss
        // calMagZ = caliMag[2];	//unit is gauss

        /* Get corrected ori data */
        GetCalOri(output);

        /* Get the fAzimuth Pitch Roll Accuracy for the eCompass */
        key = k_spin_lock(&msvr->spin);
        msvr->azimuth = output[0];
        msvr->pitch = output[1];
        msvr->roll = output[2];
        k_spin_unlock(&msvr->spin, key);
    }
}

static struct magnet_service magnet_svr = {
    .sync = Z_SEM_INITIALIZER(magnet_svr.sync, 0, 1)
};

int magnet_service_start(void)
{
    struct magnet_service *msvr = &magnet_svr;
    struct sensor_value v;
    int ret;

    if (!msvr->ready) {
        ret = magnet_service_init(msvr, MSENSOR_NAME, GSENSOR_NAME);
        if (ret) {
            LOG_ERR("Magnet service exit\n");
            return ret;
        }
        msvr->ready = true;
    }

    v.val1 = 60;
    v.val2 = 0;
    ret = sensor_attr_set(msvr->m, SENSOR_CHAN_MAGN_XYZ, 
        SENSOR_ATTR_SAMPLING_FREQUENCY, &v);
    if (!ret) {
        ret = sensor_attr_set(msvr->m, SENSOR_CHAN_MAGN_XYZ, 
            (int)SENSOR_ATTR_START, NULL);
        if (!ret) 
            k_timer_start(&msvr->timer, K_MSEC(SCHDULE_CYCLE), K_MSEC(SCHDULE_CYCLE));
    }
    return ret;
}

int magnet_service_stop(void)
{
    struct magnet_service *msvr = &magnet_svr;

    k_timer_stop(&msvr->timer);
    return sensor_attr_set(msvr->m, SENSOR_CHAN_MAGN_XYZ, 
            SENSOR_ATTR_STOP, NULL);
}

int magnet_get_xyz(float *x, float *y, float *z)
{
    struct magnet_service *msvr = &magnet_svr;
    k_spinlock_key_t key;

    __ASSERT(x != NULL, "");
    __ASSERT(y != NULL, "");
    __ASSERT(z != NULL, "");
    if (!msvr->m_axis)
        return -EINVAL;
    key = k_spin_lock(&msvr->spin);
    *x = (float)sensor_value_to_double(&msvr->m_axis[0]) / 1000.0f;
    *y = (float)sensor_value_to_double(&msvr->m_axis[1]) / 1000.0f;
    *z = (float)sensor_value_to_double(&msvr->m_axis[2]) / 1000.0f;
    k_spin_unlock(&msvr->spin, key);
    return 0;
}

int magnet_get_angle(float *azimuth, float *pitch, float *roll)
{
    struct magnet_service *msvr = &magnet_svr;
    k_spinlock_key_t key;

    __ASSERT(azimuth != NULL, "");
    key = k_spin_lock(&msvr->spin);
    *azimuth = msvr->azimuth;
    if (pitch)
        *pitch = msvr->pitch;
    if (roll)
        *roll = msvr->roll;
    k_spin_unlock(&msvr->spin, key);
    return 0;
}
K_THREAD_DEFINE(magnet, 4096, magnet_daemon, 
                &magnet_svr, NULL, NULL, 0, 0, 0);
