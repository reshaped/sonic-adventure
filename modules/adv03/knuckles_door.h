/* O KNUDOOR, a door that only draws */
#ifndef ADV03_KNUCKLES_DOOR_H
#define ADV03_KNUCKLES_DOOR_H

#include "system/task.h"

/* the step the object list names for O KNUDOOR. it draws the door on each
 * step. the task has no draw, so the door doesn't show under the pause menu.
 * once placement_try_leave finds the task out of range, the step does
 * nothing, and the task destroys itself on its next step */
void knuckles_door_step(struct task *task);

#endif
