/* models of SH-4 FPU instructions for the port's C, where the original
 * assembly runs them and C has no operator that gives their result, and
 * the port's XMTRX, the matrix the console keeps in the FPU's back bank.
 * ninja.h includes this under XBOX, so every source that takes Ninja's
 * header has them, as the console's code has the instructions.
 *
 * each assumes the console's FPU mode, which the port's startup sets on
 * x86: rounding toward zero and denormals as zero, for SSE in MXCSR and
 * for the x87 in its control word, which runs the doubles */
#ifndef XBOX_SH4_H
#define XBOX_SH4_H
#include <sg_xpt.h>
/* the types the models take. ninja.h includes this at its end, where its
 * own guard keeps this include from reading it again */
#include <ninja.h>
#include "sse/sh4_sse.h"

/* a helper that inlines at every call, as zam.h defines it for make gcc */
#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE static inline __attribute__((always_inline))
#endif

/* FTRC: f truncated toward 0, saturated to Sint32's range as the SH-4's
 * FTRC saturates. a NaN gives what a positive overflow gives */
static inline Sint32 sh4_ftrc(Float f)
{
    if (f >= 2147483648.0F || f != f) {
        return 0x7fffffff;
    }
    if (f < -2147483648.0F) {
        return (Sint32)0x80000000;
    }
    return (Sint32)f;
}

/* the 64-bit double, which make pc's double, a float where SHC's flags make
 * it one, leaves alone */
typedef float sh4_double __attribute__((mode(DF)));

/* FMAC: a * b + c, rounded once. the product of two floats is exact as a
 * double, and rounding toward zero twice, to a double and then to a float,
 * gives what rounding once does */
static inline Float sh4_fmac(Float a, Float b, Float c)
{
    return (Float)((sh4_double)a * (sh4_double)b + (sh4_double)c);
}

/* FIPR. on the Xbox and the PC, Flycast's: the four products as doubles,
 * summed in order, rounded once toward zero (sh4_sse.h). elsewhere, the
 * port's tests' model: the sum of the four products in order, each
 * rounded, then each sum */
static inline Float sh4_fipr(Float a0, Float a1, Float a2, Float a3, Float b0,
                             Float b1, Float b2, Float b3)
{
#ifdef XBOX
    return sh4_sse_fipr(a0, a1, a2, a3, b0, b1, b2, b3);
#else
    return a0 * b0 + a1 * b1 + a2 * b2 + a3 * b3;
#endif
}

/* FIPR FVm,FVm of (x, y, z, w) with itself, as njScalor's FIPR FV4,FV4 */
static inline Float sh4_fipr_self(Float x, Float y, Float z, Float w)
{
    return sh4_fipr(x, y, z, w, x, y, z, w);
}

/* XMTRX as XF0 to XF15: column j is sh4_xmtrx[4j] to sh4_xmtrx[4j + 3]. one
 * for the whole game, in 1ST_READ.BIN's image (xbox/port/xmtrx.c), which an
 * overlay's unbound name binds to; the tests' in xbox/test/libm.c */
extern Float sh4_xmtrx[16];

static inline void sh4_xmtrx_get(Float xf[16])
{
    int k;

    for (k = 0; k < 16; k++) {
        xf[k] = sh4_xmtrx[k];
    }
}

static inline void sh4_xmtrx_set(const Float xf[16])
{
    int k;

    for (k = 0; k < 16; k++) {
        sh4_xmtrx[k] = xf[k];
    }
}

/* FTRV of v by the matrix whose XF0 to XF15 are xf: out[i] is the sum over
 * j of xf[i + 4j] * v[j], in order of j. v is read whole before out is
 * written, so the two may be one */
static inline void sh4_ftrv(const Float xf[16], const Float v[4], Float out[4])
{
#ifdef XBOX
    sh4_sse_ftrv(xf, v, out);
#else
    Float in[4];
    int i;

    for (i = 0; i < 4; i++) {
        in[i] = v[i];
    }
    for (i = 0; i < 4; i++) {
        out[i] = xf[i] * in[0] + xf[i + 4] * in[1] + xf[i + 8] * in[2] + xf[i + 12] * in[3];
    }
#endif
}

/* FTRV XMTRX,FVn */
static inline void sh4_xmtrx_ftrv(const Float v[4], Float out[4])
{
    sh4_ftrv(sh4_xmtrx, v, out);
}

/* FTRV XMTRX of (p, w): w 1 for a point, 0 for a vector. out takes the
 * first three of the four results */
static inline void sh4_xmtrx_apply3(const NJS_POINT3 *p, Float w, NJS_POINT3 *out)
{
    Float v[4];

    v[0] = p->x;
    v[1] = p->y;
    v[2] = p->z;
    v[3] = w;
    sh4_xmtrx_ftrv(v, v);
    out->x = v[0];
    out->y = v[1];
    out->z = v[2];
}

/* FSRRA: 1 / sqrt(n). the console's is an estimate; this is exact. a
 * negative n gives a NaN, as the SH-4's invalid operation does, and +0 an
 * infinity */
static inline Float sh4_fsrra(Float n)
{
    return 1.0F / __builtin_sqrtf(n);
}

/* FSCA: the sine and cosine of a 16-bit angle, 65536 to a turn, by the
 * console's own table (xbox/port/math.c; the tests' model in
 * xbox/test/libm.c) */
struct sh4_sincos {
    Float sin, cos;
};
struct sh4_sincos port_sincos16(unsigned short angle);

/* SHC's intrinsics for the two, which the game's C calls */
static inline Float _builtin_fsrra(Float n)
{
    return sh4_fsrra(n);
}

/* FSCA reads only FPUL's low 16 bits */
static inline void _builtin_fsca(Angle ang, Float *sin, Float *cos)
{
    struct sh4_sincos pair = port_sincos16((unsigned short)ang);

    *sin = pair.sin;
    *cos = pair.cos;
}

#endif
