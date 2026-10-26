/* FLAG, a flag that waves, from a random frame of its wave */
#ifndef AL_RACE_FLAG_H
#define AL_RACE_FLAG_H

#include "system/task.h"

/* FLAG's step in the race's object kinds. it gives the task the flag's
 * record and callbacks, and starts the flag. a task whose record the heap
 * can't hold destroys itself on its next step */
void flag_init(struct task *task);

#endif
