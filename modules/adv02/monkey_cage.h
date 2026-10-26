/* the cage the object list names O MONKEY CAGE: a cage with a monkey inside,
 * which Sonic, E-102 and Knuckles break open */
#ifndef ADV02_MONKEY_CAGE_H
#define ADV02_MONKEY_CAGE_H
#include <sg_xpt.h>

#include "system/task.h"

/* the monkey cages, numbered from 0 by their scale x */
#define MONKEY_CAGES 2

/* the step the object list names for O MONKEY CAGE */
void monkey_cage_step(struct task *task);

/* each live cage's work, by its index, or null */
extern struct task_work *monkey_cages[MONKEY_CAGES];

/* set or clear the flag player 0's character has for a cage, and read it. a
 * set flag means the cage is blown up. where the character has no flag,
 * monkey_cage_set_flag does nothing and monkey_cage_flag returns FALSE */
void monkey_cage_set_flag(int cage, Bool on);
Bool monkey_cage_flag(int cage);

#endif
