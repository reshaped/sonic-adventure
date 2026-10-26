/* O KAMOME and O KAMOMEL seagulls that fly between random points above a
 * circle about where they were placed */
#ifndef STG01_SEAGULL_H
#define STG01_SEAGULL_H

#include "system/task.h"

/* the object list's steps. O KAMOME draws its action, and O KAMOMEL a body
 * and two swinging wings. task needs TASK_WORK's, TASK_MOTION's and
 * TASK_EXTRA's records. outside the set editor, player 0 must have a work.
 * a seagull's targets lie scl.y + 80 out from where it was placed. they lie
 * up to 80 above it */
void beach_seagull_step(struct task *task);
void beach_seagull_l_step(struct task *task);

#endif
