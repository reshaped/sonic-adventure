/* O BALL ball that bounces away once a player comes near */
#ifndef STG02_BALL_H
#define STG02_BALL_H

#include "system/task.h"

/* the object list's step for O BALL, whose task needs a work and an extra.
 * scl.x, labeled NUM_CAISUU in the set editor, is how many times the ball
 * rebounds before it falls away */
void windy_ball_step(struct task *task);

#endif
