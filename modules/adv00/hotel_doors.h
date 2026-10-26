/* O HOTEL FRONT, O HOTEL BACK and O HOTEL POOL folding doors, locked until the
 * story of player 0's character opens them */
#ifndef ADV00_HOTEL_DOORS_H
#define ADV00_HOTEL_DOORS_H

#include "system/task.h"

/* the steps the object list names for O HOTEL FRONT, O HOTEL BACK and O HOTEL
 * POOL. a door is locked until its flag for player 0's character is set. the
 * back door has no flag for E-102, so it stays locked for him. only Big needs a
 * flag for the pool door. an unlocked door folds open when a player comes
 * near it, and folds shut 90 steps after it opens fully. a player near it as it
 * shuts opens it again */
void hotel_front_init(struct task *task);
void hotel_back_init(struct task *task);
void hotel_pool_init(struct task *task);

#endif
