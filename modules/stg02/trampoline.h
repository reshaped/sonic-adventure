/* O POLINE: a trampoline. it throws a player higher on each bounce in a row, up
 * to the third */
#ifndef STG02_TRAMPOLINE_H
#define STG02_TRAMPOLINE_H

#include "system/task.h"

/* the object list's step for O POLINE. task needs TASK_WORK's record, and a
 * player the trampoline holds must keep its task. while the trampoline waits
 * for a player, its cylinder pushes back players, enemies, objects and pickups.
 * none of them pushes it */
void windy_trampoline_step(struct task *task);

#endif
