#ifndef __STRINGS_H__
#define __STRINGS_H__

#include "engine/define.h"

#include "engine/memory/memory.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static inline uint64_t string_length(const char *str) {
	return strlen(str);
}

static inline char *string_duplicate(const char *str) {
	uint64_t length = string_length(str);
	char *copy = memory_alloc(length + 1, MEMTAG_STRING);
	memory_copy(copy, str, length + 1);
	return copy;
}

static inline b8 string_equal(const char *str1, const char *str2) {
	return strcmp(str1, str2);
}


static inline int32_t string_format_v(char *dest, const char *format, void *va_lispt) {
	if (dest) {
		char buffer[24000];
		int32_t written = vsnprintf(buffer, 24000, format, va_lispt);
		buffer[written] = 0;
		memory_copy(dest, buffer, (uint16_t)written + 1);
		return written;
	}

	return -1;
}

static inline int32_t string_format(char *dest, const char *format, ...) {
	if (dest) {
		va_list arg_ptr;
		va_start(arg_ptr, format);
		int32_t written = string_format_v(dest, format, arg_ptr);
		va_end(arg_ptr);
		return written;
	}

	return -1;
}

/*
static inline const char *string_find(const char *haystack, const char *needle) {
	while (*haystack) {
		const char *h = haystack;
		const char *n = needle;
		while (*h && *n && (*h == *n)) {
			++h;
			++n;
		}
		if (!*n) return haystack;
		++haystack;
	}
	return 0;
}
*/


#endif //__STRINGS_H__
