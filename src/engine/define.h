#ifndef __DEFINE_H__
#define __DEFINE_H__

#include <stdint.h>

#define true 1
#define false 0

typedef _Bool b8;

#if defined(__linux__) || defined(__gnu_linux__)
	#define OS_LINUX 1
#elif defined(_WIN32) || defined(WIN32) || defined(__WIN32__)
	#define OS_WINDOWS 1
	#include <windows.h>
#endif

#endif //__DEFINE_H__
