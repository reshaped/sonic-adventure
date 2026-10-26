/* O ISHIA and O ISHIB: solid stones */
#ifndef STG02_STONES_H
#define STG02_STONES_H

#include "system/task.h"

/* the object list's steps. task needs TASK_WORK's record. outside the set
 * editor, each stone's capsule pushes back players, enemies, objects and
 * pickups. none of them pushes it */
void windy_stone_a_step(struct task *task);
void windy_stone_b_step(struct task *task);

#endif
