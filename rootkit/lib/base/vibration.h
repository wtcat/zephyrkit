#ifndef BASE_VIBRATION_H_
#define BASE_VIBRATION_H_

#ifdef __cplusplus
extern "C"{
#endif

/*
 * Start vibration
 *
 * @return: 0 is success
 */
int vib_on(void);

/*
 * Stop vibration
 *
 * @return: 0 is success
 */
int vib_off(void);

/*
 * Generate vibration
 *
 * @ton:  on time [unit: ms]
 * @toff: off time [unit: ms]
 * @times: execute times, repeat execute when it is 0
 * @return: 0 is success
 */
int vib_launch(uint32_t ton, uint32_t toff, uint32_t times);

/*
 * Generate vibration with duty-cycle
 *
 * @ton:  on time [unit: ms]
 * @toff: off time [unit: ms]
 * @duty_on: the on time of pwm cycle
 * @duty_off: the off time of pwm cycle
 * @times: execute times, repeat execute when it is 0
 * @return: 0 is success
 */
int vib_start(uint32_t ton, uint32_t toff, uint32_t duty_on, 
    uint32_t duty_off, uint32_t times);

#ifdef __cplusplus
}
#endif
#endif
