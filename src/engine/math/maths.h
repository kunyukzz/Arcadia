#ifndef __MATHS_H__
#define __MATHS_H__

#include "engine/math/math_type.h"
#include "engine/memory/memory.h"

#define AR_USE_COLUMN_MAJOR

#ifdef AR_USE_COLUMN_MAJOR
    #ifdef AR_USE_ROW_MAJOR
        #error "Cannot define both COLUMN_MAJOR and ROW_MAJOR. Define one layout only."
    #endif
#elif defined(AR_USE_ROW_MAJOR)
    #ifdef AR_USE_COLUMN_MAJOR
        #error "Cannot define both COLUMN_MAJOR and ROW_MAJOR. Define one layout only."
    #endif
#endif

#define _ar_PI 3.14159265358979323846f
#define _ar_SQRT_2 1.41421356237309504880f
#define _ar_SQRT_3 1.73205080756887729252f
#define _ar_SEC2MS_MULTI 1000.0f
#define _ar_MS2SEC_MULTI 0.001f
#define _ar_INFINITE 1e30f
#define _ar_EPSILON 1.192092896e-07f

#define _ar_2PI (2.0f * _ar_PI)
#define _ar_HALF_PI (0.5f * _ar_PI)
#define _ar_QUARTER_PI (0.25f * _ar_PI)
#define _ar_DEG2RAD (_ar_PI / 180.0f)
#define _ar_RAD2DEG (180.0f / _ar_PI)

_arapi float _ar_sinf(float x);
_arapi float _ar_cosf(float x);
_arapi float _ar_tanf(float x);
_arapi float _ar_acosf(float x);
_arapi float _ar_sqrtf(float x);
_arapi float _ar_absf(float x);

_arapi int32_t _ar_random(void);
_arapi int32_t _ar_random_in_range(int32_t min, int32_t max);
_arapi float _ar_frandom(void);
_arapi float _ar_frandom_in_range(float min, float max);

_arinline b8   is_power_of_2(uint64_t value) {
    return (value != 0) && ((value & (value - 1)) == 0);
}

_arinline float deg_to_rad(float degree) { return degree * _ar_DEG2RAD; }
_arinline float rad_to_deg(float rad) { return rad * _ar_RAD2DEG; }

/* ========================= VECTOR OPERATION =============================== */
/* ========================================================================== */

/* ============================== VECTOR 2 ================================== */
_arinline vec2 vec2_create(float x, float y) {
    return (vec2){.x = x, .y = y};
}

_arinline vec2 vec2_zero(void) {
    return (vec2){.x = 0.0f, .y = 0.0f};
}

_arinline vec2 vec2_one(void) {
    return (vec2){.x = 1.0f, .y = 1.0f};
}

_arinline vec2 vec2_add(vec2 v1, vec2 v2) {
    return (vec2){.x = v1.x + v2.x, .y = v1.y + v2.y};
}

_arinline vec2 vec2_sub(vec2 v1, vec2 v2) {
    return (vec2){.x = v1.x - v2.x, .y = v1.y - v2.y};
}

_arinline vec2 vec2_multi(vec2 v1, vec2 v2) {
    return (vec2){.x = v1.x * v2.x, .y = v1.y * v2.y};
}

_arinline vec2 vec2_divide(vec2 v1, vec2 v2) {
    return (vec2){.x = v1.x / v2.x, .y = v1.y / v2.y};
}

_arinline float vec2_length_square(vec2 v) {
	return v.x * v.x + v.y * v.y;
}

_arinline float vec2_length(vec2 v) {
	return _ar_sqrtf(vec2_length_square(v));
}

_arinline float vec2_dot(vec2 v1, vec2 v2) {
	return v1.x * v2.x + v1.y * v2.y;
}

_arinline float vec2_distance(vec2 v1, vec2 v2) {
    vec2 dist = (vec2){.x = v1.x - v2.x, .y = v1.y - v2.y};
    return vec2_length(dist);
}

