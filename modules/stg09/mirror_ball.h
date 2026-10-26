/* O MB mirror ball that spins and turns the act's light with it */
#ifndef STG09_MIRROR_BALL_H
#define STG09_MIRROR_BALL_H

#include "system/task.h"

/* the object list's step. the ball turns scl.x + 2 degrees about y each
 * step. it sets light_ry to its turn outside the set editor, in the act it
 * started in. as it is destroyed there, it gives light_ry back the low byte
 * light_ry held as it started, sign-extended */
void mirror_ball_step(struct task *task);

#endif
