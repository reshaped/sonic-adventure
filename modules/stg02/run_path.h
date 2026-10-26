/* a path's step that catches players at a sphere and runs them along the
 * path */
#ifndef STG02_RUN_PATH_H
#define STG02_RUN_PATH_H

#include "system/task.h"

/* the step one of the stage's paths names. the path task's vars hold the
 * path and the stage and act it was made for. struct path_vars gives their
 * layout. task needs TASK_WORK's record. a failed copy of the sphere's shape
 * goes unchecked */
void windy_run_path_step(struct task *task);

#endif