/* ============================== VECTOR 3 ================================== */
/* Vector3 has 2 ways implementation. using scalar operation and using SIMD
 * for now, SIMD only works on SSE1 but portable for Windows or Linux.
 * and i think conpatible with SSE2 also
 */
#define _AR_USE_SIMD true

_arapi vec3 svec3_add(vec3 v1, vec3 v2);
_arapi vec3 svec3_sub(vec3 v1, vec3 v2);
_arapi vec3 svec3_multi(vec3 v1, vec3 v2);
_arapi vec3 svec3_divide(vec3 v1, vec3 v2);
_arapi vec3 svec3_multi_scalar(vec3 v, float s);
_arapi float svec3_length_square(vec3 v);
_arapi float svec3_length(vec3 v);
_arapi float svec3_dot(vec3 v1, vec3 v2);
_arapi vec3 svec3_cross(vec3 v1, vec3 v2);
_arapi void svec3_normalized(vec3 *v);
_arapi vec3 svec3_get_normalized(vec3 v);

_arinline vec3 vec3_create(float x, float y, float z) {
    return (vec3){.x = x, .y = y, .z = z};
}

_arinline vec3 vec3_zero(void) {
    return (vec3){.x = 0.0f, .y = 0.0f, .z = 0.0f};
}

_arinline vec3 vec3_one(void) {
	return (vec3){.x = 1.0f, .y = 1.0f, .z = 1.0f};
}

_arinline vec3 vec3_add(vec3 v1, vec3 v2) {
    return (vec3){.x = v1.x + v2.x, .y = v1.y + v2.y, .z = v1.z + v2.z};
}

_arinline vec3 vec3_sub(vec3 v1, vec3 v2) {
    return (vec3){.x = v1.x - v2.x, .y = v1.y - v2.y, .z = v1.z - v2.z};
}

_arinline vec3 vec3_multi(vec3 v1, vec3 v2) {
    return (vec3){.x = v1.x * v2.x, .y = v1.y * v2.y, .z = v1.z * v2.z};
}

_arinline vec3 vec3_divide(vec3 v1, vec3 v2) {
    return (vec3){.x = v1.x / v2.x, .y = v1.y / v2.y, .z = v1.z / v2.z};
}

_arinline vec3 vec3_multi_scalar(vec3 v, float s) {
    return (vec3){.x = v.x * s, .y = v.y * s, .z = v.z * s};
}

