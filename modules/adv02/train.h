/* O TRAIN three-car train at the station, which takes player 0 to Station
 * Square */
#ifndef ADV02_TRAIN_H
#define ADV02_TRAIN_H

#include "system/task.h"

/* the step the object list names for O TRAIN. in an odd act it does nothing,
 * and stays the task's step. while the set editor holds the task, it draws the
 * train with its doors shut, and stays the task's step too. otherwise it
 * creates the three cars and hands the task the train's own step. the doors
 * start open if player 0's character has its train flag set */
void train_start(struct task *task);

#endif
