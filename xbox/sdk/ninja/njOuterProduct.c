/* njOuterProduct: ov = v1 x v2, and its length. make pc compiles this in
 * place of sdk/ninja/njOuterProduct.c, whose body is SH-4 assembly. each
 * part is a product, then an FMAC of the other, negated, as the body runs
 * them. v1 and v2 are read whole before ov is written, so ov may be either */
#include <ninja.h>

#include "xbox/include/sh4.h"

Float(njOuterProduct)(NJS_VECTOR *v1, NJS_VECTOR *v2, NJS_VECTOR *ov)
{
    Float x1 = v1->x;
    Float y1 = v1->y;
    Float z1 = v1->z;
    Float x2 = v2->x;
    Float y2 = v2->y;
    Float z2 = v2->z;
    Float x = sh4_fmac(-z1, y2, y1 * z2);
    Float y = sh4_fmac(x1, -z2, z1 * x2);
    Float z = sh4_fmac(y1, -x2, x1 * y2);

    ov->x = x;
    ov->y = y;
    ov->z = z;
    return __builtin_sqrtf(sh4_fipr(x, y, z, 0.0F, x, y, z, 0.0F));
}