_arinline float vec3_length_square(vec3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

_arinline float vec3_length(vec3 v) {
	return _ar_sqrtf(vec3_length_square(v));
}

_arinline float vec3_dot(vec3 v1, vec3 v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

_arinline vec3 vec3_cross(vec3 v1, vec3 v2) {
    return (vec3){.x = v1.y * v2.z - v1.z * v2.y,
                  .y = v1.z * v2.x - v1.x * v2.z,
                  .z = v1.x * v2.y - v1.y * v2.x};
}

_arinline void vec3_normalized(vec3 *v) {
	const float length = vec3_length(*v);
	v->x /= length;
	v->y /= length;
	v->z /= length;
}

_arinline vec3 vec3_get_normalized(vec3 v) {
	vec3_normalized(&v);
	return v;
}

_arinline float vec3_distance(vec3 v1, vec3 v2) {
	vec3 dist = (vec3){.x = v1.x - v2.x, .y = v1.y - v2.y, .z = v1.z - v2.z};
	return vec3_length(dist);
}

_arinline b8 vec3_compared(vec3 v1, vec3 v2, float tolerance) {
	if (_ar_absf(v1.x - v2.x) > tolerance) {
		return false;
	}

	if (_ar_absf(v1.y - v2.y) > tolerance) {
		return false;
	}

	if (_ar_absf(v1.z - v2.z) > tolerance) {
		return false;
	}
	return true;
}

/* ============================== VECTOR 4 ================================== */

_arinline vec4 vec4_create(float x, float y, float z, float w) {
    vec4 out;
    out.x = x;
    out.y = y;
    out.z = z;
    out.w = w;
    return out;
}

_arinline vec4 vec4_zero(void) {
	return (vec4){.x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 0.0f};
}

_arinline vec4 vec4_one(void) {
	return (vec4){.x = 1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f};
}

_arinline vec4 vec4_add(vec4 v1, vec4 v2) {
    return (vec4){.x = v1.x + v2.x,
                  .y = v1.y + v2.y,
                  .z = v1.z + v2.z,
                  .w = v1.w + v2.w};
}

_arinline vec4 vec4_sub(vec4 v1, vec4 v2) {
    return (vec4){.x = v1.x - v2.x,
                  .y = v1.y - v2.y,
                  .z = v1.z - v2.z,
                  .w = v1.w - v2.w};
}

_arinline vec4 vec4_multi(vec4 v1, vec4 v2) {
    return (vec4){.x = v1.x * v2.x,
                  .y = v1.y * v2.y,
                  .z = v1.z * v2.z,
                  .w = v1.w * v2.w};
}

_arinline vec4 vec4_divide(vec4 v1, vec4 v2) {
    return (vec4){.x = v1.x / v2.x,
                  .y = v1.y / v2.y,
                  .z = v1.z / v2.z,
                  .w = v1.w / v2.w};
}

_arinline float vec4_length_square(vec4 v) {
	return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

_arinline float vec4_length(vec4 v) {
	return _ar_sqrtf(vec4_length_square(v));
}

_arinline void vec4_normalized(vec4 *v) {
	const float length = vec4_length(*v);
	v->x /= length;
	v->y /= length;
	v->z /= length;
	v->w /= length;
}

_arinline vec4 vec4_get_normalized(vec4 v) {
	vec4_normalized(&v);
	return v;
}

_arinline float vec4_dot_float(float a0, float a1, float a2, float a3, float b0,
                               float b1, float b2, float b3) {
    return (a0 * b0) + (a1 * b1) + (a2 * b2) + (a3 * b3);
}

/* ========================= MATRIX OPERATION =============================== */
/* ========================================================================== */

#define USE_COLUMN_MAJOR
_arinline mat4 mat4_identity() {
    mat4 result;
    memory_zero(result.data, sizeof(float) * 16);
    result.data[0]  = 1.0f;
    result.data[5]  = 1.0f;
    result.data[10] = 1.0f;
    result.data[15] = 1.0f;
    return result;
}

_arinline mat4 mat4_translate(vec3 pos) {
	mat4 result = mat4_identity();

	// Row-Major & Column-Major
	result.data[12] = pos.x;
	result.data[13] = pos.y;
	result.data[14] = pos.z;

	return result;
}

_arinline mat4 mat4_scale(vec3 scale) {
	mat4 result = mat4_identity();

	// Row-Major & Column-Major
	result.data[0] = scale.x;
	result.data[5] = scale.y;
	result.data[10] = scale.z;

	return result;
}

_arinline vec3 mat4_forward(mat4 matrix) {
    vec3 result;

	// Row-Major & Column-Major
    result.x = -matrix.data[2];
    result.y = -matrix.data[6];
    result.z = -matrix.data[10];
    vec3_normalized(&result);

    return result;
}

_arinline vec3 mat4_backward(mat4 matrix) {
    vec3 result;

	// Row-Major & Column-Major
    result.x = matrix.data[2];
    result.y = matrix.data[6];
    result.z = matrix.data[10];
    vec3_normalized(&result);

    return result;
}

_arinline vec3 mat4_up(mat4 matrix) {
    vec3 result;

	// Row-Major & Column-Major
    result.x = matrix.data[1];
    result.y = matrix.data[5];
    result.z = matrix.data[9];
    vec3_normalized(&result);

    return result;
}

_arinline vec3 mat4_down(mat4 matrix) {
    vec3 result;

	// Row-Major & Column-Major
    result.x = -matrix.data[1];
    result.y = -matrix.data[5];
    result.z = -matrix.data[9];
    vec3_normalized(&result);

    return result;
}

_arinline vec3 mat4_left(mat4 matrix) {
    vec3 result;

	// Row-Major & Column-Major
    result.x = -matrix.data[0];
    result.y = -matrix.data[4];
    result.z = -matrix.data[8];
    vec3_normalized(&result);

    return result;
}

_arinline vec3 mat4_right(mat4 matrix) {
    vec3 result;

	// Row-Major & Column-Major
    result.x = matrix.data[0];
    result.y = matrix.data[4];
    result.z = matrix.data[8];
    vec3_normalized(&result);

    return result;
}

_arinline mat4 mat4_euler_x(float angle_rad) {
	mat4 result = mat4_identity();
	float cos = _ar_cosf(angle_rad);
	float sin = _ar_sinf(angle_rad);

	// Row-Major & Column-Major
	result.data[5] = cos;
	result.data[6] = sin;
	result.data[9] = -sin;
	result.data[10] = cos;

	return result;
}

_arinline mat4 mat4_euler_y(float angle_rad) {
	mat4 result = mat4_identity();
	float cos = _ar_cosf(angle_rad);
	float sin = _ar_sinf(angle_rad);

	// Row-Major & Column-Major
	result.data[0] = cos;
	result.data[2] = -sin;
	result.data[8] = sin;
	result.data[10] = cos;

	return result;
}

_arinline mat4 mat4_euler_z(float angle_rad) {
	mat4 result = mat4_identity();
	float cos = _ar_cosf(angle_rad);
	float sin = _ar_sinf(angle_rad);

	// Row-Major & Column-Major
	result.data[0] = cos;
	result.data[1] = sin;
	result.data[4] = -sin;
	result.data[5] = cos;

	return result;
}

_arinline mat4 mat4_multi(mat4 m1, mat4 m2) {
    mat4         result;
    const float *m1_ptr  = m1.data;
    const float *m2_ptr  = m2.data;
    float       *dst_ptr = result.data;

#ifdef AR_USE_COLUMN_MAJOR
    for (int32_t i = 0; i < 4; ++i) {
        for (int32_t j = 0; j < 4; ++j) {
            *dst_ptr = m1_ptr[0] * m2_ptr[0 + j] + m1_ptr[1] * m2_ptr[4 + j] +
                       m1_ptr[2] * m2_ptr[8 + j] + m1_ptr[3] * m2_ptr[12 + j];
            dst_ptr++;
        }
        m1_ptr += 4;
    }
	
#elif defined(AR_USE_ROW_MAJOR)
	for (int32_t i = 0; i < 4; ++i) {
        for (int32_t j = 0; j < 4; ++j) {
            float sum = 0.0f;
            for (int32_t k = 0; k < 4; ++k) {
                sum += m1_ptr[k] * m2_ptr[k * 4 + j];
            }
            *dst_ptr++ = sum;
        }
        m1_ptr += 4;
    }
#else
#error "Matrix layout not defined. Select either COLUMN or ROW MAJOR."
#endif
    return result;
}

_arinline mat4 mat4_ortho(float left, float right, float bottom, float top,
                          float near, float far) {
	mat4 result;
	memory_zero(result.data, sizeof(float) * 16);

#ifdef AR_USE_COLUMN_MAJOR
	float lr = 1.0f / (left - right);
	float bt = 1.0f / (bottom - top);
	float nf = 1.0f / (near - far);

	result.data[0] = -2.0f * lr;
	result.data[5] = -2.0f * bt;
	result.data[10] = 2.0f * nf;
	result.data[12] = (left + right) * lr;
	result.data[13] = (top - bottom) * bt;
	result.data[14] = (far - near) * nf;
	result.data[15] = 1.0f;

#elif defined(AR_USE_ROW_MAJOR)
    float lr = 1.0f / (right - left);
    float bt = 1.0f / (top - bottom);
    float nf = 1.0f / (far - near);

    result.data[0]  = 2.0f * lr;
    result.data[5]  = 2.0f * bt;
    result.data[10] = -2.0f * nf;
    result.data[3]  = -(right + left) * lr;
    result.data[7]  = -(top + bottom) * bt;
    result.data[11] = -(far + near) * nf;
    result.data[15] = 1.0f;
#else
#error "Matrix layout not defined. Select either COLUMN or ROW MAJOR."
#endif

	return result;
}

_arinline mat4 mat4_perspective(float fov_rad, float aspect_ratio, float near,
                                float far) {
    float half_tan = _ar_tanf(fov_rad * 0.5f);
    mat4  result;
    memory_zero(result.data, sizeof(float) * 16);

#ifdef AR_USE_COLUMN_MAJOR
    result.data[0]  = 1.0f / (aspect_ratio * half_tan);
    result.data[5]  = 1.0f / half_tan;
    result.data[10] = -((far + near) / (far - near));
    result.data[11] = -1.0f;
    result.data[14] = -((2.0f * far * near) / (far - near));

#elif defined(AR_USE_ROW_MAJOR)
	result.data[0]  = 1.0f / (aspect_ratio * half_tan);
    result.data[5]  = 1.0f / half_tan;
    result.data[10] = -((far + near) / (far - near));
    result.data[7] = -1.0f;
    result.data[14] = -((2.0f * far * near) / (far - near));
#else
#error "Matrix layout not defined. Select either COLUMN or ROW MAJOR."
#endif

    return result;
}

_arinline mat4 mat4_lookat(vec3 pos, vec3 target, vec3 up) {
    mat4 result;
    vec3 z_axis;
    z_axis.x        = target.x - pos.x;
    z_axis.y        = target.y - pos.y;
    z_axis.z        = target.z - pos.z;
    z_axis          = vec3_get_normalized(z_axis);

    vec3 x_axis     = vec3_get_normalized(vec3_cross(z_axis, up));
    vec3 y_axis     = vec3_cross(x_axis, z_axis);

#ifdef AR_USE_COLUMN_MAJOR
    result.data[0]  = x_axis.x;
    result.data[1]  = y_axis.x;
    result.data[2]  = -z_axis.x;
    result.data[3]  = 0;

    result.data[4]  = x_axis.y;
    result.data[5]  = y_axis.y;
    result.data[6]  = -z_axis.y;
    result.data[7]  = 0;

    result.data[8]  = x_axis.z;
    result.data[9]  = y_axis.z;
    result.data[10] = -z_axis.z;
    result.data[11] = 0;

    result.data[12] = -vec3_dot(x_axis, pos);
    result.data[13] = -vec3_dot(y_axis, pos);
    result.data[14] = vec3_dot(z_axis, pos);
    result.data[15] = 1.0f;

#elif defined(AR_USE_ROW_MAJOR)
	result.data[0]  = x_axis.x;
    result.data[1]  = x_axis.y;
    result.data[2]  = x_axis.z;
    result.data[3]  = -vec3_dot(x_axis, pos);

    result.data[4]  = y_axis.x;
    result.data[5]  = y_axis.y;
    result.data[6]  = y_axis.z;
    result.data[7]  = -vec3_dot(y_axis, pos);

    result.data[8]  = -z_axis.x;
    result.data[9]  = -z_axis.y;
    result.data[10] = -z_axis.z;
    result.data[11] = vec3_dot(z_axis, pos);

    result.data[12] = 0.0f;
    result.data[13] = 0.0f;
    result.data[14] = 0.0f;
    result.data[15] = 1.0f;
#else
#error "Matrix layout not defined. Select either COLUMN or ROW MAJOR."
#endif

    return result;
}

_arinline mat4 mat4_transpose(mat4 matrix) {
    mat4 result; 

#ifdef AR_USE_COLUMN_MAJOR
	result.data[0]  = matrix.data[0];
    result.data[4]  = matrix.data[1];
    result.data[8]  = matrix.data[2];
    result.data[12] = matrix.data[3];

    result.data[1]  = matrix.data[4];
    result.data[5]  = matrix.data[5];
    result.data[9]  = matrix.data[6];
    result.data[13] = matrix.data[7];

    result.data[2]  = matrix.data[8];
    result.data[6]  = matrix.data[9];
    result.data[10] = matrix.data[10];
    result.data[14] = matrix.data[11];

    result.data[3]  = matrix.data[12];
    result.data[7]  = matrix.data[13];
    result.data[11] = matrix.data[14];
    result.data[15] = matrix.data[15];

#elif defined(AR_USE_ROW_MAJOR)
    result.data[0]  = matrix.data[0];
    result.data[1]  = matrix.data[4];
    result.data[2]  = matrix.data[8];
    result.data[3]  = matrix.data[12];

    result.data[4]  = matrix.data[1];
    result.data[5]  = matrix.data[5];
    result.data[6]  = matrix.data[9];
    result.data[7]  = matrix.data[13];

    result.data[8]  = matrix.data[2];
    result.data[9]  = matrix.data[6];
    result.data[10] = matrix.data[10];
    result.data[11] = matrix.data[14];

    result.data[12] = matrix.data[3];
    result.data[13] = matrix.data[7];
    result.data[14] = matrix.data[11];
    result.data[15] = matrix.data[15];
#else
#error "Matrix layout not defined. Select either COLUMN or ROW MAJOR."
#endif
    return result;
}

_arinline mat4 mat4_inverse(mat4 matrix) {
    const float *m   = matrix.data;
    float        t0  = m[10] * m[15];
    float        t1  = m[14] * m[11];
    float        t2  = m[6] * m[15];
    float        t3  = m[14] * m[7];
    float        t4  = m[6] * m[11];
    float        t5  = m[10] * m[7];
    float        t6  = m[2] * m[15];
    float        t7  = m[14] * m[3];
    float        t8  = m[2] * m[11];
    float        t9  = m[10] * m[3];
    float        t10 = m[2] * m[7];
    float        t11 = m[6] * m[3];
    float        t12 = m[8] * m[13];
    float        t13 = m[12] * m[9];
    float        t14 = m[4] * m[13];
    float        t15 = m[12] * m[5];
    float        t16 = m[4] * m[9];
    float        t17 = m[8] * m[5];
    float        t18 = m[0] * m[13];
    float        t19 = m[12] * m[1];
    float        t20 = m[0] * m[9];
    float        t21 = m[8] * m[1];
    float        t22 = m[0] * m[5];
    float        t23 = m[4] * m[1];

    mat4 result;
	float *o = result.data;

    o[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) -
           (t1 * m[5] + t2 * m[9] + t5 * m[13]);

    o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) -
           (t0 * m[1] + t7 * m[9] + t8 * m[13]);

    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) -
           (t3 * m[1] + t6 * m[5] + t11 * m[13]);

    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) -
           (t4 * m[1] + t9 * m[5] + t10 * m[9]);

#ifdef AR_USE_COLUMN_MAJOR
    float d = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);

    o[0]    = d * o[0];
    o[1]    = d * o[1];
    o[2]    = d * o[2];
    o[3]    = d * o[3];

    o[4]    = d * ((t1 * m[4] + t2 * m[8] + t5 * m[12]) -
                (t0 * m[4] + t3 * m[8] + t4 * m[12]));
	
    o[5]    = d * ((t0 * m[0] + t7 * m[8] + t8 * m[12]) -
                (t1 * m[0] + t6 * m[8] + t9 * m[12]));

    o[6]    = d * ((t3 * m[0] + t6 * m[4] + t11 * m[12]) -
                (t2 * m[0] + t7 * m[4] + t10 * m[12]));

    o[7]    = d * ((t4 * m[0] + t9 * m[4] + t10 * m[8]) -
                (t5 * m[0] + t8 * m[4] + t11 * m[8]));

    o[8]    = d * ((t12 * m[7] + t15 * m[11] + t16 * m[15]) -
                (t13 * m[7] + t14 * m[11] + t17 * m[15]));

    o[9]    = d * ((t13 * m[3] + t18 * m[11] + t21 * m[15]) -
                (t12 * m[3] + t19 * m[11] + t20 * m[15]));

    o[10]   = d * ((t14 * m[3] + t19 * m[7] + t22 * m[15]) -
                 (t15 * m[3] + t18 * m[7] + t23 * m[15]));

    o[11]   = d * ((t17 * m[3] + t20 * m[7] + t23 * m[11]) -
                 (t16 * m[3] + t21 * m[7] + t22 * m[11]));

    o[12]   = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6]) -
                 (t16 * m[14] + t12 * m[6] + t15 * m[10]));
	
    o[13]   = d * ((t20 * m[14] + t12 * m[2] + t19 * m[10]) -
                 (t18 * m[10] + t21 * m[14] + t13 * m[2]));

    o[14]   = d * ((t18 * m[6] + t23 * m[14] + t15 * m[2]) -
                 (t22 * m[14] + t14 * m[2] + t19 * m[6]));

    o[15]   = d * ((t22 * m[10] + t16 * m[2] + t21 * m[6]) -
                 (t20 * m[6] + t23 * m[10] + t17 * m[2]));

