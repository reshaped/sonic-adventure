/* O TSDOOR pair of sliding doors that open as a player approaches */
#ifndef STG09_TS_DOOR_H
#define STG09_TS_DOOR_H

#include "system/task.h"

/* the step the object list names for O TSDOOR. the doors start to open when
 * the nearest player comes within 40 units. once open, they start to shut
 * when the nearest is beyond 50 */
void ts_door_step(struct task *task);

#endif
