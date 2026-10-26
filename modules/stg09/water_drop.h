/* water drops that fall, bounce off the floor and player 0, and splash */
#ifndef STG09_WATER_DROP_H
#define STG09_WATER_DROP_H
#include <ninja.h>
#include <sg_xpt.h>

/* add a drop at pos, moving at speed. each axis of its speed moves up to spread
 * either way at random. the drop falls. a drop that meets floor falling faster
 * than 0.1 a step bounces off it. balls at player 0's head and weld points 0
 * and 1 bounce a drop that falls onto them. a drop is gone once it falls below
 * floor, or on its 151st step.
 *
 * a splash is a puff of water smoke. a bounce can splash, and some drops splash
 * on their own. each drop splashes at most once.
 *
 * one task runs every drop, and an add makes it when none runs. this returns
 * FALSE when that task or the drop can't be allocated. the task ends a step
 * after it finds no drop, and frees any drop added in between */
Bool water_drop_add(NJS_POINT3 *pos, NJS_VECTOR *speed, Float spread,
                    Float floor);

#endif
