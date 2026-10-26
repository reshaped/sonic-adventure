/* O 6SLOT and O 6SLOT2, spinning slots that scatter rings when hit */
#ifndef STG09_SIX_SLOT_H
#define STG09_SIX_SLOT_H

#include "system/task.h"

/* the object list's steps. scl.y spins the top, in degrees a step. O 6SLOT's
 * face changes every scl.z + 2 steps, never while scl.z is negative.
 * O 6SLOT2's never changes. a slot's first four hits scatter 4, 3, 2 and 1
 * rings. after the fourth, players can no longer aim at it. its top slows,
 * and its face stays */
void six_slot_step(struct task *task);
void six_slot2_step(struct task *task);

#endif
