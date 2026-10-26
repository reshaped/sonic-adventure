/* the sky chase's hud: the score, its combo, and the share of the targets
 * hit */
#ifndef SHOOTING_HUD_H
#define SHOOTING_HUD_H

#include "system/task.h"

/* the hud's step. its first step gives the tornado 5 lives on a new gauge.
 * it also clears the score, the combo and the targets hit */
void shooting_hud_step(struct task *task);
/* for a shot's hit on work, add points times the combo to shooting_score
 * and raise the combo. a hit by anything else adds nothing. shooting_score
 * stops at 99999, and score takes its value. work->collision->hit must hold
 * the hit */
void shooting_score_hit(int points, struct task_work *work);
/* count a target destroyed, toward the share of the act's targets hit */
void shooting_count_hit(void);
/* add a bonus for a carrier fight that took steps to shooting_score and
 * score. it adds 2000 under 30 seconds, 1000 under 60 and 500 under 90, at
 * 60 steps a second. a longer fight earns none */
void shooting_time_bonus(int steps);

#endif
