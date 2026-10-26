/* O FL: a spout that sends flames off at random, with a sphere that hurts
 * players */
#ifndef STG09_CASINO_FLAME_H
#define STG09_CASINO_FLAME_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record. scl is the
 * flames' speed, which ang.y turns */
void casino_flame_step(struct task *task);

#endif
