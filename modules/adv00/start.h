/* player start positions in Station Square, by act and entrance */
#ifndef ADV00_START_H
#define ADV00_START_H

#include "system/task.h"

/* put a player's work at the start for the current act and entrance,
 * upright and facing the start's angle about y. 1ST_READ.BIN's
 * stage_place_player calls it */
void station_square_place_player(struct task_work *work);

#endif
