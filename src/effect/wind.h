/* the wind: one task that blows across the ground in gusts */
#ifndef EFFECT_WIND_H
#define EFFECT_WIND_H
#include <ninja.h>
#include <sg_xpt.h>

/* start the wind, or change the one blowing. level runs from 0, still air,
 * to 4, and any other level blows as 4. heading, unless null, sets the way
 * it blows. a new wind given no heading blows a random way. if its task
 * can't be made, no wind starts. the wind takes up a new level only when
 * it picks a strength. it picks one on its first step. its phase makes a
 * turn 181 to 898 steps after that. the wind then picks one on every step
 * until the phase overflows, at least 5.8 million steps on. levels 1
 * to 4 pick a random strength from 0.5 to 1, 1 to 2, 2 to 3 and 3 to 5 */
void wind_set(Sint8 level, const Angle *heading);
/* end the wind, if one blows */
void wind_stop(void);
/* set force to the wind's push at pos, unless force is null. the push is
 * horizontal and along the heading. its size runs in gusts from 0 to 0.7 of
 * the strength. returns TRUE while a wind blows, even at level 0. with no
 * wind it returns FALSE and leaves force as it was */
Bool wind_get_force(NJS_POINT3 *pos, NJS_VECTOR *force);

#endif
