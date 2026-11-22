/* World-space bounds enclose a swept sphere and its rounding error. */
#ifndef COLLISION_BOUNDS_H
#define COLLISION_BOUNDS_H
#include <ninja.h>
#include "system/port.h"

struct sweep_bounds {
    NJS_POINT3 low, high;
};

/* Allow for rounding the endpoint in world coordinates and for the face
 * test's tolerance at a triangle boundary. */
ALWAYS_INLINE void sweep_interval(Float start, Float speed, Float radius,
                                  Float *low, Float *high)
{
    Float end = start + speed;
    Float pad = radius * (1.0F + 1.0F / 32768.0F) +
                (__builtin_fabsf(start) + __builtin_fabsf(speed)) * 0x1p-21F;
    *low = (start < end ? start : end) - pad;
    *high = (start > end ? start : end) + pad;
}

ALWAYS_INLINE void sweep_extents(struct sweep_bounds *box, NJS_POINT3 pos,
                                 NJS_VECTOR speed, Float radius)
{
    sweep_interval(pos.x, speed.x, radius, &box->low.x, &box->high.x);
    sweep_interval(pos.y, speed.y, radius, &box->low.y, &box->high.y);
    sweep_interval(pos.z, speed.z, radius, &box->low.z, &box->high.z);
}

ALWAYS_INLINE Bool sweep_outside(Float a, Float b, Float c, Float low,
                                 Float high)
{
    return (a < low && b < low && c < low) ||
           (a > high && b > high && c > high);
}

LINKED_LAST(sweep_box_misses)
static __attribute__((noinline)) Bool
sweep_box_misses(const struct sweep_bounds *box, const NJS_POINT3 *v)
{
    return sweep_outside(v[0].x, v[1].x, v[2].x, box->low.x, box->high.x) ||
           sweep_outside(v[0].z, v[1].z, v[2].z, box->low.z, box->high.z) ||
           sweep_outside(v[0].y, v[1].y, v[2].y, box->low.y, box->high.y);
}

#endif
