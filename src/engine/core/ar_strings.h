#ifndef __AR_STRINGS_H__
#define __AR_STRINGS_H__

#include "engine/define.h"
#include "engine/math/math_type.h"
#include "engine/memory/memory.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#ifndef _MSC_VER
#include <strings.h>
#endif

_arinline char *string_empty(char *str) {
	if (str) {
		str[0] = 0;
	}

	return str;
}

_arinline uint64_t string_length(const char *str) {
	return strlen(str);
}

_arinline char *string_duplicate(const char *str) {
	uint64_t length = string_length(str);
	char *copy = memory_alloc(length + 1, MEMTAG_STRING);
	memory_copy(copy, str, length + 1);
	return copy;
}

_arinline b8 string_equal(const char *str1, const char *str2) {
	return strcmp(str1, str2);
}

_arinline b8 string_equali(const char *str1, const char *str2) {
#if defined(__GNUC__)
	return strcasecmp(str1, str2) == 0;
#elif define(_MSC_VER)
	return _strcmpi(str1, str2) == 0;
#endif
}

_arinline int32_t string_format_v(char *dest, const char *format, va_list va_lispt) {
	if (dest) {
		char buffer[24000];
		int32_t written = vsnprintf(buffer, 24000, format, va_lispt);
		buffer[written] = 0;
		memory_copy(dest, buffer, (uint16_t)written + 1);
		return written;
	}

	return -1;
}

_arinline int32_t string_format(char *dest, const char *format, ...) {
	if (dest) {
		va_list arg_ptr;
		va_start(arg_ptr, format);
		int32_t written = string_format_v(dest, format, arg_ptr);
		va_end(arg_ptr);
		return written;
	}

	return -1;
}

_arinline char *string_copy(char *dest, const char *source) {
    return strcpy(dest, source);
}

_arinline char *string_ncopy(char *dest, const char *source, int64_t length) {
    return strncpy(dest, source, (uint64_t)length);
}

_arinline char *string_trim(char *str) {
    while (isspace((unsigned char)*str)) {
        str++;
    }

	// if not empty string
    if (*str != '\0') {
		char *end = str + string_length(str) - 1;

		// trim trailing whitespace
        while (end > str && isspace((unsigned char)*end)) {
            end--;
        }

		// Null-terminate after last non-whitespace char
		end[1] = '\0';
    }

    return str;
}

_arinline void string_mid(char *dest, const char *source, int32_t start,
                          int32_t length) {
    if (length == 0 || start < 0) {
        dest[0] = 0;
        return;
    }

    uint64_t src_length = string_length(source);
    if ((uint64_t)start >= src_length) {
        dest[0] = 0;
        return;
    }

    uint64_t j = 0;
    if (length > 0) {
        for (uint64_t i = (uint64_t)start; j < (uint64_t)length && source[i];
             ++i, ++j) {
            dest[j] = source[i];
        }
    } else {
        for (uint64_t i = (uint64_t)start; source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
    }

    dest[j] = 0;
}

_arinline int32_t string_index_of(char *str, char c) {
    if (!str) {
        return -1;
    }

	// Version 1
	for (uint32_t i = 0; str[i] != '\0'; ++i) {
		if (str[i] == c) {
			return (int32_t)i;
		}
	}

	// Version 2
	/*
    uint32_t length = string_length(str);
    if (length > 0) {
        for (uint32_t i = 0; i < length; ++i) {
            if (str[i] == c) {
                return (int32_t)i;
            }
        }
    }
	*/

    return -1;
}

_arinline b8 string_to_vec2(char *str, vec2 *v) {
	if (!str) {
		return false;
	}

	memory_zero(v, sizeof(vec2));
	int32_t result = sscanf(str, "%f %f", &v->x, &v->y);
	return result == 2;
}

_arinline b8 string_to_vec3(char *str, vec3 *v) {
	if (!str) {
		return false;
	}

	memory_zero(v, sizeof(vec3));
	int32_t result = sscanf(str, "%f %f %f", &v->x, &v->y, &v->z);
	return result == 3;
}

_arinline b8 string_to_vec4(char *str, vec4 *v) {
    if (!str) {
        return false;
    }

    memory_zero(v, sizeof(vec4));
    int32_t result = sscanf(str, "%f %f %f %f", &v->x, &v->y, &v->z, &v->w);
    return result == 4;
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


#endif //__AR_STRINGS_H__
