/* the snake: the steps of two of the ruin's paths */
#ifndef STG07_SNAKE_H
#define STG07_SNAKE_H

#include "system/task.h"

/* the steps two of the stage's paths name. path_create_tasks makes a task
 * for ruin_path_snake_step's path, which ruin_act0_paths holds. no list
 * holds fun_0c907520's path, so no task runs it */
void fun_0c907520(struct task *task);
void ruin_path_snake_step(struct task *task);

#endif
