#ifndef __MATH_TYPE_H__
#define __MATH_TYPE_H__

#include "engine/define.h"

typedef union uni_vec2 {
    float elements[2];
    struct {
		// Default
        float x, y;
    };
    struct {
		// Color
        float r, g;
    };
    struct {
		// Texture Coordinate
        float u, v;
    };
} vec2;

typedef union uni_vec3 {
    float elements[3];
    struct {
		// Default
        float x, y, z;
    };
    struct {
		// Color
        float r, g, b;
    };
    struct {
		// Texture Coordinate
        float u, v, w;
    };
} vec3;

#if defined(_MSC_VER)
	#include <intrin.h>
#else
	#include <xmmintrin.h>
#endif

typedef union uni_vec4 {
#if defined(_arsimd)
    _aralignas __m128 data;
#endif
    float elements[4];
	struct {
		// Default
		float x, y, z, d;
	};
	struct {
		// Color
		float r, g, b, a;
	};
	struct {
		// Texture Coordinate
		float u, v, w, h;
	};
} vec4;

typedef vec4 quat;

typedef union uni_mat4 {
    _aralignas float data[16];
#if defined(_arsimd)
    _aralignas vec4 rows[4];
#endif
} mat4;

typedef struct vertex_3d {
    vec3 position;
    vec2 texcoord;
} vertex_3d;

#endif //__MATH_TYPE_H__
