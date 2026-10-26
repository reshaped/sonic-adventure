/* BigStar: a sprite that turns to face the camera and shows its eight images
 * in turn */
#ifndef STG09_BIG_STAR_H
#define STG09_BIG_STAR_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record. scl.x scales the
 * star. each image shows for scl.z + 1 steps, scl.z a whole number */
void big_star_step(struct task *task);

#endif
