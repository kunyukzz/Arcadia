#ifndef __MATHS_H__
#define __MATHS_H__

#include "engine/define.h"
#include "engine/math/math_type.h"
#include "engine/memory/memory.h"

#define _ar_PI 3.14159265358979323846f
#define _ar_2PI 2.0f * _ar_PI
#define _ar_HALF_PI 0.5f * _ar_PI
#define _ar_QUARTER_PI 0.25f * _ar_PI
#define _ar_SQRT_2 1.41421356237309504880f
#define _ar_SQRT_3 1.73205080756887729252f
#define _ar_DEG2RAD _ar_PI / 180.0f
#define _ar_RAD2DEG 180.0f / _ar_PI

#define _ar_SEC2MS_MULTI 1000.0f
#define _ar_MS2SEC_MULTI 0.001f
#define _ar_INFINITE 1e30f
#define _ar_EPSILON 1.192092896e-07f

_arapi float _arsin(float x);
_arapi float _arcos(float x);
_arapi float _artan(float x);
_arapi float _aracos(float x);
_arapi float _arsqrt(float x);
_arapi float _arabs(float x);

_arapi int32_t _ar_random();
_arapi int32_t _ar_random_in_range(int32_t min, int32_t max);
_arapi float _ar_frandom();
_arapi float _ar_frandom_in_range(float min, float max);


_arinline b8   is_power_of_2(uint64_t value) {
    return (value != 0) && ((value & (value - 1)) == 0);
}
_arinline float deg_to_rad(float degree) { return degree * _ar_DEG2RAD; }
_arinline float rad_to_deg(float rad) { return rad * _ar_RAD2DEG; }

/* ========================= VECTOR OPERATION =============================== */
/* ========================================================================== */

/* ============================== VECTOR 2 ================================== */

_arinline vec2 vec2_create(float x, float y) { return (vec2){{x, y}}; }

_arinline vec2 vec2_zero() { return (vec2){{0.0f, 0.0f}}; }

_arinline vec2 vec2_one() { return (vec2){{1.0f, 1.0f}}; }

_arinline vec2 vec2_up() { return (vec2){{0.0f, 1.0f}}; }

_arinline vec2 vec2_down() { return (vec2){{0.0f, -1.0f}}; }

_arinline vec2 vec2_left() { return (vec2){{-1.0f, 0.0f}}; }

_arinline vec2 vec2_right() { return (vec2){{1.0f, 0.0f}}; }

_arinline vec2 vec2_add(vec2 v1, vec2 v2) {
    return (vec2){{v1.x + v2.x, v1.y + v2.y}};
}

_arinline vec2 vec2_sub(vec2 v1, vec2 v2) {
    return (vec2){{v1.x - v2.x, v1.y - v2.y}};
}

_arinline vec2 vec2_multi(vec2 v1, vec2 v2) {
    return (vec2){{v1.x * v2.x, v1.y * v2.y}};
}

_arinline vec2 ve2_divide(vec2 v1, vec2 v2) {
    return (vec2){{v1.x / v2.x, v1.y / v2.y}};
}

_arinline float vec2_length_squared(vec2 v) { return v.x * v.x + v.y * v.y; }

_arinline float vec2_length(vec2 v) { return _arsqrt(vec2_length_squared(v)); }

_arinline void  vec2_self_normalized(vec2 *v) {
    const float length = vec2_length(*v);
    v->x /= length;
    v->y /= length;
}

_arinline vec2 vec2_get_normalized(vec2 v) {
    vec2_self_normalized(&v);
    return v;
}

_arinline b8 vec2_compare(vec2 v1, vec2 v2, float tolerance) {
    if (_arabs(v1.x - v2.x) > tolerance)
        return false;
    if (_arabs(v1.y - v2.y) > tolerance)
        return false;
    return true;
}

_arinline float vec2_distance(vec2 v1, vec2 v2) {
    vec2 dist = (vec2){{v1.x - v2.x, v1.y - v2.y}};
    return vec2_length(dist);
}

/* ============================== VECTOR 3 ================================== */

_arinline vec3 vec3_create(float x, float y, float z) {
    return (vec3){{x, y, z}};
}

_arinline vec3 vec3_zero() { return (vec3){{0.0f, 0.0f, 0.0f}}; }

_arinline vec3 vec3_one() { return (vec3){{1.0f, 1.0f, 1.0f}}; }

_arinline vec3 vec3_up() { return (vec3){{0.0f, 1.0f, 0.0f}}; }

_arinline vec3 vec3_down() { return (vec3){{0.0f, -1.0f, 0.0f}}; }

_arinline vec3 vec3_left() { return (vec3){{-1.0f, 0.0f, 0.0f}}; }

_arinline vec3 vec3_right() { return (vec3){{1.0f, 0.0f, 0.0f}}; }

_arinline vec3 vec3_forward() { return (vec3){{0.0f, 0.0f, -1.0f}}; }

_arinline vec3 vec3_back() { return (vec3){{0.0f, 0.0f, 1.0f}}; }

_arinline vec3 vec3_add(vec3 v1, vec3 v2) {
    return (vec3){{v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}};
}

