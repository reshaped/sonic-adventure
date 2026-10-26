/* O SONZ Sonic statue, which shows one of five levels by the rings
 * deposited */
#ifndef STG09_SONIC_STATUE_H
#define STG09_SONIC_STATUE_H

#include "system/task.h"

/* the step the object list names for O SONZ. while player 0 is Knuckles,
 * O SONZ is the O KSONICZO statue, through sonic_statue_knuckles_step */
void sonic_statue_step(struct task *task);
/* show level on the kept statue: the first started outside the set editor
 * while none is kept. level must not be negative. a level above 4 changes
 * nothing, as does a call while none is kept */
void sonic_statue_set_level(int level);

#endif
