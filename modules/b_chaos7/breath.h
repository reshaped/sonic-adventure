/* Perfect Chaos's breath beams, and the lights that rise as he breathes */
#ifndef B_CHAOS7_BREATH_H
#define B_CHAOS7_BREATH_H

#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* create a beam that grows from the mouth matrix and steers at target. the beam
 * keeps both pointers, which must stay valid while it lives */
void chaos7_breath_create(NJS_MATRIX *mouth, NJS_POINT3 *target);
/* a light's first step. it puts the light at height 0. it gives the light a
 * random size, pulse rate and circle, and a random angle on the circle */
void chaos7_breath_light_start(struct task *task);

#endif
