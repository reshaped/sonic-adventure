/* the roll: one of act 1's paths, which carries player 0 along it curled
 * up */
#ifndef STG07_ROLL_PATH_H
#define STG07_ROLL_PATH_H

#include "system/task.h"

/* the step of the task path_create_tasks makes for the path */
void ruin_path_roll_step(struct task *task);

#endif
