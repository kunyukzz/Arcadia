#ifndef __RESOURCE_TYPE_H__
#define __RESOURCE_TYPE_H__

#include "engine/math/math_type.h"

typedef struct texture_t {
	uint32_t id;
	uint32_t width;
	uint32_t height;
	uint32_t gen;

	uint8_t channel_count;
	b8 has_transparent;

	void *interal_data;
} texture_t;

#endif // __RESOURCE_TYPE_H__
