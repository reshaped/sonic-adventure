/* O TRAIN train to the Mystic Ruins, which leaves once player 0 boards */
#ifndef ADV00_TRAIN_H
#define ADV00_TRAIN_H

#include "system/task.h"

/* the object list's step. in the casino act it makes the train's three
 * cars and hands the task the train's own step. the doors stand open if the
 * story of player 0's character has opened the line, and shut until it does
 * if not. in any other act it prints ACT CASINO ONLY !, and in the set
 * editor it draws the train. either way it stays the task's step */
void station_square_train_init(struct task *task);

#endif
