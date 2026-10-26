/* the shock wave Zero makes as it lands from a leap */
#ifndef B_ROBO_SHOCK_WAVE_H
#define B_ROBO_SHOCK_WAVE_H

#include "system/task.h"

/* make a shock wave, a ring on the deck. its center lies 15 along the
 * negated cosine and sine of work's ang.y, the way Zero faces. it spreads
 * for 34 steps and ends on its 35th. its one shape stands on the ring's
 * edge nearest player 0, and hurts players. when task_create fails, it
 * makes none */
void erobo_shock_wave_create(struct task_work *work);

#endif
