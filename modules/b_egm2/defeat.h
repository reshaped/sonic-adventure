/* the Egg Walker beaten: its last mode, which clears the stage */
#ifndef B_EGM2_DEFEAT_H
#define B_EGM2_DEFEAT_H

#include "system/task.h"

/* the boss's step in EGG_WALKER_DEFEAT. small blasts break out about it
 * at random. once player 0 is down on the street, it turns the pads off and
 * stops the time. it clears the stage two steps later. task is the
 * boss's */
void egg_walker_defeat(struct task *task);

#endif
