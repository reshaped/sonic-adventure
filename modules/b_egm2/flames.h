/* the Egg Walker's flames, breathed from the nozzle on its turret */
#ifndef B_EGM2_FLAMES_H
#define B_EGM2_FLAMES_H

#include "system/task.h"

/* the boss's step in EGG_WALKER_FLAMES. it sweeps the turret across
 * player 0 and tips the nozzle, breathing a flame every other step. it goes
 * back to EGG_WALKER_WALK once its time runs out. it goes back sooner once
 * player 0 comes close across the ground. task is the boss's */
void egg_walker_flames(struct task *task);
/* set up the sprite the flames draw. call it before any flame draws, as the
 * boss's EGG_WALKER_INIT step does */
void egg_walker_flames_init(void);

#endif
