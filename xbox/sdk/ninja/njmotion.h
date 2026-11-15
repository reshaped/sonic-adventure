/* the Xbox port's weighted_sum, weighted_sum_angles and squared_length:
 * the static functions that njMotSrc.c, njMotLinkSrc.c, njCameraMotion.c
 * and njMotionE.c define for SHC in inline assembly, as C of those bodies.
 * each of those sources includes this under XBOX, then declares the ones it
 * calls. each definition has the type those declarations give */
#ifndef XBOX_NJMOTION_H
#define XBOX_NJMOTION_H
#include <ninja.h>
#include <sg_xpt.h>

#include "xbox/include/sh4.h"

/* out = wa * a + wb * b + wc * c + wd * d, a to d three Floats each: per
 * component a FIPR FV4,FV0 of the weights with that component of a to d,
 * the weights first. all three are formed before out is stored, z first */
ALWAYS_INLINE void weighted_sum(Float *a, Float *b, Float *c, Float *d,
                                Float wa, Float wb, Float wc, Float wd,
                                Float *out)
{
    Float x = sh4_fipr(wa, wb, wc, wd, a[0], b[0], c[0], d[0]);
    Float y = sh4_fipr(wa, wb, wc, wd, a[1], b[1], c[1], d[1]);
    Float z = sh4_fipr(wa, wb, wc, wd, a[2], b[2], c[2], d[2]);

    out[2] = z;
    out[1] = y;
    out[0] = x;
}

/* weighted_sum for Angles: each converted by FLOAT, each sum truncated back
 * by FTRC, which saturates */
ALWAYS_INLINE void weighted_sum_angles(Angle *a, Angle *b, Angle *c, Angle *d,
                                       Float wa, Float wb, Float wc, Float wd,
                                       Angle *out)
{
    Float x = sh4_fipr(wa, wb, wc, wd, (Float)a[0], (Float)b[0], (Float)c[0],
                       (Float)d[0]);
    Float y = sh4_fipr(wa, wb, wc, wd, (Float)a[1], (Float)b[1], (Float)c[1],
                       (Float)d[1]);
    Float z = sh4_fipr(wa, wb, wc, wd, (Float)a[2], (Float)b[2], (Float)c[2],
                       (Float)d[2]);

    out[2] = (Angle)sh4_ftrc(z);
    out[1] = (Angle)sh4_ftrc(y);
    out[0] = (Angle)sh4_ftrc(x);
}

/* x * x + y * y + z * z + w * w, by the shipped FIPR FV4,FV4 */
ALWAYS_INLINE Float squared_length(Float x, Float y, Float z, Float w)
{
    return sh4_fipr_self(x, y, z, w);
}

#endif
