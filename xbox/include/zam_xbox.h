/* the helpers of sdk/include/ninja_zam.h that the Dreamcast engine's drawers
 * (engine/dc/) call, for the Xbox port. make dc's run the SH-4's FIPR, FSRRA
 * and FSCA through SH4ZAM's SH-4 back end. the port's run its models of them
 * (sh4.h), over the types of SH4ZAM's software back end, whose XMTRX is the
 * port's (xbox/port/xmtrx.c). sh4.h, which ninja.h includes under XBOX where
 * make dc's includes ninja_zam.h, includes this after its models. ninja_zam.h's macros, which put its
 * helpers in place of Ninja's own calls, stay make dc's: the game's calls
 * keep the port's models of Ninja's bodies (xbox/sdk/ninja/) */
#ifndef XBOX_ZAM_XBOX_H
#define XBOX_ZAM_XBOX_H
/* the C library's float math that SH4ZAM's software back end calls, declared
 * ahead of its headers: the SDK's math.h declares none of it, and C17 takes
 * no implicit declaration. an inline body of SH4ZAM's that no caller uses
 * references none */
float acosf(float x);
float asinf(float x);
float atan2f(float y, float x);
float atanf(float x);
float ceilf(float x);
float copysignf(float x, float y);
float cosf(float x);
float expf(float x);
float fabsf(float x);
float floorf(float x);
float fmaf(float x, float y, float z);
float fmodf(float x, float y);
float hypotf(float x, float y);
float logf(float x);
float powf(float x, float y);
float roundf(float x);
float sinf(float x);
float sqrtf(float x);
float tanf(float x);
float truncf(float x);
double sin(double x);
double cos(double x);
#include <zam.h>
#include <stdint.h>
#include <sh4zam/shz_trig.h>
#include <sh4zam/shz_vector.h>
#include <sh4zam/shz_xmtrx.h>

#include "sh4.h"

_Static_assert(sizeof(shz_vec3_t) == sizeof(NJS_POINT3), "shz_vec3_t stands for an NJS_POINT3");
_Static_assert(sizeof(shz_vec4_t) == 4 * sizeof(Float), "shz_vec4_t is four Floats");

/* p's x, y and z as an SH4ZAM vector */
ALWAYS_INLINE shz_vec3_t zam_load_vec3(const NJS_POINT3 *p)
{
    return shz_vec3_init(p->x, p->y, p->z);
}

/* p's x, y and z, with w as the fourth component */
ALWAYS_INLINE shz_vec4_t zam_load_vec4(const NJS_POINT3 *p, Float w)
{
    return shz_vec4_init(p->x, p->y, p->z, w);
}

/* store v in p's x, y and z */
ALWAYS_INLINE void zam_store_vec3(NJS_POINT3 *p, shz_vec3_t v)
{
    p->x = v.x;
    p->y = v.y;
    p->z = v.z;
}

/* FIPR of a and b, a as FIPR's first operand */
ALWAYS_INLINE Float zam_fipr(shz_vec4_t a, shz_vec4_t b)
{
    return sh4_fipr(a.x, a.y, a.z, a.w, b.x, b.y, b.z, b.w);
}

/* FIPR FV4,FV4: the sum of the squares of x, y, z and w */
ALWAYS_INLINE Float zam_fipr_self(Float x, Float y, Float z, Float w)
{
    return sh4_fipr_self(x, y, z, w);
}

/* the square of v's length: the FIPR of (v, 0) with itself */
ALWAYS_INLINE Float zam_squared_length(shz_vec3_t v)
{
    return zam_fipr_self(v.x, v.y, v.z, 0.0F);
}

/* FSCA's sine and cosine of ang */
ALWAYS_INLINE shz_sincos_t zam_fsca(Angle ang)
{
    Float sin;
    Float cos;
    shz_sincos_t pair;

    _builtin_fsca(ang, &sin, &cos);
    pair.sin = sin;
    pair.cos = cos;
    return pair;
}

#endif
