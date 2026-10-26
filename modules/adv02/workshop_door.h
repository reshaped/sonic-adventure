/* workshop door that swings open a quarter turn as the nearest player comes
 * near */
#ifndef ADV02_WORKSHOP_DOOR_H
#define ADV02_WORKSHOP_DOOR_H

#include "system/task.h"

/* task step the Mystic Ruins field task creates in the station act */
void workshop_door_step(struct task *task);

#endif
