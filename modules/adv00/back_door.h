/* O URADOOR back door that swings open when a player touches it in the sewer
 * act */
#ifndef ADV00_BACK_DOOR_H
#define ADV00_BACK_DOOR_H

#include "system/task.h"

/* the step the object list names for O URADOOR. in the sewer act a
 * player's touch swings the door open. the door swings shut 90 steps after
 * it opens fully, and a touch on the way opens it again. in any other act it
 * stays shut */
void back_door_init(struct task *task);

#endif
