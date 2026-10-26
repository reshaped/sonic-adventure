/* O ELEBOX IN and O ELEBOX OUT elevator doorways. the way in opens for player 0
 * and leads to the station act, and the way out stays shut */
#ifndef ADV00_ELEVATOR_BOX_H
#define ADV00_ELEVATOR_BOX_H

#include "system/task.h"

/* the steps the object list names for O ELEBOX IN and O ELEBOX OUT. the way in
 * opens while player 0 is near it. it shuts again once player 0 moves away or
 * stands on its floor, and a player's touch opens it again as it shuts. once
 * it shuts on player 0 standing on its floor, the act fades to the station
 * act. the way out stays shut */
void elevator_box_in_init(struct task *task);
void elevator_box_out_init(struct task *task);

#endif
