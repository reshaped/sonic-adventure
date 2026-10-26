/* player start positions in the Mystic Ruins, by act and entrance */
#ifndef ADV02_START_H
#define ADV02_START_H

#include "system/task.h"

/* put a player's work at the start for the current act and entrance,
 * upright and facing the start's angle about y. the entrance must name one of
 * the act's starts; nothing checks it. 1ST_READ.BIN's stage_place_player
 * calls it */
void mystic_ruins_place_player(struct task_work *work);

#endif
