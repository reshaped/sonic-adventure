/* the go-round, which the set editor names O GO ROUND. its carousel and
 * floor turn and rest by turns. the turning floor carries the players on
 * it */
#ifndef STG03_GO_ROUND_H
#define STG03_GO_ROUND_H

#include "system/task.h"

/* the step twinkle_object_kinds names for O GO ROUND. its task needs a
 * work, and player 0 must exist */
void go_round_step(struct task *task);

#endif
