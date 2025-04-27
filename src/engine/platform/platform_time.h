#ifndef __PLATFORM_TIME_H__
#define __PLATFORM_TIME_H__

#include "engine/define.h"
#include <time.h>

typedef struct platform_time_t {
	double start_time;
	double elapsed;
} platform_time_t;

_arinline double get_absolute_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;

	/*
    static LARGE_INTEGER frequency;
    static BOOL initialized = FALSE;
    LARGE_INTEGER counter;

    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = TRUE;
    }

    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
	*/

    return 0;
}

_arinline void os_sleep(uint64_t ms) {
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000 * 1000;
}

_arinline void time_start(platform_time_t *time) {
	time->start_time = get_absolute_time();
	time->elapsed = 0;
}

_arinline void time_update(platform_time_t *time) {
	if (time->start_time != 0)
		time->elapsed = get_absolute_time() - time->elapsed;
}

_arinline void time_sleep(platform_time_t *time) {
	time->start_time = 0;
}

#endif // __PLATFORM_TIME_H__
