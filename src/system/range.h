/* a value inside a range, snapped to one of its ends */
#ifndef SYSTEM_RANGE_H
#define SYSTEM_RANGE_H
#include <sg_xpt.h>

/* return x unchanged outside [lo, hi]. inside, snap to lo if x is
 * below (hi - lo) / 2, otherwise to hi. the threshold is half the width,
 * not the midpoint of the range */
int range_snap(int x, int lo, int hi);
Float range_snap_float(Float x, Float lo, Float hi);

#endif
