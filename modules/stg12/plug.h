/* the plug at the bottom of act 0's upper pool, which Big fishes out */
#ifndef STG12_PLUG_H
#define STG12_PLUG_H

#include "system/task.h"

/* the plug's first step. the upper pool creates the plug as its child, with
 * the work and the motion the plug needs */
void shelter_plug_start(struct task *task);

#endif
