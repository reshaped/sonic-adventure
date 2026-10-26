/* O HW BELL hanging bell that rings and scatters rings when struck */
#ifndef STG04_BELL_H
#define STG04_BELL_H

#include "system/task.h"

/* task step for O HW BELL, named in the object list. its set entry's scale x,
 * y and z give the bell's constant material its red, green and blue */
void bell_step(struct task *task);

#endif
