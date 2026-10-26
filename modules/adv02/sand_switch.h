/* O SAND SWITCH button that opens the way to Sand Hill when a player stands on
 * it */
#ifndef ADV02_SAND_SWITCH_H
#define ADV02_SAND_SWITCH_H

#include "system/task.h"

/* the step the object list names for O SAND SWITCH */
void sand_switch_step(struct task *task);
/* TRUE once the switch has sunk for player 0's character, and otherwise FALSE.
 * the character's event flag keeps the state, and only Sonic and Tails have
 * one */
Bool sand_switch_flag(void);

#endif
