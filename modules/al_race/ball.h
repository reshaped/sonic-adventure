/* the balls of a race of kind 0, one to a lane, which each lane's Chao
 * pushes toward its lane's target */
#ifndef AL_RACE_BALL_H
#define AL_RACE_BALL_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the ball's item kind, ball_tell's index in item_tells */
#define BALL_KIND 2

/* the commands a Chao sends the ball it claimed, as it starts to push the
 * ball and as it stops */
#define BALL_PUSH 5
#define BALL_STOP 6

/* create a ball for each lane, where ball_get_place says, as a child of the
 * scenery task. without the scenery task, it creates none */
void ball_create(void);
/* the ball's handler. a command takes effect only while other is the Chao
 * that claimed the ball. ITEM_LET_GO drops the claim. BALL_PUSH starts the
 * push the claim waits for. BALL_STOP ends the push, and leaves the ball
 * where it is. returns 1 when the command takes effect, else 0 */
int ball_tell(struct task *task, struct task *other, Sint8 command);
/* store in pos where lane's ball starts, and return 1. return 0, storing
 * nothing, for a null pos or a lane outside 0 to RACE_LANES - 1 */
int ball_get_place(NJS_POINT3 *pos, int lane);

#endif
