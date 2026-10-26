/* Sand Hill's stage task, which boards player 0 and widens the view with
 * the board's speed */
#ifndef SBOARD_SUNABOARD_H
#define SBOARD_SUNABOARD_H

#include "object/placement.h"
#include "system/task.h"

/* the kinds of object the stage places. sunaboard_object_list points to
 * them and gives their count */
extern struct object_kind sunaboard_object_kinds[];

/* the stage task's step, which 1ST_READ.BIN's stage_steps names */
void sunaboard_step(struct task *task);

#endif
