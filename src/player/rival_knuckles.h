/* the rival Knuckles: Knuckles as player 1, on the pad its task writes */
#ifndef PLAYER_RIVAL_KNUCKLES_H
#define PLAYER_RIVAL_KNUCKLES_H

#include "player/rival.h"

/* start the task that drives the rival Knuckles, for rival_begin. it
 * returns the task, or NULL when task_create fails */
struct task *rival_knuckles_start(struct battle *battle);

#endif
