/* vector sums, differences and a dot product. make pc compiles this in place
 * of src/system/vector.src. each sum reads a and b whole before it writes
 * out, so out may be a or b, as vector.h says */
#include <ninja.h>
#include <sg_xpt.h>

#include "system/vector.h"

/* NOLINTBEGIN(readability-identifier-length) */
void vector_sum(NJS_VECTOR *a, NJS_VECTOR *b, NJS_VECTOR *out)
{
    Float x = a->x + b->x;
    Float y = a->y + b->y;
    Float z = a->z + b->z;

    out->x = x;
    out->y = y;
    out->z = z;
}

void vector_diff(NJS_VECTOR *a, NJS_VECTOR *b, NJS_VECTOR *out)
{
    Float x = a->x - b->x;
    Float y = a->y - b->y;
    Float z = a->z - b->z;

    out->x = x;
    out->y = y;
    out->z = z;
}

/* vector.src's FIPR FV4,FV0, b's differences first, by the port's model of
 * FIPR (xbox/include/sh4.h) */
Float vector_dot_from(NJS_POINT3 *from, NJS_POINT3 *a, NJS_POINT3 *b)
{
    Float ax = a->x - from->x;
    Float ay = a->y - from->y;
    Float az = a->z - from->z;
    Float bx = b->x - from->x;
    Float by = b->y - from->y;
    Float bz = b->z - from->z;

    return sh4_fipr(bx, by, bz, 0.0F, ax, ay, az, 0.0F);
}
/* NOLINTEND(readability-identifier-length) */
