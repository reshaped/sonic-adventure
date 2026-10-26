/* O ROOF turning roof that carries a player around */
#ifndef STG03_ROOF_H
#define STG03_ROOF_H

#include "system/task.h"

/* task step for O ROOF, named in the object list. its set entry's scale x is
 * the roof's turn a step about y, in degrees */
void roof_step(struct task *task);

#endif
