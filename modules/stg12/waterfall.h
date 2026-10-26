/* the waterfall that act 0's upper pool pours down into the act's water.
 * two ripples stretch down from its top, and a splash rides the water at its
 * foot */
#ifndef STG12_WATERFALL_H
#define STG12_WATERFALL_H

#include "system/task.h"

/* the waterfall's first step. the upper pool creates the waterfall as its
 * child as the pool starts to pour */
void shelter_waterfall_start(struct task *task);

#endif
