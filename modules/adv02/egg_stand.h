/* the egg stand the object list names O EGG STAND. once something touches its
 * lid, the lid slides open, and the drawer pushes the Chao egg into the water.
 * the egg then floats to the bank */
#ifndef ADV02_EGG_STAND_H
#define ADV02_EGG_STAND_H

#include "system/task.h"

/* the step the object list names for O EGG STAND */
void egg_stand_step(struct task *task);
/* set and read whether the egg has floated to the bank. only egg_stand.c
 * calls them */
void egg_stand_set_floated(Bool floated);
Bool egg_stand_floated(void);

#endif
