/* act 0's sea: a surface that follows player 0, and a grid of points that
 * ripples raise */
#ifndef STG01_SEA_H
#define STG01_SEA_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the height of the sea's surface where no ripple reaches */
#define SEA_LEVEL (-1.5F)

/* the step the sea's task starts with. task needs TASK_WORK's and
 * TASK_FORCE's records. it puts the surface's plane in the collision world
 * as moving water. the steps after it make the sea follow player 0, who must
 * have a work */
void beach_sea_init(struct task *task);
/* add a ripple to sea at x, z, unless sea holds 30 already. radius and
 * width count waves, each 65536 / 150 units long. the ring's outer edge
 * starts radius out and spreads 8 units a step. the ring reaches width in
 * from that edge, or to the center. in the ring, the surface rises and
 * falls by up to twice height. height keeps 0.985 of itself each step, and
 * the ripple lasts 200 steps */
void beach_sea_add_ripple(struct task *sea, Float x, Float z, Float radius,
                          Float width, Float height);
/* the height of the sea's surface at x, z: SEA_LEVEL plus the waves of each
 * ripple whose ring holds x, z */
Float beach_sea_get_height(struct task *sea, Float x, Float z);

#endif
