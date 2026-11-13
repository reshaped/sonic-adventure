/* njOuterProduct on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njOuterProduct.c */
#include <ninja.h>

/* ov = v1 x v2. each component is one FMAC of a product and a negated one,
 * as in the shipped body. v1 and v2 are read whole before ov is written, so ov
 * may be either. returns the length of ov */
Float njOuterProduct(NJS_VECTOR *v1, NJS_VECTOR *v2, NJS_VECTOR *ov)
{
    shz_vec3_t a = zam_load_vec3(v1);
    shz_vec3_t b = zam_load_vec3(v2);
    shz_vec3_t out = shz_vec3_init(__builtin_fmaf(-a.z, b.y, a.y * b.z),
                                   __builtin_fmaf(a.x, -b.z, a.z * b.x),
                                   __builtin_fmaf(a.y, -b.x, a.x * b.y));

    zam_store_vec3(ov, out);
    return __builtin_sqrtf(zam_squared_length(out));
}
