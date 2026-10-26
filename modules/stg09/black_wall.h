/* KuroKabe: the tables' black walls, whose alpha pulses. one wall is left out
 * while ShoogaiL is down, and another while ShoogaiR is */
#ifndef STG09_BLACK_WALL_H
#define STG09_BLACK_WALL_H

#include "system/task.h"

/* the object list's step. it clears CASINO_SHOOGAI_L_DOWN and
 * CASINO_SHOOGAI_R_DOWN. task needs TASK_WORK's record */
void black_wall_step(struct task *task);

#endif
