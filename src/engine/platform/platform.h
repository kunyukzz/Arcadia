#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include "engine/define.h"

b8   platform_init(uint64_t *memory_require, void *state, const char *name,
                   int32_t x, int32_t y, uint32_t w, uint32_t h);
void platform_shut(void *state);
b8 platform_push(void);

/* Function for memory allocation */
void *platform_allocate(uint64_t size, b8 aligned);
void platform_free(void *block, b8 aligned);
void* platform_zero_mem(void* block, uint64_t size);
void* platform_copy_mem(void* dest, const void* source, uint64_t size);
void* platform_set_mem(void* dest, int32_t value, uint64_t size);

#endif //__PLATFORM_H__
