/* njScalor: v's length, FSQRT of FIPR FV4,FV4 of (v, 0). make pc compiles
 * this in place of sdk/ninja/njScalor.c, whose body is SH-4 assembly */
#include <ninja.h>

#include "xbox/include/sh4.h"

Float(njScalor)(NJS_VECTOR *v)
{
    return __builtin_sqrtf(sh4_fipr_self(v->x, v->y, v->z, 0.0F));
}
