/* O KKDOOR paired doors, whose collision follows their motion */
#ifndef STG09_KK_DOOR_H
#define STG09_KK_DOOR_H
#include "system/task.h"

/* the object list's step. the doors open when the nearest player comes
 * within 70 units. once open, they shut when the nearest player is beyond
 * 80 */
void kk_door_step(struct task *task);

#endif
