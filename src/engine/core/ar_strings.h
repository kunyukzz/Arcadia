#ifndef __AR_STRINGS_H__
#define __AR_STRINGS_H__

#include "engine/define.h"
#include "engine/container/dyn_array.h"
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
	memory_copy(copy, str, length);
	copy[length] = 0;
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

_arinline b8 string_to_float(char *str, float *f) {
	if (!str || !f)
		return false;

	*f = 0;
	int32_t result = sscanf(str, "%f", f);
	return result == 1;
}

_arinline b8 string_to_double(char *str, double *f) {
	if (!str || !f)
		return false;

	*f = 0;
	int32_t result = sscanf(str, "%lf", f);
	return result == 1;
}

_arinline b8 string_to_i8(char *str, int8_t *i) {
	if (!str || !i)
		return false;

	*i = 0;
	int32_t result = sscanf(str, "%hhi", i);
	return result == 1;
}

_arinline b8 string_to_i16(char *str, int16_t *i) {
	if (!str || !i)
		return false;

	*i = 0;
	int32_t result = sscanf(str, "%hi", i);
	return result == 1;
}

_arinline b8 string_to_i32(char *str, int32_t *i) {
	if (!str || !i)
		return false;

	*i = 0;
	int32_t result = sscanf(str, "%i", i);
	return result == 1;
}

_arinline b8 string_to_i64(char *str, int64_t *i) {
	if (!str || !i)
		return false;

	*i = 0;
	int32_t result = sscanf(str, "%li", i);
	return result == 1;
}

_arinline b8 string_to_u8(char *str, uint8_t *u) {
	if (!str || !u)
		return false;

	*u = 0;
	int32_t result = sscanf(str, "%hhu", u);
	return result == 1;
}

_arinline b8 string_to_u16(char *str, uint16_t *u) {
	if (!str || !u)
		return false;

	*u = 0;
	int32_t result = sscanf(str, "%hu", u);
	return result == 1;
}

_arinline b8 string_to_u32(char *str, uint32_t *u) {
	if (!str || !u)
		return false;

	*u = 0;
	int32_t result = sscanf(str, "%u", u);
	return result == 1;
}

_arinline b8 string_to_u64(char *str, uint64_t *u) {
	if (!str || !u)
		return false;

	*u = 0;
	int32_t result = sscanf(str, "%lu", u);
	return result == 1;
}

_arinline b8 string_to_bool(char *str, b8 *b) {
	if (!str || !b)
		return false;

	*b = string_equal(str, "1") || string_equali(str, "true");
	return *b;
}

_arinline uint32_t string_split(const char *str, char delimit,
                                char ***str_array, b8 trim_entry,
                                b8 inc_empty) {
    if (!str || !str_array) {
        return 0;
    }

    char    *result      = 0;
    uint32_t trim_length = 0;
    uint32_t entry_count = 0;
    uint32_t length      = string_length(str);
    char buffer[16384]; // if a single enty goes beyond, please just don't do
                        // that
    uint32_t curr_length = 0;

    for (uint32_t i = 0; i < length; ++i) {
        char c = str[i];

        /* found delimit. finalize string */
        if (c == delimit) {
            buffer[curr_length] = 0;
            result              = buffer;
            trim_length         = curr_length;

            // Trim if doable
            if (trim_entry && curr_length > 0) {
                result      = string_trim(result);
                trim_length = string_length(result);
            }

            // Add new entry
            if (trim_length > 0 || inc_empty) {
                char *ee = memory_alloc(sizeof(char) * (trim_length + 1),
                                        MEMTAG_STRING);
                if (trim_length == 0) {
                    ee[0] = 0;
                } else {
                    string_ncopy(ee, result, trim_length);
                    ee[trim_length] = 0;
                }

                char **a = *str_array;
                dyn_array_push(a, ee);
                *str_array = a;
                entry_count++;
            }

            // Clear buffer
            memory_zero(buffer, sizeof(char) * 16384);
            curr_length = 0;
            continue;
        }
        buffer[curr_length] = c;
        curr_length++;
    }

    /* If any char queued up, read them */
    result      = buffer;
    trim_length = curr_length;

    if (trim_entry && curr_length > 0) {
        result      = string_trim(result);
        trim_length = string_length(result);
    }

    if (trim_length > 0 || inc_empty) {
        char *ee =
            memory_alloc(sizeof(char) * (trim_length + 1), MEMTAG_STRING);
        if (trim_length == 0) {
            ee[0] = 0;
        } else {
            string_ncopy(ee, result, trim_length);
            ee[trim_length] = 0;
        }

        char **a = *str_array;
        dyn_array_push(a, ee);
        *str_array = a;
        entry_count++;
    }

    return entry_count;
}

_arinline void string_clean_split_array(char **str_array) {
    if (str_array) {
        uint32_t count = dyn_array_length(str_array);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t len = string_length(str_array[i]);
            memory_free(str_array[i], sizeof(char) * (len + 1), MEMTAG_STRING);
        }

        dyn_array_clear(str_array);
    }
}

#endif //__AR_STRINGS_H__
