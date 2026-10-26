/* O HIYOKOYUKA: a floor that breaks into pieces when an attack hurts it */
#ifndef STG12_HIYOKO_FLOOR_H
#define STG12_HIYOKO_FLOOR_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's and TASK_FORCE's records */
void hiyoko_floor_start(struct task *task);

#endif
