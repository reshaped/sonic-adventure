/* njUnitVector: scale v to unit length by FSRRA of its squared length, and
 * return that squared length times the FSRRA, its length. make pc compiles
 * this in place of sdk/ninja/njUnitVector.c, whose body is SH-4 assembly:
 * FIPR FV0,FV0 of (v, 0), then v stored z first */
#include <ninja.h>

#include "xbox/include/sh4.h"

Float(njUnitVector)(NJS_VECTOR *v)
{
    Float x = v->x;
    Float y = v->y;
    Float z = v->z;
    Float squared = sh4_fipr_self(x, y, z, 0.0F);
    Float scale = _builtin_fsrra(squared);

    v->z = z * scale;
    v->y = y * scale;
    v->x = x * scale;
    return squared * scale;
}
