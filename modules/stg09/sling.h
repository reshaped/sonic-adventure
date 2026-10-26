/* SlingL and SlingR, the slingshots on the pinball tables */
#ifndef STG09_SLING_H
#define STG09_SLING_H

#include "system/task.h"

/* the object list's steps. a touch launches player 0 at scl.z plus his
 * forward speed and a random share of scl.y. the launch speed is at most 8.
 * each touch gives 1 ring, but the 20th gives 11, the 40th 21, and so on */
void sling_left_step(struct task *task);
void sling_right_step(struct task *task);

#endif
