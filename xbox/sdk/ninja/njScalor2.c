/* njScalor2: v's squared length, FIPR FV4,FV4 of (v, 0). make pc compiles
 * this in place of sdk/ninja/njScalor2.c, whose body is SH-4 assembly */
#include <ninja.h>

#include "xbox/include/sh4.h"

Float(njScalor2)(NJS_VECTOR *v)
{
    return sh4_fipr_self(v->x, v->y, v->z, 0.0F);
}
