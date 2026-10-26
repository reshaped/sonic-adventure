/* O BURNING, flames that fly out and up from a ring round the object */
#ifndef ADV03_BURNING_H
#define ADV03_BURNING_H

#include "system/task.h"

/* the step the object list names for O BURNING. one step in 8, from the
 * second on, starts a flame from flame_pool when particle_alloc gives one.
 * the flame starts on a ring of radius scl.x round the object, ang.x further
 * round it than the last flame. it leaves the ring outward at scl.x a step,
 * give or take up to 1 along x and z. its upward speed starts at 0 to
 * 2 * scl.y a step, by the sine of a phase that ang.y advances from flame to
 * flame */
void burning_step(struct task *task);

#endif
