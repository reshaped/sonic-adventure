/* C runtime float math, as far as the sources use it */
#ifndef MATHF_H
#define MATHF_H

float acosf(float x);
float asinf(float x);
float atan2f(float y, float x);
float ceilf(float x);
float floorf(float x);
float modff(float value, float *iptr);
float powf(float x, float y);
#ifdef DREAMCAST
#define sqrtf _builtin_sqrtf
#else
float sqrtf(float x);
#endif
#ifdef DREAMCAST
#ifdef NONMATCHING
#define _builtin_sqrtf __builtin_sqrtf
#include <zam.h>
#include <sh4zam/shz_scalar.h>

/* on this side, a C call of floorf or ceilf runs zam_floorf or zam_ceilf
 * inline. a large x here is one outside int32_t's range and below 2^32 in
 * magnitude. at run time, zam_floorf and zam_ceilf return the runtime's
 * result, but for a NaN or a large x. they return either as it is. the
 * runtime's return 2^31 - 128 for a large positive x and -2^31 for a large
 * negative one. their FTRC saturates, and FLOAT rounds 2^31 - 1 toward
 * zero, as the console's FPSCR.RM selects. the runtime's floorf gives the
 * default NaN for a NaN with its sign set. its ceilf does so for one with
 * its sign clear. neither zam_floorf nor zam_ceilf sets errno, which the
 * runtime's set for a NaN or an infinity. for a constant x, GCC folds the
 * call to the exact floor or ceiling.
 *
 * a float of magnitude 2^23 or more is an integer. so it is its own floor
 * and ceiling, as an infinity or a NaN is. shz_floorf and shz_ceilf round
 * any smaller x by FTRC and FLOAT. FLOAT's zero is +0, but the runtime's
 * zero has x's sign, as C's does. floorf(-0.0F) and ceilf(-0.5F) give -0, so
 * a zero result here is x * 0.0F. the console runs with FPSCR.DN set, so the
 * FPU takes a denormal as a zero of its sign. here as in the runtime's, a
 * negative denormal floors to -0 and a positive one ceils to +0 */
ALWAYS_INLINE float zam_floorf(float x)
{
    float result;

    if (!(__builtin_fabsf(x) < 0x1p23F)) {
        return x;
    }
    result = shz_floorf(x);
    return result == 0.0F ? x * 0.0F : result;
}
ZAM_ASSERT_COMPATIBLE(floorf);
#define floorf(x) zam_floorf(x)

ALWAYS_INLINE float zam_ceilf(float x)
{
    float result;

    if (!(__builtin_fabsf(x) < 0x1p23F)) {
        return x;
    }
    result = shz_ceilf(x);
    return result == 0.0F ? x * 0.0F : result;
}
ZAM_ASSERT_COMPATIBLE(ceilf);
#define ceilf(x) zam_ceilf(x)
#endif
#endif

#endif
