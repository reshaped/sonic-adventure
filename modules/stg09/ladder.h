/* O HASGO ladder, which players climb on either face */
#ifndef STG09_LADDER_H
#define STG09_LADDER_H
#include "system/task.h"

/* the object list's step. scl.x scales the ladder's width, and players climb
 * the face on player 0's side. in act 1, the ladder requests act 0 when
 * player 0 nears its top. player 0 must come within 20 of the point 120
 * above the ladder's position while either player climbs. the ladder then
 * resets player 0, and no one climbs it again */
void ladder_step(struct task *task);

#endif
