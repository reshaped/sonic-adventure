/* player start positions in the past, by act and entrance */
#ifndef ADV03_START_H
#define ADV03_START_H

#include "system/task.h"

/* put a player's work at the start for the current act and entrance,
 * upright and facing the start's angle about y. the act must be 0, 1 or 2,
 * and the entrance 0 or 1: neither is checked. 1ST_READ.BIN's
 * stage_place_player calls it */
void past_place_player(struct task_work *work);

#endif
