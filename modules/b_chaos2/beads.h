/* the beads that rise inside Chaos 2's ball */
#ifndef B_CHAOS2_BEADS_H
#define B_CHAOS2_BEADS_H
#include <sg_xpt.h>

#include "system/task.h"

/* make half to all of count beads, at random, as children of parent. each
 * bead's scale is scale to 1.5 times scale. the beads rise from the boss's
 * position, and move with him. a bead whose extra the heap can't give
 * writes through null */
void chaos2_beads_create(struct task *parent, int count, Float scale);

#endif
