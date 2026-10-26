/* O BOWLING: ten pins, with a gate and a platform that gives rings beyond
 * them. O BALL: a ball that rolls off at a player's touch, and its gate */
#ifndef STG03_BOWLING_H
#define STG03_BOWLING_H

#include "system/task.h"

/* the object list's steps for O BOWLING and O BALL. each task needs
 * TASK_WORK's record, and player 0's work must exist. each gate is killed
 * outside Twinkle Park's act 1, and O BALL's while player 0 is Sonic. a
 * failed copy of a pin's, a gate's or the ball's shapes goes unchecked */
void bowling_step(struct task *task);
void bowling_ball_step(struct task *task);

#endif
