/* njAddVector: add vs to vd. make pc compiles this in place of
 * sdk/ninja/njAddVector.c, whose body is SH-4 assembly. vs is read whole
 * before vd is written, so the two may be one */
#include <ninja.h>

void(njAddVector)(NJS_VECTOR *vd, NJS_VECTOR *vs)
{
    Float x = vd->x + vs->x;
    Float y = vd->y + vs->y;
    Float z = vd->z + vs->z;

    vd->z = z;
    vd->y = y;
    vd->x = x;
}
