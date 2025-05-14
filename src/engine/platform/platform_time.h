#ifndef __PLATFORM_TIME_H__
#define __PLATFORM_TIME_H__

#define _POSIX_C_SOURCE 200809L
#include <time.h>

#include "engine/define.h"

typedef struct platform_time_t {
	double start_time;
	double elapsed;
} platform_time_t;

_arinline double get_absolute_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

_arinline void os_sleep(double wake_time) {
	struct timespec ts;
	ts.tv_sec = (time_t)wake_time;
	ts.tv_nsec = (long)((wake_time - ts.tv_sec) * 1e9);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
}

_arinline void time_start(platform_time_t *time) {
	time->start_time = get_absolute_time();
	time->elapsed = 0.0;
}

_arinline void time_update(platform_time_t *time) {
	if (time->start_time != 0)
		time->elapsed = get_absolute_time() - time->start_time;
}

_arinline void time_sleep(platform_time_t *time) {
	time->start_time = 0;
}

#endif // __PLATFORM_TIME_H__
