/* BanjuDoor: a door that sinks while CASINO_BANJU_DOOR_OPEN is set, and
 * rises while it is clear */
#ifndef STG09_BANJU_DOOR_H
#define STG09_BANJU_DOOR_H

#include "system/task.h"

/* the object list's step. it clears CASINO_BANJU_DOOR_OPEN. task needs
 * TASK_WORK's record */
void banju_door_step(struct task *task);

#endif
