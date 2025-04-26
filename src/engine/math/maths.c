#include "engine/math/maths.h"

#include "engine/platform/platform_time.h"

#include <math.h>

static b8 random_seeded = false;

float     _arsin(float x) { return sinf(x); }
float     _arcos(float x) { return cosf(x); }
float     _artan(float x) { return tanf(x); }
float     _aracos(float x) { return acosf(x); }
float     _arsqrt(float x) { return sqrtf(x); }
float     _arabs(float x) { return fabsf(x); }

int32_t   _ar_random() {
    if (!random_seeded) {
        srand((uint32_t)get_absolute_time());
        random_seeded = true;
    }
    return rand();
}

int32_t _ar_random_in_range(int32_t min, int32_t max) {
    if (!random_seeded) {
        srand((uint32_t)get_absolute_time());
        random_seeded = true;
    }
    return (rand() % (max - min + 1) + max);
}

float _ar_frandom() { return (float)_ar_random() / (float)RAND_MAX; }

float _ar_frandom_in_range(float min, float max) {
    return min + ((float)_ar_random() / ((float)RAND_MAX / (max - min)));
}

/* ========================= MATRIX OPERATION =============================== */
/* ========================================================================== */

// I got this from Doom 3 Engine from Id Software. It is extremely fast!!
mat4 mat4_multi(mat4 m1, mat4 m2) {
    mat4         result  = mat4_identity();

    const float *m1_ptr  = m1.data;
    const float *m2_ptr  = m2.data;
    float       *dst_ptr = result.data;

    for (int32_t i = 0; i < 4; ++i) {
        for (int32_t j = 0; j < 4; ++j) {
            *dst_ptr = m1_ptr[0] * m2_ptr[0 + j] + m1_ptr[1] * m2_ptr[4 + j] +
                       m1_ptr[2] * m2_ptr[8 + j] + m1_ptr[3] * m2_ptr[12 + j];
            dst_ptr++;
        }
        m1_ptr += 4;
    }
    return result;
}

mat4 mat4_ortho(float left, float right, float bottom, float top, float near,
                float far) {
    mat4  result    = mat4_identity();

    float lr        = 1.0f / (left - right);
    float bt        = 1.0f / (bottom - top);
    float nf        = 1.0f / (near - far);

    result.data[0]  = -2.0f * lr;
    result.data[5]  = -2.0f * bt;
    result.data[10] = 2.0f * nf;

    result.data[12] = (left + right) * lr;
    result.data[13] = (top + bottom) * bt;
    result.data[14] = (far + near) * nf;

    return result;
}

mat4 mat4_perspective(float fov_rad, float aspect_ratio, float near,
                      float far) {
    float half_tan_fov = _artan(fov_rad * 0.5f);
    mat4  result;
    memory_zero(result.data, sizeof(float) * 16);

    result.data[0]  = 1.0f / (aspect_ratio * half_tan_fov);
    result.data[5]  = 1.0f / half_tan_fov;
    result.data[10] = -((far + near) / (far - near));
    result.data[11] = -1.0f;
    result.data[14] = -((2.0f * far * near) / (far - near));

    return result;
}

mat4 mat4_lookat(vec3 pos, vec3 target, vec3 up) {
    mat4 result;
    vec3 z_axis;
    z_axis.x        = target.x - pos.x;
    z_axis.y        = target.y - pos.y;
    z_axis.z        = target.z - pos.z;

    z_axis          = vec3_get_normalized(z_axis);
    vec3 x_axis     = vec3_get_normalized(vec3_cross(z_axis, up));
    vec3 y_axis     = vec3_cross(x_axis, z_axis);

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

    return result;
}

mat4 mat4_transpose(mat4 matrix) {
    mat4 result     = mat4_identity();

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

    return result;
}

mat4 mat4_inverse(mat4 matrix) {
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

    mat4         result;
    float       *o = result.data;

    o[0]           = (t0 * m[5] + t3 * m[9] + t4 * m[13]) -
           (t1 * m[5] + t2 * m[9] + t5 * m[13]);
    o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) -
           (t0 * m[1] + t7 * m[9] + t8 * m[13]);
    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) -
           (t3 * m[1] + t6 * m[5] + t11 * m[13]);
    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) -
           (t4 * m[1] + t9 * m[5] + t10 * m[9]);

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

    return result;
}

/* ============================== QUATERNION ================================ */
/* ========================================================================== */

quat quat_multi(quat q1, quat q2) {
    quat result;

    result.x = q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.z * q1.x;
    result.y = -q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y;
    result.z = q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z;
    result.w = -q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w;

    return result;
}

mat4 quat_to_mat4(quat q) {
    mat4 result;

    /* https://stackoverflow.com/questions/1556260/convert-quaternion-rotation-to-rotation-matrix
     */
    quat n          = quat_get_normalized(q);
    result.data[0]  = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
    result.data[1]  = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
    result.data[2]  = 2.0f * n.x * n.z + 2.0f * n.y * n.w;
    result.data[4]  = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
    result.data[5]  = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
    result.data[6]  = 2.0f * n.y * n.z - 2.0f * n.x * n.w;
    result.data[8]  = 2.0f * n.x * n.z - 2.0f * n.y * n.w;
    result.data[9]  = 2.0f * n.y * n.z + 2.0f * n.x * n.w;
    result.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;

    return result;
}

mat4 quat_to_rotation_matrix(quat q, vec3 center) {
    mat4   result;
    float *o = result.data;
    o[0]     = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[1]     = 2.0f * ((q.x * q.y) + (q.z * q.w));
    o[2]     = 2.0f * ((q.x * q.z) - (q.y * q.w));
    o[3]     = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];

    o[4]     = 2.0f * ((q.x * q.y) - (q.z * q.w));
    o[5]     = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[6]     = 2.0f * ((q.y * q.z) + (q.x * q.w));
    o[7]     = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];

    o[8]     = 2.0f * ((q.x * q.z) + (q.y * q.w));
    o[9]     = 2.0f * ((q.y * q.z) - (q.x * q.w));
    o[10]    = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
    o[11]    = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];

    o[12]    = 0.0f;
    o[13]    = 0.0f;
    o[14]    = 0.0f;
    o[15]    = 1.0f;

    return result;
}

quat quat_from_axis_angle(vec3 axis, float angle, b8 normalize) {
    const float half_angle = 0.5f * angle;
    float       s          = _arsin(half_angle);
    float       c          = _arcos(half_angle);
    quat        result     = (quat){{s * axis.x, s * axis.y, s * axis.z, c}};

    if (normalize)
        return quat_get_normalized(result);

    return result;
}

quat quat_slerp(quat q_0, quat q_1, float percentage) {
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
        out_quaternion = (quat){{v0.x + ((v1.x - v0.x) * percentage),
                                 v0.y + ((v1.y - v0.y) * percentage),
                                 v0.z + ((v1.z - v0.z) * percentage),
                                 v0.w + ((v1.w - v0.w) * percentage)}};
        return quat_get_normalized(out_quaternion);
    }

    // Since dot is in range [0, DOT_THRESHOLD], acos is safe
    float theta_0     = _aracos(dot);
    float theta       = theta_0 * percentage;
    float sin_theta   = _arsin(theta);
    float sin_theta_0 = _arsin(theta_0);
    float s0          = _arcos(theta) - dot * sin_theta / sin_theta_0;
    float s1          = sin_theta / sin_theta_0;

    return (quat){{(v0.x * s0) + (v1.x * s1), (v0.y * s0) + (v1.y * s1),
                   (v0.z * s0) + (v1.z * s1), (v0.w * s0) + (v1.w * s1)}};
}
