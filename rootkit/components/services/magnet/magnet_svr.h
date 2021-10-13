#ifndef SERVICES_MAGNET_MAGNET_SVR_H_
#define SERVICES_MAGNET_MAGNET_SVR_H_

#ifdef __cplusplus
extern "C"{
#endif

int magnet_service_start(void);
int magnet_service_stop(void);
int magnet_get_xyz(float *x, float *y, float *z);
int magnet_get_angle(float *azimuth, float *pitch, float *roll);

#ifdef __cplusplus
}
#endif
#endif /* SERVICES_MAGNET_MAGNET_SVR_H_ */
