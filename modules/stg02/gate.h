/* O GATE: a solid gate whose sides spin about y and whose top spins about z */
#ifndef STG02_GATE_H
#define STG02_GATE_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record. the gate's posts and
 * the balls along its top push back players, enemies, objects and pickups. none
 * of them pushes the gate. scl.x is the sides' spin and scl.y the top's, in
 * degrees a step */
void windy_gate_step(struct task *task);

#endif
