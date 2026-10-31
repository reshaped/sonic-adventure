/* GCC's weighted_sum, weighted_sum_angles and squared_length: the static
 * functions that njMotSrc.c, njMotLinkSrc.c, njCameraMotion.c and njMotionE.c
 * define for SHC in inline assembly. each of those sources includes this
 * under GCC, then declares the ones it calls. each definition has the type
 * those declarations give, so no pointer is const */
#ifndef NJMOTION_ZAM_H
#define NJMOTION_ZAM_H
#include <ninja.h>
#include <sg_xpt.h>

/* the asm runs the shipped body's loads, FIPRs and stores in its order, and
 * each FIPR is the shipped FIPR FV4,FV0. SH4ZAM's shz_vec4_dot3 would compute
 * the same three dot products from vectors loaded in C. FMOV.S has no
 * displacement form, so GCC would then compute the addresses of the second
 * and third Float of each of a, b, c and d, where the asm's loads
 * post-increment each pointer */
ALWAYS_INLINE void weighted_sum(Float *a, Float *b, Float *c, Float *d,
                                Float wa, Float wb, Float wc, Float wd,
                                Float *out)
{
    register Float weight_a __asm__("fr4") = wa;
    register Float weight_b __asm__("fr5") = wb;
    register Float weight_c __asm__("fr6") = wc;
    register Float weight_d __asm__("fr7") = wd;
    Float x;
    Float y;

    __asm__("fmov.s @%[a]+,fr0\n\t"
            "fmov.s @%[b]+,fr1\n\t"
            "fmov.s @%[c]+,fr2\n\t"
            "fmov.s @%[d]+,fr3\n\t"
            "fipr fv4,fv0\n\t"
            "fmov fr3,%[x]\n\t"
            "fmov.s @%[a]+,fr0\n\t"
            "fmov.s @%[b]+,fr1\n\t"
            "fmov.s @%[c]+,fr2\n\t"
            "fmov.s @%[d]+,fr3\n\t"
            "fipr fv4,fv0\n\t"
            "fmov fr3,%[y]\n\t"
            "fmov.s @%[a]+,fr0\n\t"
            "fmov.s @%[b]+,fr1\n\t"
            "fmov.s @%[c]+,fr2\n\t"
            "fmov.s @%[d]+,fr3\n\t"
            "fipr fv4,fv0\n\t"
            "add #12,%[out]\n\t"
            "fmov.s fr3,@-%[out]\n\t"
            "fmov.s %[y],@-%[out]\n\t"
            "fmov.s %[x],@-%[out]"
            : [a] "+r"(a), [b] "+r"(b), [c] "+r"(c), [d] "+r"(d),
              [out] "+r"(out), [x] "=&f"(x), [y] "=&f"(y),
              "=m"(*(Float(*)[3])out)
            : "f"(weight_a), "f"(weight_b), "f"(weight_c), "f"(weight_d),
              "m"(*(const Float(*)[3])a), "m"(*(const Float(*)[3])b),
              "m"(*(const Float(*)[3])c), "m"(*(const Float(*)[3])d)
            : "fr0", "fr1", "fr2", "fr3");
}

/* weighted_sum for Angles, in C: MOV.L has the displacement form FMOV.S
 * lacks, so GCC loads the second and third Angle of each of a, b, c and d
 * without an add */
ALWAYS_INLINE void weighted_sum_angles(Angle *a, Angle *b, Angle *c, Angle *d,
                                       Float wa, Float wb, Float wc, Float wd,
                                       Angle *out)
{
    shz_vec4_t weights = shz_vec4_init(wa, wb, wc, wd);
    Angle x = (Angle)zam_fipr(weights, shz_vec4_init((Float)a[0], (Float)b[0],
                                                     (Float)c[0], (Float)d[0]));
    Angle y = (Angle)zam_fipr(weights, shz_vec4_init((Float)a[1], (Float)b[1],
                                                     (Float)c[1], (Float)d[1]));
    Angle z = (Angle)zam_fipr(weights, shz_vec4_init((Float)a[2], (Float)b[2],
                                                     (Float)c[2], (Float)d[2]));

    out[0] = x;
    out[1] = y;
    out[2] = z;
}

ALWAYS_INLINE Float squared_length(Float x, Float y, Float z, Float w)
{
    return zam_fipr_self(x, y, z, w);
}

#endif
