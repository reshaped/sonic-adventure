/* O CASINO BUTTON button that opens the casino when a player touches it */
#ifndef ADV00_CASINO_BUTTON_H
#define ADV00_CASINO_BUTTON_H

#include "system/task.h"

/* the step the object list names for O CASINO BUTTON. outside the casino
 * act it prints ACT CASINO ONLY ! and stays the task's step, so the message
 * shows every step */
void casino_button_init(struct task *task);

#endif
