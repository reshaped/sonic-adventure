/* njScaleV: njScale by v. make pc compiles this in place of
 * sdk/ninja/njScaleV.c, whose body is SH-4 assembly (njscale.h). v is read
 * whole first, as the body loads it */
#include <ninja.h>

#include "xbox/sdk/ninja/njscale.h"

void(njScaleV)(NJS_MATRIX *m, NJS_VECTOR *v)
{
    nj_scale(m, v->x, v->y, v->z);
}
