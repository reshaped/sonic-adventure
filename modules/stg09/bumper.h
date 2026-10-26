/* Bumper1: a bumper on the pinball tables. Bumper2: a pair of bumper posts,
 * after whose bounce the stick steers player 0 */
#ifndef STG09_BUMPER_H
#define STG09_BUMPER_H

#include "system/task.h"

/* Bumper1's step, from the object list. task needs TASK_WORK's record. a
 * hit sends player 0 off at a speed capped at 8. the speed is scl.z and
 * player 0's own, with up to scl.y more at random. a hit gives a ring.
 * every 20th hit gives a bonus instead. the nth bonus is (10n + 1) mod 256
 * rings */
void bumper_step(struct task *task);
/* Bumper2's step, from the object list. task needs TASK_WORK's record. a
 * post's hit sets player 0's accel to minus its x and y speed, plus 2 up.
 * until player 0 falls below Bumper2, the stick pushes it along z in the
 * air. as it falls below, gravity tilts -15 degrees about x */
void bumper2_step(struct task *task);

#endif