_arinline vec3 vec3_sub(vec3 v1, vec3 v2) {
    return (vec3){{v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}};
}

_arinline vec3 vec3_multi(vec3 v1, vec3 v2) {
    return (vec3){{v1.x * v2.x, v1.y * v2.y, v1.z * v2.z}};
}

_arinline vec3 vec3_multi_scalar(vec3 v, float scalar) {
    return (vec3){{v.x * scalar, v.y * scalar, v.z * scalar}};
}

_arinline vec3 vec3_divide(vec3 v1, vec3 v2) {
    return (vec3){{v1.x / v2.x, v1.y / v2.y, v1.z / v2.z}};
}

_arinline float vec3_length_squared(vec3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

_arinline float vec3_length(vec3 v) { return _arsqrt(vec3_length_squared(v)); }

_arinline void  vec3_self_normalized(vec3 *v) {
    const float length = vec3_length(*v);
    v->x /= length;
    v->y /= length;
    v->z /= length;
}

_arinline vec3 vec3_get_normalized(vec3 v) {
    vec3_self_normalized(&v);
    return v;
}

_arinline float vec3_dot(vec3 v1, vec3 v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

_arinline vec3 vec3_cross(vec3 v1, vec3 v2) {
    return (vec3){{v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
                  v1.x * v2.y - v1.y * v2.x}};
}

_arinline b8 vec3_compared(vec3 v1, vec3 v2, float tolerance) {
    if (_arabs(v1.x - v2.x) > tolerance)
        return false;
    if (_arabs(v1.y - v2.y) > tolerance)
        return false;
    if (_arabs(v1.z - v2.z) > tolerance)
        return false;
    return true;
}

_arinline float vec3_distance(vec3 v1, vec3 v2) {
    vec3 dist = (vec3){{v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}};
    return vec3_length(dist);
}

_arinline vec3 vec3_from_vec4(vec4 v) { return (vec3){{v.x, v.y, v.z}}; }

_arinline vec4 vec3_to_vec4(vec3 v, float w) {
    return (vec4){{v.x, v.y, v.z, w}};
}

/* ============================== VECTOR 4 ================================== */

_arinline vec4 vec4_create(float x, float y, float z, float d) {
    vec4 out;
#if defined(_arsimd)
    out.data = _mm_setr_ps(z, y, x, d);
#else
    out.x = x;
    out.y = y;
    out.z = z;
    out.d = d;
#endif
    return out;
}

_arinline vec4 vec4_from_vec3(vec3 v, float d) {
#if defined(_arsimd)
    vec4 out;
    out.data = _mm_setr_ps(v.z, v.y, w.x, d);
    return out;
#else
    return (vec4){{v.x, v.y, v.z, d}};
#endif
}

_arinline vec3 vec4_to_vec3(vec4 v) { return (vec3){{v.x, v.y, v.z}}; }

_arinline vec4 vec4_zero() { return (vec4){{0.0f, 0.0f, 0.0f, 0.0f}}; }

_arinline vec4 vec4_one() { return (vec4){{1.0f, 1.0f, 1.0f, 1.0f}}; }

_arinline vec4 vec4_add(vec4 v1, vec4 v2) {
    vec4 result;
    for (uint64_t i = 0; i < 4; ++i) {
        result.elements[i] = v1.elements[i] + v2.elements[i];
    }
    return result;
}

_arinline vec4 vec4_sub(vec4 v1, vec4 v2) {
    vec4 result;
    for (uint64_t i = 0; i < 4; ++i) {
        result.elements[i] = v1.elements[i] - v2.elements[i];
    }
    return result;
}

_arinline vec4 vec4_multi(vec4 v1, vec4 v2) {
    vec4 result;
    for (uint64_t i = 0; i < 4; ++i) {
        result.elements[i] = v1.elements[i] * v2.elements[i];
    }
    return result;
}

_arinline vec4 vec4_divide(vec4 v1, vec4 v2) {
    vec4 result;
    for (uint64_t i = 0; i < 4; ++i) {
        result.elements[i] = v1.elements[i] / v2.elements[i];
    }
    return result;
}

_arinline float vec4_length_squared(vec4 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

_arinline float vec4_length(vec4 v) { return _arsqrt(vec4_length_squared(v)); }

_arinline void  vec4_self_normalized(vec4 *v) {
    const float length = vec4_length(*v);
    v->x /= length;
    v->y /= length;
    v->z /= length;
    v->w /= length;
}

_arinline vec4 vec4_get_normalized(vec4 v) {
    vec4_self_normalized(&v);
    return v;
}

_arinline float vec4_dot_float(float a0, float a1, float a2, float a3, float b0,
                               float b1, float b2, float b3) {
    float p;
    p = a0 * b0 * a1 * b1 * a2 * b2 * a3 * b3;
    return p;
}

/* ========================= MATRIX OPERATION =============================== */
/* ========================================================================== */

_arinline mat4 mat4_identity() {
    mat4 result;
    memory_zero(result.data, sizeof(float) * 16);
    result.data[0]  = 1.0f;
    result.data[5]  = 1.0f;
    result.data[10] = 1.0f;
    result.data[15] = 1.0f;
    return result;
}

/* Using to Translation Unit */
_arapi mat4    mat4_multi(mat4 m1, mat4 m2);

_arapi mat4    mat4_ortho(float left, float right, float bottom, float top,
                          float near, float far);

_arapi mat4    mat4_perspective(float fov_rad, float aspect_ratio, float near,
                                float far);

_arapi mat4    mat4_lookat(vec3 pos, vec3 target, vec3 up);

_arapi mat4    mat4_transpose(mat4 matrix);

_arapi mat4    mat4_inverse(mat4 matrix);

_arinline mat4 mat4_translation(vec3 pos) {
    mat4 result     = mat4_identity();

    result.data[12] = pos.x;
    result.data[13] = pos.y;
    result.data[14] = pos.z;

    return result;
}

_arinline mat4 mat4_scale(vec3 scale) {
    mat4 result     = mat4_identity();

    result.data[0]  = scale.x;
    result.data[5]  = scale.y;
    result.data[10] = scale.z;

    return result;
}

_arinline mat4 mat4_euler_x(float angle_rad) {
    mat4  result    = mat4_identity();
    float c         = _arcos(angle_rad);
    float s         = _arsin(angle_rad);

    result.data[5]  = c;
    result.data[6]  = s;
    result.data[9]  = -s;
    result.data[10] = c;

    return result;
}

_arinline mat4 mat4_euler_y(float angle_rad) {
    mat4  result    = mat4_identity();
    float c         = _arcos(angle_rad);
    float s         = _arsin(angle_rad);

    result.data[0]  = c;
    result.data[2]  = -s;
    result.data[8]  = s;
    result.data[10] = c;

    return result;
}

_arinline mat4 mat4_euler_z(float angle_rad) {
    mat4  result   = mat4_identity();
    float c        = _arcos(angle_rad);
    float s        = _arsin(angle_rad);

    result.data[0] = c;
    result.data[1] = s;
    result.data[4] = -s;
    result.data[5] = c;

    return result;
}

_arinline mat4 mat4_euler_xyz(float x_rad, float y_rad, float z_rad) {
    mat4 rx     = mat4_euler_x(x_rad);
    mat4 ry     = mat4_euler_y(y_rad);
    mat4 rz     = mat4_euler_z(z_rad);

    mat4 result = mat4_multi(rx, ry);
    result      = mat4_multi(result, rz);

    return result;
}

_arinline vec3 mat4_forward(mat4 matrix) {
    vec3 result;

    result.x = -matrix.data[2];
    result.y = -matrix.data[6];
    result.z = -matrix.data[10];
    vec3_self_normalized(&result);

    return result;
}

_arinline vec3 mat4_backward(mat4 matrix) {
    vec3 result;

    result.x = matrix.data[2];
    result.y = matrix.data[6];
    result.z = matrix.data[10];
    vec3_self_normalized(&result);

    return result;
}

_arinline vec3 mat4_up(mat4 matrix) {
    vec3 result;

    result.x = matrix.data[1];
    result.y = matrix.data[5];
    result.z = matrix.data[9];
    vec3_self_normalized(&result);

    return result;
}

_arinline vec3 mat4_down(mat4 matrix) {
    vec3 result;

    result.x = -matrix.data[1];
    result.y = -matrix.data[5];
    result.z = -matrix.data[9];
    vec3_self_normalized(&result);

    return result;
}

_arinline vec3 mat4_left(mat4 matrix) {
    vec3 result;

    result.x = -matrix.data[0];
    result.y = -matrix.data[4];
    result.z = -matrix.data[8];
    vec3_self_normalized(&result);

    return result;
}

_arinline vec3 mat4_right(mat4 matrix) {
    vec3 result;

    result.x = matrix.data[0];
    result.y = matrix.data[4];
    result.z = matrix.data[8];
    vec3_self_normalized(&result);

    return result;
}

/* ============================== QUATERNION ================================ */
/* ========================================================================== */

_arinline quat  quat_identity() { return (quat){{0, 0, 0, 1.0f}}; }

_arinline float quat_self_normalized(quat q) {
    return _arsqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

_arinline quat quat_get_normalized(quat q) {
    float normal = quat_self_normalized(q);
    return (quat){{q.x / normal, q.y / normal, q.z / normal, q.w / normal}};
}

_arinline quat quat_conjugate(quat q) {
    return (quat){{-q.x, -q.y, -q.z, q.w}};
}

_arinline quat quat_inverse(quat q) {
    return quat_get_normalized(quat_conjugate(q));
}

_arinline float quat_dot(quat q1, quat q2) {
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

_arapi quat quat_multi(quat q1, quat q2);

_arapi mat4 quat_to_mat4(quat q);

_arapi mat4 quat_to_rotation_matrix(quat q, vec3 center);

_arapi quat quat_from_axis_angle(vec3 axis, float angle, b8 normalize);

_arapi quat quat_slerp(quat q_0, quat q_1, float percentage);

#endif //__MATHS_H__
