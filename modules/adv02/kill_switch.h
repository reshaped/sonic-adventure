/* the switch the object list names O KILL SWITCH: a player carries it, and
 * pressing it blows up the nearest monkey cage within reach */
#ifndef ADV02_KILL_SWITCH_H
#define ADV02_KILL_SWITCH_H

#include "system/task.h"

/* the held kind player_set_held_kind gives player 0 while a player carries the
 * switch */
#define HELD_KILL_SWITCH 9

/* the live switch's work, or null. one switch at most is live */
extern struct task_work *kill_switch_work;

/* the step the object list names for O KILL SWITCH. O DIG PLACE2 also creates
 * switches with it */
void kill_switch_step(struct task *task);

#endif
