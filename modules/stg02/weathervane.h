/* O KAZAMI: a solid weathervane that turns to random headings as its propeller
 * spins */
#ifndef STG02_WEATHERVANE_H
#define STG02_WEATHERVANE_H

#include "system/task.h"

/* the object list's step for O KAZAMI. task needs TASK_WORK's record. the
 * weathervane's capsule pushes back players, enemies, objects and pickups. none
 * of them pushes it. scl.x is the propeller's spin in degrees a step, and it
 * also scales how fast the head turns */
void windy_weathervane_step(struct task *task);

#endif
