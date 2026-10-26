/* O SAKANA8K: a school of fish that swim about a tank and flee player 0 */
#ifndef STG01_SCHOOL_H
#define STG01_SCHOOL_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record. ang.z's low byte
 * is the count of fish, or 1 in the set editor. scl is the tank's half-size
 * along each axis, and ang.y turns the tank about y. the step allocates 40
 * bytes for each fish from the heap, unchecked. camera_work must be set
 * while the tank is in view, and player 0's work while a fish moves */
void beach_school_step(struct task *task);

#endif
