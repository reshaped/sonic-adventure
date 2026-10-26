/* Emergency: an invisible launcher on the tables. its sphere sends player 0
 * flying once hit for long enough */
#ifndef STG09_EMERGENCY_H
#define STG09_EMERGENCY_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record. the sphere's
 * radius is scl.x. a hit launches player 0 after at least scl.z steps of hits
 * in a row. the launch turns player 0 to the y angle and sends it off at
 * scl.y */
void emergency_step(struct task *task);

#endif
