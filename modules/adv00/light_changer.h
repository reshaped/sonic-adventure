/* O LIGHTCHANGER box that blends the act's lighting palette when a player
 * comes near */
#ifndef ADV00_LIGHT_CHANGER_H
#define ADV00_LIGHT_CHANGER_H

#include "system/task.h"

/* the step the object list names for O LIGHTCHANGER. a placement's angle x,
 * modulo 2, gives the way it blends. its scale sizes its box */
void light_changer_init(struct task *task);

#endif
