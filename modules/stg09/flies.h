/* O HAE: flies that wander about a box and flee player 0 and the camera */
#ifndef STG09_FLIES_H
#define STG09_FLIES_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record. ang.z's low byte
 * counts the flies. a task the set editor holds as it starts has one. scl is
 * half the box's size on each axis, and ang.y turns the box about y. the
 * step asks the heap for 32 bytes a fly. it writes through null when the heap
 * has no room. camera_work must be set while the box is in view, and player
 * 0's work while any fly moves */
void flies_step(struct task *task);

#endif
