/* the slide: one of act 1's paths, down which player 0 slides */
#ifndef STG07_SLIDE_H
#define STG07_SLIDE_H

#include "system/task.h"

/* the first step of the task path_create_tasks makes for the path */
void ruin_path_slide_step(struct task *task);

#endif
