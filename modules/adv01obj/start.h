/* where the player starts on the Egg Carrier */
#ifndef ADV01OBJ_START_H
#define ADV01OBJ_START_H

#include "system/task.h"

/* place work at the start for the current act and entrance, turned to the
 * start's y angle with its x and z angles 0. stage_place_player calls it for
 * both Egg Carrier stages. STAGE_EGG_CARRIER takes the outside's starts, and
 * any other stage the inside's. the act must be one with starts: 0 to 6
 * outside, and 0 to 5 inside. the entrance must be one the act has a start
 * for */
void egg_carrier_place_player(struct task_work *work);

#endif
