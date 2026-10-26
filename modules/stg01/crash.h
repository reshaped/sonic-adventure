/* O BZ, Tails beside a smoking wreck. player 0 clears the stage by touching
 * Tails */
#ifndef STG01_CRASH_H
#define STG01_CRASH_H

#include "system/task.h"

/* the step the object list names for O BZ. until Sonic has cleared
 * Emerald Coast, it draws Tails and the wreck, and waits for player 0's
 * touch. once Sonic has cleared it, the step starts the stage's capsule where
 * it stands, and draws nothing */
void beach_crash_step(struct task *task);

#endif
