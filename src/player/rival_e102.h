/* the rival E-102: E-102 as player 1, on the pad its task writes */
#ifndef PLAYER_RIVAL_E102_H
#define PLAYER_RIVAL_E102_H

#include "player/rival.h"

/* start the task that drives the rival E-102, for rival_begin, and stop
 * E-102's countdown. it returns the task, or NULL when task_create fails.
 * the countdown stops even then */
struct task *rival_e102_start(struct battle *battle);

#endif
