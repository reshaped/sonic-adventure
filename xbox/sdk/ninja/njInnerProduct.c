/* njInnerProduct: v1 . v2. make pc compiles this in place of
 * sdk/ninja/njInnerProduct.c, whose body is SH-4 assembly: FIPR FV4,FV0 of
 * (v2, 0) and (v1, 0), v2 first */
#include <ninja.h>

#include "xbox/include/sh4.h"

Float(njInnerProduct)(NJS_VECTOR *v1, NJS_VECTOR *v2)
{
    return sh4_fipr(v2->x, v2->y, v2->z, 0.0F, v1->x, v1->y, v1->z, 0.0F);
}
