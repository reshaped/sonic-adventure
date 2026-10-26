/* sparks: short streaks of light that fly, slow and fall */
#ifndef EFFECT_SPARK_H
#define EFFECT_SPARK_H
#include <ninja.h>
#include <sg_xpt.h>

/* a spark task at level 6 at pos, moving at speed, both copied. it lives
 * sixteen steps, but moves and draws only on a frame's first step while in
 * view. the float is unread */
void spark_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float unused);

#endif
