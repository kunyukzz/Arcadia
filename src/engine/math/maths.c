#include "engine/math/maths.h"

#include "engine/platform/platform_time.h"
#include "engine/core/assertion.h"

#include <math.h>

#if _AR_USE_SIMD
    #if defined(_MSC_VER)
        #include <intrin.h>
    #else
        #include <xmmintrin.h>
    #endif
#endif

#define srcX 0
#define srcY 1
#define srcZ 2
#define srcW 3

/* ========================= INTERNAL USING M128 ============================ */
/* ========================================================================== */
static __m128 _vec_load(const void *v, uint32_t size) {
    if (size == 3) {
        const vec3 *v3 = (const vec3 *)v;
        return _mm_set_ps(v3->_pad1, v3->z, v3->y,
                          v3->x);
    } else if (size == 4) {
        const vec4 *v4 = (const vec4 *)v;
        return _mm_set_ps(v4->w, v4->z, v4->y, v4->x);
    }

    return _mm_setzero_ps();
    ar_assert(size == 3 || size == 4);
}

static void _vec_store(void *out, __m128 in, uint32_t size) {
    if (size == 3) {
        vec3 *v3 = (vec3 *)out;
        _mm_store_ss(&v3->x, in);
        _mm_store_ss(&v3->y,
                     _mm_shuffle_ps(in, in, _MM_SHUFFLE(1, 1, 1, 1)));
        _mm_store_ss(&v3->z,
                     _mm_shuffle_ps(in, in, _MM_SHUFFLE(2, 2, 2, 2)));
        _mm_store_ss(&v3->_pad1,
                     _mm_shuffle_ps(in, in, _MM_SHUFFLE(3, 3, 3, 3)));
    } else if (size == 4) {
        vec4 *v4 = (vec4 *)out;
        _mm_store_ss(&v4->x, in);
        _mm_store_ss(&v4->y,
                     _mm_shuffle_ps(in, in, _MM_SHUFFLE(1, 1, 1, 1)));
        _mm_store_ss(&v4->z,
                     _mm_shuffle_ps(in, in, _MM_SHUFFLE(2, 2, 2, 2)));
        _mm_store_ss(&v4->w,
                     _mm_shuffle_ps(in, in, _MM_SHUFFLE(3, 3, 3, 3)));
    }
}

/* ========================================================================== */
/* ========================================================================== */
static b8 random_seed = false;

float _ar_sinf(float x) { return sinf(x); }
float _ar_cosf(float x) { return cosf(x); }
float _ar_tanf(float x) { return tanf(x); }
float _ar_acosf(float x) { return acosf(x); }
float _ar_sqrtf(float x) { return sqrtf(x); }
float _ar_absf(float x) { return fabsf(x); }

int32_t _ar_random(void) {
	if (!random_seed) {
		srand((uint32_t)get_absolute_time());
		random_seed = true;
	}
	return rand();
}

int32_t _ar_random_in_range(int32_t min, int32_t max) {
	if (!random_seed) {
		srand((uint32_t)get_absolute_time());
		random_seed = true;
	}
	return (rand() % (max - min + 1)) + min;
}

float _ar_frandom(void) {
	return (float)_ar_random() / (float)RAND_MAX;
}

float _ar_frandom_in_range(float min, float max) {
    return min + ((float)_ar_random() / ((float)RAND_MAX / (max / min)));
}

/* ========================= VECTOR OPERATION =============================== */
/* ========================================================================== */

vec3 svec3_add(vec3 v1, vec3 v2) {
    __m128 va = _vec_load(&v1, sizeof(v1));
    __m128 vb = _vec_load(&v2, sizeof(v2));
    __m128 vc = _mm_add_ps(va, vb);

    vec3   result;
    _vec_store(&result, vc, sizeof(result));
    return result;
}

vec3 svec3_sub(vec3 v1, vec3 v2) {
	__m128 va = _vec_load(&v1, sizeof(v1));	
	__m128 vb = _vec_load(&v2, sizeof(v2));
	__m128 vc = _mm_sub_ps(va, vb);

	vec3 result;
	_vec_store(&result, vc, sizeof(result));
	return result;
}

vec3 svec3_multi(vec3 v1, vec3 v2) {
	__m128 va = _vec_load(&v1, sizeof(v1));
	__m128 vb = _vec_load(&v2, sizeof(v2));
	__m128 vc = _mm_mul_ps(va, vb);

	vec3 result;
	_vec_store(&result, vc, sizeof(result));
	return result;
}

vec3 svec3_divide(vec3 v1, vec3 v2) {
	__m128 va = _vec_load(&v1, sizeof(v1));
	__m128 vb = _vec_load(&v2, sizeof(v2));
	__m128 vc = _mm_div_ps(va, vb);

	vec3 result;
	_vec_store(&result, vc, sizeof(result));
	return result;
}

vec3 svec3_multi_scalar(vec3 v, float s) {
	__m128 va = _vec_load(&v, sizeof(v));
	__m128 vs = _mm_set1_ps(s);
	__m128 vc = _mm_mul_ps(va, vs);

	vec3 result;
	_vec_store(&result, vc, sizeof(result));
	return result;
}

float svec3_length_square(vec3 v) {
	__m128 va = _vec_load(&v, sizeof(v));
	__m128 square = _mm_mul_ps(va, va);

    __m128 shuf1 =
        _mm_shuffle_ps(square, square, _MM_SHUFFLE(srcZ, srcY, srcX, srcW));
    __m128 sum   = _mm_add_ps(square, shuf1);

    __m128 shuf2 = _mm_movehl_ps(sum, sum);
	sum = _mm_add_ss(sum, shuf2);

	return _mm_cvtss_f32(sum);
}

float svec3_length(vec3 v) {
	return	_ar_sqrtf(vec3_length_square(v));
}

float svec3_dot(vec3 v1, vec3 v2) {
	__m128 va = _vec_load(&v1, sizeof(v1));
	__m128 vb = _vec_load(&v2, sizeof(v2));
	__m128 multi = _mm_mul_ps(va, vb);

    __m128 shuf1 =
        _mm_shuffle_ps(multi, multi, _MM_SHUFFLE(srcZ, srcY, srcX, srcW));
    __m128 sum    = _mm_add_ss(multi, shuf1);

    __m128 shuf2 = _mm_movehl_ps(sum, sum);
	__m128 result = _mm_add_ss(sum, shuf2);

	return _mm_cvtss_f32(result);
}

vec3 svec3_cross(vec3 v1, vec3 v2) {
	__m128 va = _vec_load(&v1, sizeof(v1));	
	__m128 vb = _vec_load(&v2, sizeof(v2));

	__m128 va_yzx = _mm_shuffle_ps(va, va, _MM_SHUFFLE(srcW, srcX, srcZ, srcY));
	__m128 vb_yzx = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(srcW, srcX, srcZ, srcY));

	__m128 mul1 = _mm_mul_ps(va, vb_yzx);
	__m128 mul2 = _mm_mul_ps(va_yzx, vb);

	__m128 result = _mm_sub_ps(mul1, mul2);

	vec3 out;
	_vec_store(&out, result, sizeof(out));

	return out;
}





/* ========================= MATRIX OPERATION =============================== */
/* ========================================================================== */


/* ============================== QUATERNION ================================ */
/* ========================================================================== */