#elif defined(AR_USE_ROW_MAJOR)
	float d = 1.0f / (m[0] * o[0] + m[1] * o[4] + m[2] * o[8] + m[3] * o[12]);
	for (int i = 0; i < 16; ++i) o[i] *= d;

#define SWAP(a, b) { float tmp = a; a = b; b = tmp; }
	SWAP(o[1], o[4]);
    SWAP(o[2], o[8]);
    SWAP(o[3], o[12]);
    SWAP(o[6], o[9]);
    SWAP(o[7], o[13]);
    SWAP(o[11], o[14]);
#undef SWAP
#else
#error "Matrix layout not defined. Select either COLUMN or ROW MAJOR."
#endif

    return result;
}

_arinline mat4 mat4_euler_xyz(float x_rad, float y_rad, float z_rad) {
	mat4 rx = mat4_euler_x(x_rad);
	mat4 ry = mat4_euler_y(y_rad);
	mat4 rz = mat4_euler_z(z_rad);

#ifdef AR_USE_COLUMN_MAJOR
	mat4 result = mat4_multi(rz, ry);
	result = mat4_multi(result, rx);

#elif defined(AR_USE_ROW_MAJOR)
	mat4 result = mat4_multi(rx, ry);
	result = mat4_multi(result, rz);
#else
#error "Matrix layout not defined. Select either COLUMN or ROW MAJOR."
#endif

	return result;
}

