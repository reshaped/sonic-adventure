/* O CHAOCARD Chao Stadium membership card, which spins until a player picks it
 * up */
#ifndef ADV00_CHAO_CARD_H
#define ADV00_CHAO_CARD_H

#include "system/task.h"

/* the step the object list names for O CHAOCARD. picking the card up shows
 * a message, and a dropped card goes once it has lain 1801 steps */
void chao_card_init(struct task *task);

#endif
