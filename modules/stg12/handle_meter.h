/* O KAITENMETER: a gauge of the whole turns each way under a handle's key */
#ifndef STG12_HANDLE_METER_H
#define STG12_HANDLE_METER_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record */
void handle_meter_step(struct task *task);

#endif
