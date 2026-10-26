/* O SAKUC, O HANEA, O HANEB, O BIGFLO and O HASIE: solid models with
 * spinning parts */
#ifndef STG02_ROTORS_H
#define STG02_ROTORS_H

#include "system/task.h"

/* the object list's steps. task needs TASK_WORK's record. each object's
 * shapes push back players, enemies, objects and pickups, and none of them
 * pushes it. scl.x is the spin, in degrees. O SAKUC, O HANEA, O HANEB and
 * O BIGFLO turn by it each frame they are in view. they take part in the
 * collision checks only then. O HASIE turns by it each step */
void windy_fence_c_step(struct task *task);
void windy_propeller_a_step(struct task *task);
void windy_propeller_b_step(struct task *task);
void windy_big_flower_step(struct task *task);
void windy_hasie_step(struct task *task);

#endif
