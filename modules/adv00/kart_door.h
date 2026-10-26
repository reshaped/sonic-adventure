/* O KARTDOOR sliding door to Twinkle Circuit, locked until the story opens
 * it */
#ifndef ADV00_KART_DOOR_H
#define ADV00_KART_DOOR_H

#include "system/task.h"

/* the step the object list names for O KARTDOOR. the door stays shut until
 * player 0's character has its ENTRANCE_CIRCUIT flag. after that, a player
 * within 20 slides it open. it shuts once it has stood open more than 90 steps
 * and no player is within 20. a player who comes back while it shuts opens it
 * again */
void kart_door_init(struct task *task);

#endif
