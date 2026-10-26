/* O KAITENASHIBA: a platform that its own handle raises and lowers */
#ifndef STG12_HANDLE_LIFT_H
#define STG12_HANDLE_LIFT_H

#include "system/task.h"

/* the object list's step. task needs TASK_MOTION's, TASK_WORK's and
 * TASK_FORCE's records */
void handle_lift_step(struct task *task);

#endif
