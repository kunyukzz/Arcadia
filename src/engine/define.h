#ifndef __DEFINE_H__
#define __DEFINE_H__

#if defined(__linux__) || defined(__gnu_linux__)
	#define OS_LINUX 1
#elif defined(_WIN32) || defined(WIN32) || defined(__WIN32__)
	#define OS_WINDOWS 1
#endif

#include <stdint.h>

#define true 1
#define false 0

typedef _Bool b8;

#define ar_CLAMP(value, min, max)                                              \
  (value <= min) ? min : (value >= max) ? max : value

#endif //__DEFINE_H__
