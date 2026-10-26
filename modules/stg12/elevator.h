/* O ELEVATOR: a car that carries the players down its shaft, then opens its
 * doors */
#ifndef STG12_ELEVATOR_H
#define STG12_ELEVATOR_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's and TASK_FORCE's records */
void shelter_elevator_step(struct task *task);

#endif
