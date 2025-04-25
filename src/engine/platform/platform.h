#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include "engine/define.h"

b8   platform_init(uint64_t *memory_require, void *state, const char *name,
                   int32_t x, int32_t y, uint32_t w, uint32_t h);
void platform_shut(void *state);
b8 platform_push(void);

#endif //__PLATFORM_H__