/* ============================== QUATERNION ================================ */
/* ========================================================================== */

_arinline quat quat_identity() {
    return (quat){.x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f};
}

_arinline float quat_normalized(quat q) {
	return _ar_sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

_arinline quat quat_get_normalized(quat q) {
    float normal = quat_normalized(q);
    return (quat){.x = q.x * normal,
                  .y = q.y * normal,
                  .z = q.z * normal,
                  .w = q.w * normal};
}

_arinline quat quat_conjugate(quat q) {
	return (quat){.x = -q.x, .y = -q.y, .z = -q.z, .w = q.w};
}

_arinline quat quat_inverse(quat q) {
	return quat_get_normalized(quat_conjugate(q));
}

_arinline float quat_dot(quat q1, quat q2) {
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

_arinline quat quat_multi(quat q1, quat q2) {
    quat result;

    result.x = q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.z * q1.x;
    result.y = -q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y;
    result.z = q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z;
    result.w = -q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w;

    return result;
}

_arinline quat quat_from_axis_angle(vec3 axis, float angle, b8 normalize) {
    const float half_angle = 0.5f * angle;
    float       s          = _ar_sinf(half_angle);
    float       c          = _ar_cosf(half_angle);
    quat        result =
        (quat){.x = s * axis.x, .y = s * axis.y, .z = s * axis.z, .w = c};

    if (normalize)
        return quat_get_normalized(result);

    return result;
}

_arinline quat quat_slerp(quat q_0, quat q_1, float percentage) {
    quat out_quaternion;
    // Source: https://en.wikipedia.org/wiki/Slerp
    // Only unit quaternions are valid rotations.
    quat  v0  = quat_get_normalized(q_0);
    quat  v1  = quat_get_normalized(q_1);
    float dot = quat_dot(v0, v1);

    // If the dot product is negative, slerp won't take
    // the shorter path. Note that v1 and -v1 are equivalent when
    // the negation is applied to all four components. Fix by reversing one
    // quaternion
    if (dot < 0.0f) {
        v1.x = -v1.x;
        v1.y = -v1.y;
        v1.z = -v1.z;
        v1.w = -v1.w;
        dot  = -dot;
    }
    const float DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD) {
        // If the inputs are too close for comfort, linearly interpolate &
        // normalized
        out_quaternion = (quat){.x = v0.x + ((v1.x - v0.x) * percentage),
                                .y = v0.y + ((v1.y - v0.y) * percentage),
                                .z = v0.z + ((v1.z - v0.z) * percentage),
                                .w = v0.w + ((v1.w - v0.w) * percentage)};
        return quat_get_normalized(out_quaternion);
    }

    // Since dot is in range [0, DOT_THRESHOLD], acos is safe
    float theta_0     = _ar_acosf(dot);
    float theta       = theta_0 * percentage;
    float sin_theta   = _ar_sinf(theta);
    float sin_theta_0 = _ar_sinf(theta_0);
    float s0          = _ar_cosf(theta) - dot * sin_theta / sin_theta_0;
    float s1          = sin_theta / sin_theta_0;

    return (quat){.x = (v0.x * s0) + (v1.x * s1),
                  .y = (v0.y * s0) + (v1.y * s1),
                  .z = (v0.z * s0) + (v1.z * s1),
                  .w = (v0.w * s0) + (v1.w * s1)};
}

_arinline mat4 quat_to_rotation_matrix(quat q, vec3 center) {
    mat4   result;
    float *o = result.data;

	float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;
	
#ifdef AR_USE_COLUMN_MAJOR
    o[0]     = 1.0f - 2.0f * (yy + zz);
    o[1]     = 2.0f * (xx + wz);
    o[2]     = 2.0f * (xz - wy);
    o[3]     = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];

    o[4]     = 2.0f * (xy - wz);
    o[5]     = 1.0f - 2.0f * (xx + zz);
    o[6]     = 2.0f * (yz + wx);
    o[7]     = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];

    o[8]     = 2.0f * (xz + wy);
    o[9]     = 2.0f * (yz - wx);
    o[10]    = 1.0f - 2.0f * (xx + yy);
    o[11]    = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];

    o[12]    = 0.0f;
    o[13]    = 0.0f;
    o[14]    = 0.0f;
    o[15]    = 1.0f;

#elif defined(AR_USE_ROW_MAJOR)
	o[0]  = 1.0f - 2.0f * (yy + zz);
    o[1]  = 2.0f * (xy - wz);
    o[2]  = 2.0f * (xz + wy);
    o[3]  = 0.0f;

    o[4]  = 2.0f * (xy + wz);
    o[5]  = 1.0f - 2.0f * (xx + zz);
    o[6]  = 2.0f * (yz - wx);
    o[7]  = 0.0f;

    o[8]  = 2.0f * (xz - wy);
    o[9]  = 2.0f * (yz + wx);
    o[10] = 1.0f - 2.0f * (xx + yy);
    o[11] = 0.0f;

    o[12] = center.x - center.x * o[0] - center.y * o[4] - center.z * o[8];
    o[13] = center.y - center.x * o[1] - center.y * o[5] - center.z * o[9];
    o[14] = center.z - center.x * o[2] - center.y * o[6] - center.z * o[10];
    o[15] = 1.0f;
#else
#error "Matrix layout not defined. Select either COLUMN or ROW MAJOR."
#endif

    return result;
}

#endif //__MATHS_H__
