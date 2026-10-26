/* bursts of short pale streaks that fly out, slow and fall */
#ifndef STG08_STREAKS_H
#define STG08_STREAKS_H
#include <ninja.h>
#include <sg_xpt.h>

/* start a burst of count streaks at pos. a streak's velocity is speed plus a
 * random part. that part is within spread of 0 in x and z, and from 0 to
 * spread in y. the burst moves and draws for 71 steps. FALSE when the
 * bursts' task can't be created, or the burst's record can't be allocated.
 * the heap must have room for the streaks' arrays: with none, they are
 * written through null */
Bool streaks_burst(NJS_POINT3 *pos, NJS_VECTOR *speed, int count, Float spread);
/* start a burst as streaks_burst does, with each streak at its own random
 * point. the points lie on the circle of radius about center, at center's
 * height */
Bool streaks_ring(NJS_POINT3 *center, NJS_VECTOR *speed, Float spread,
                  int count, Float radius);

#endif
