/* Path, the second table's four rides. a touch of one of its four switches
 * carries player 0 along that ride's path, curled up. the camera follows
 * behind him */
#ifndef STG09_RIDES_H
#define STG09_RIDES_H

#include "system/task.h"

/* Path's step in the object list */
void ride_step(struct task *task);

#endif
