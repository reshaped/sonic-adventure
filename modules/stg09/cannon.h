/* HoudaiL and HoudaiR: cannons on the tables, which fire confetti */
#ifndef STG09_CANNON_H
#define STG09_CANNON_H

#include "system/task.h"

/* the object list's steps. task needs TASK_WORK's record. a cannon fires on
 * a step that finds its bit set and player 0 on land. the bits are
 * CASINO_HOUDAI_L_FIRES and CASINO_HOUDAI_R_FIRES, and firing clears the
 * cannon's */
void cannon_left_step(struct task *task);
void cannon_right_step(struct task *task);

#endif
