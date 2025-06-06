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
#define INVALID_ID 4294967295u

#define GIBIBYTES(amount) ((amount) * 1024 * 1024 * 1024)
#define MEBIBYTES(amount) ((amount) * 1024 * 1024)
#define KIBIBYTES(amount) ((amount) * 1024)
#define GIGABYTES(amount) ((amount) * 1000 * 1000 * 1000)
#define MEGABYTES(amount) ((amount) * 1000 * 1000)
#define KILOBYTES(amount) ((amount) * 1000)

typedef _Bool b8;

#define ar_CLAMP(value, min, max)                                              \
  (value <= min) ? min : (value >= max) ? max : value

// Imports
#ifdef _MSC_VER
	#define _arapi __declspec(dllimport)
#else
	#define _arapi
#endif

// Inline
#if defined(__clang__) || defined(__gcc__)
	#define _arinline static __attribute__((always_inline)) inline
	#define _arnoinline __attribute__((noinline))
#elif defined(_MSC_VER)
	#define _arinline __forceinline
	#define _arnoinline __declspec(noinline)
#endif

// SIMD
#ifdef _MSC_VER
	#define _aralignas __declspec(align(16))
#else
	#define _aralignas __attribute__((aligned(16)))
#endif

#endif //__DEFINE_H__
