/* the sparks a blast throws */
#ifndef STG06_SPARKS_H
#define STG06_SPARKS_H
#include <ninja.h>
#include <sg_xpt.h>

/* start a batch of count sparks at pos. a spark's speed is speed plus a
 * random part, within spread of 0 along each axis. the batch moves and draws
 * as streaks for 251 steps. a spark that crosses the height floor with a y
 * speed over 4 either way bounces there. the bounce reverses that speed and
 * keeps 0.6 of it. FALSE when the sparks' task can't be created, or the
 * batch's record can't be allocated. the heap must have room for the
 * sparks' arrays: with none, they are written through null */
Bool sky_deck_sparks_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float spread,
                            Float floor, int count);

#endif
