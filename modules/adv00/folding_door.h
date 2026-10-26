/* folding door of four panels that opens when a player touches it, which no
 * object list names */
#ifndef ADV00_FOLDING_DOOR_H
#define ADV00_FOLDING_DOOR_H

#include "system/task.h"

/* a task step that starts the door. create the task with TASK_WORK. a player's
 * touch folds the door open, and it folds shut 90 steps after it opens fully. a
 * touch as it shuts opens it again */
void folding_door_init(struct task *task);

#endif
