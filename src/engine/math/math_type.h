#ifndef __MATH_TYPE_H__
#define __MATH_TYPE_H__

#include "engine/define.h"

typedef union _aralignas {
	struct { float x, y, _padz1, _padw1; };
	struct { float r, g, _padz2, _padw2; } comp2;
	struct { float u, v, _padz3, _padw3; } comp3;
	float elements[4];
} vec2;

typedef union _aralignas {
    struct { float x, y, z, _pad1; };
    struct { float r, g, b, _pad2; } comp2;
    struct { float u, v, t, _pad3; } comp3;
    float elements[4];
} vec3;

typedef union _aralignas {
	struct { float x, y, z, w; };
	struct { float r, g, b, a; } comp2;
	struct { float u, v, t, s; } comp3;
	float elements[4];
} vec4;

typedef union _aralignas {
	float data[16];
	vec4 rows[4];
} mat4;

typedef vec4 quat;

typedef struct vertex_3d {
    vec3 position;
    vec2 texcoord;
} vertex_3d;

#endif //__MATH_TYPE_H__
