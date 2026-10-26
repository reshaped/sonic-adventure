/* a value inside a range, snapped to one of its ends */
#include <sg_xpt.h>

#include "system/range.h"

int range_snap(int x, int lo, int hi)
{
    int half;

    if (x < lo || x > hi) {
        return x;
    }
    half = (hi - lo) >> 1;
    if (x < half) {
        return lo;
    }
    return hi;
}

Float range_snap_float(Float x, Float lo, Float hi)
{
    Float half;

    if (x < lo || x > hi) {
        return x;
    }
    half = (hi - lo) * 0.5F;
    if (x < half) {
        return lo;
    }
    return hi;
}
