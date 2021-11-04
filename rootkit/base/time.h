#ifndef BASE_TIME_H_
#define BASE_TIME_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

#ifndef __type_time64_t
#define __type_time64_t
typedef int64_t time64_t;
#endif

struct tm {
	/*
	 * the number of seconds after the minute, normally in the range
	 * 0 to 59, but can be up to 60 to allow for leap seconds
	 */
	int tm_sec;
	/* the number of minutes after the hour, in the range 0 to 59*/
	int tm_min;
	/* the number of hours past midnight, in the range 0 to 23 */
	int tm_hour;
	/* the day of the month, in the range 1 to 31 */
	int tm_mday;
	/* the number of months since January, in the range 0 to 11 */
	int tm_mon;
	/* the number of years since 1900 */
	long tm_year;
	/* the number of days since Sunday, in the range 0 to 6 */
	int tm_wday;
	/* the number of days since January 1, in the range 0 to 365 */
	int tm_yday;
};

void time64_to_tm(time64_t totalsecs, int offset, struct tm *result);

#ifdef __cplusplus
}
#endif
#endif /* BASE_TIME_H_ */
