/* O CASINO DOOR the casino's double doors, which swing open once the casino
 * button is pushed */
#ifndef ADV00_CASINO_DOOR_H
#define ADV00_CASINO_DOOR_H

#include "system/task.h"

/* the step the object list names for O CASINO DOOR. outside the casino act
 * it prints ACT CASINO ONLY ! and stays the task's step, so the message
 * shows every step */
void casino_door_init(struct task *task);

#endif
