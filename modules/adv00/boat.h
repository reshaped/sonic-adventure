/* O BOAT boat that takes player 0 to the Egg Carrier once the story opens the
 * way */
#ifndef ADV00_BOAT_H
#define ADV00_BOAT_H

#include "system/task.h"

/* the step the object list names for O BOAT. outside the station act it
 * prints ACT STATION ONLY ! and stays the task's step, so the message shows
 * every step */
void station_square_boat_init(struct task *task);

#endif
