/* O MANF hinged flap, which opens while its switch is on */
#ifndef STG09_MANF_H
#define STG09_MANF_H

#include "system/task.h"

/* the object list's step. the flap opens once switch scl.z is on, and shuts
 * once the switch is off. a scl.z of 0 names no switch, and the flap stays
 * shut. SE_FLAP plays as the flap ends each move */
void manf_step(struct task *task);

#endif
